# Xbox 360 XDK toolset notes (VS2010-era)

Quick reference for the toolchain quirks hit while building this project
against the real XDK headers. Keep this next to the build machine.

## 1. `snprintf` does not exist in the XDK CRT

The XDK CRT is VS2010-era (MSVC 10.0) and predates C99:

| function     | XDK status                                            |
|--------------|-------------------------------------------------------|
| `snprintf`   | **missing** — only `_snprintf` (no NUL on truncation) |
| `vsnprintf`  | present (stdio.h) — returns **-1** on truncation      |
| `sprintf`    | present                                               |

**Rule:** never call `snprintf` in `src/`. Use `fnaf::Snprintf` from
`include/XdkCompat.h` instead. It always null-terminates, never overflows,
and behaves identically on the XDK CRT and modern C99 CRTs.

## 2. XAudio2 flag naming differs from the PC SDK

The XDK `xaudio2.h` names the end-of-stream buffer flag with extra
underscores:

```
XDK:      XAUDIO2_END_OF_STREAM   (0x0040)
PC SDK:   XAUDIO2_END_OFSTREAM    (0x0040)
```

`src/AudioSystem.cpp` contains a compatibility `#define` that normalizes
the PC name to the XDK name, so both spellings compile. `GetState` flags
(`XAUDIO2_VOICE_NOSAMPLESPLAYED`) are unaffected.

## 3. struct/class forward declarations must match

MSVC emits C4099 when a type is forward-declared as `class` but defined as
`struct` (or vice versa). Project types are defined as `struct`
(`PakLoadedTexture`, `PakLoadedSound`) — forward-declare them with `struct`
everywhere. Fixed in `include/GameRender.h`.

## 4. Verified-present XAudio2 API (against the actual XDK headers)

`XAudio2Create`, `CreateMasteringVoice`, `CreateSourceVoice`,
`SubmitSourceBuffer`, `GetState(pVoiceState, Flags)`,
`XAUDIO2_DEFAULT_PROCESSOR`, `XAUDIO2_DEFAULT_FILTER_FREQUENCY`,
`XAUDIO2_LOOP_INFINITE`, `WAVEFORMATEX` (via `audiodefs.h`, included by
`xaudio2.h`).

## 5. C++ features to avoid in this codebase

The XDK compiler is MSVC 10.0: no `std::to_string`, no `stoull`, no
`= delete`, no `override`/`final` context keywords, no `noexcept`.
The codebase currently avoids all of these — keep it that way.
`auto`, lambdas, `nullptr`, `static_assert` are fine.
**`enum class` is NOT fine** — strongly typed enums arrived in MSVC 11.0
(VS2012); plain `enum` works everywhere. Same for C++11 brace-init:
`T var{...}` / `return T{...}` compiles on g++ but is C2143/C2275 on VC10
(use a constructor + `T(a, b)` — see §10).

## 6. Debug output

`printf` compiles and runs, but stdout goes nowhere on the 360 without a
debugger attached (use the XDK debugger's output window or XDM). The in-game
DebugConsole renders the same text on screen.

## 7. Black screen on console — root causes and the v2.2 fix (Xenos GPU)

The v2.1 build booted to a permanently black screen. All facts below were
verified against the real XDK headers (`d3d9.h`, `d3d9types.h`,
`d3dx9shader.h`, `d3dx9tex.h`, `xgraphics.h`) and cross-checked with public
reverse-engineering references (hedge-dev/XenosRecomp, Xenia).

### 7.1 Xenos has no fixed-function pipeline

Unlike PC D3D9, the Xbox 360 GPU executes **only** programmable shaders:
`SetVertexShader(NULL)` does NOT fall back to FFP/T&L, there is no
`SetTransform`, no texture stages, and FVF-style `DrawPrimitiveUP` streams
nothing without an explicit vertex declaration. Every draw needs a real
vertex + pixel shader plus a `D3DVERTEXELEMENT9` declaration
(`CreateVertexDeclaration` / `SetVertexDeclaration` — both present in the
XDK d3d9.h).

### 7.2 PC fxc bytecode never runs on Xenos

The v2.1 scheme compiled `Shaders/*.hlsl` with the **PC** `fxc.exe /T vs_2_0`
and loaded the resulting `.vsh/.psh` from `game:\Shaders\`. Xenos does not
execute PC D3D9 shader tokens — 360 shader binaries are a separate
microcode (this is exactly why XenosRecomp exists to transpile them).
On top of that, "all D3D APIs return S_OK on 360" (see the comment at the
top of the XDK d3d9.h), so `CreateVertexShader` "succeeded" with garbage and
everything failed silently → black screen. v2.2 removed the Shaders/ folder
and the fxc custom-build steps entirely.

### 7.3 Runtime shader compilation IS available on 360

The XDK ships `d3dx9shader.h` with `D3DXCompileShader`; the only profiles
are `vs_3_0`/`ps_3_0` (any `vs_*`/`ps_*` string is promoted, per the header
comment). It emits real Xenos microcode, which `CreateVertexShader`/
`CreatePixelShader` accept directly. v2.2 embeds the sprite shaders as HLSL
strings and compiles them once in `SpriteBatch::Init()` — nothing external
to deploy. Requires `d3dx9.lib`/`d3dx9d.lib` (already in every vcxproj
config).

### 7.4 Matrix constant packing pitfall

`float4x4` constants are subject to the HLSL column_major/row_major packing
convention. A transposed ortho matrix moves the translation term into `.w`,
every pixel fails the clip test and the frame is black even though the
shaders work. v2.2 avoids matrices entirely: the vertex shader takes two
scalar `float4` registers (c0 = scales, c1 = offsets) — packing-independent.

### 7.5 Textures are tiled; LockRect returns tiled bits

Textures created with `D3DFMT_A8R8G8B8`/`D3DFMT_DXT1`/`D3DFMT_DXT5` are
stored in the Xenos **tiled** layout (tiled addressing runs in
texel/block units with dimensions aligned to 32 — see
`XGAddress2DTiledOffset`; that is why the pak pads to 32). `LockRect`
hands out the raw tiled bits, so a raw `memcpy` of linear pak data produced
garbled pixels. The XDK utility `XGTileTextureLevel(Width, Height, Level,
GpuFormat, Flags=0, pDest, NULL, pLinearSrc, RowPitch, NULL)` swizzles a
linear image into the locked level correctly (packed miptails included).
RowPitch is the source stride **per block row** for the compressed formats:
`(W/4)*8` for DXT1, `(W/4)*16` for DXT5, `W*4` for A8R8G8B8. The pak data
itself must already be in the GPU's big-endian byte order (it is — see
docs/pak_format.md).

### 7.6 No silent failures anymore

On the console printf is invisible, so any fatal init failure must be
shown with `XShowMessageBoxUI` (xbox.h; `XMB_ERRORICON`, `MESSAGEBOX_RESULT`,
`XOVERLAPPED` — all verified). v2.2 wires every init step through
`ShowFatalError()`: CreateDevice (with a second, tolerant PresentParameters
attempt), SpriteBatch (reports the D3DXCompileShader error text), and
TextRenderer. If something is wrong you now get a readable dialog instead
of black.

### 7.7 Sprite queue must be flushed once per frame

`SpriteBatch::Draw` only queues quads; they are flushed on texture change
or when the batch is full. Without an explicit `End()` per frame the last
same-texture batch rendered one frame late (or never, on static screens).
`main.cpp` now calls `g_batch.Begin()` in `FrameBegin` and `g_batch.End()`
in `FrameEnd` (before `EndScene`), covering sprites and text alike.

### 7.8 Reference projects

- **hedge-dev/XenosRecomp** (github.com/hedge-dev/XenosRecomp) — converts
  360 shader microcode back to HLSL; proof that 360 shader binaries are
  their own format.
- **Xenia** (xenia.jp) — the 360 emulator; its GPU docs describe Xenos
  render targets/tiling that motivated 7.5.
- XDK header self-documentation: the "D3DCOMPILE_USEVOIDS" comment at the
  top of d3d9.h ("On Xbox 360, all D3D APIs always return S_OK") is the
  key reason failures are silent and must be surfaced in the UI.

## 8. v2.3 — findings verified against the official xbox360sdk.chm

Verified against the user-supplied official XDK documentation
(`xbox360sdk.chm`, extracted to HTML) and the real XDK headers.

### 8.1 DXT textures: the pak's byte order was wrong (the big one)

The Xenos GPU is little-endian; the CPU is big-endian. Official refs:

* **Big-Endian vs. Little-Endian** (Getting Started): PC-authored resources
  must be byte-swapped for the 360; the required swap is encoded in the
  format: **DXT textures = GPUENDIAN_8IN16**, 32 bpp = GPUENDIAN_8IN32.
  8-in-16 = adjacent byte pairs: `abcdefgh -> badcfehg` (NOT word reversal).
* **Xbox 360 Texture and Render Target Layouts** (ATG white paper): a DXT5A
  block in 360 memory is `80 82 ...` with **Anchor0 = 0x82, Anchor1 = 0x80**
  — the anchors appear swapped in memory, exactly the 8-in-16 form.
* **d3d9types.h**: `D3DFMT_DXT1/DXT4/5 = GPUENDIAN_8IN16`,
  `D3DFMT_A8R8G8B8 = GPUENDIAN_8IN32`.

The pak stores DXT blocks as "whole-word big-endian" (a natural but wrong
reading of "the 360 wants BE"). Resulting differences vs the GPU layout:

```
                    PC/LE bytes    pak (BE words)    GPU (8-in-16)
DXT5 anchors        a0 a1          a0 a1             a1 a0
DXT5 alpha idx      i0..i5         i5..i0 (reverse)  i1 i0 i3 i2 i5 i4
DXT* color u16s     lo hi          hi lo             hi lo          (same)
DXT* color idx      i0 i1 i2 i3    i3 i2 i1 i0       i1 i0 i3 i2
```

Fix: `PakLoader.cpp` now converts every DXT block to the GPU layout
(`ConvertDxt1Block`/`ConvertDxt5Block`) before `XGTileTextureLevel`, which
copies bytes verbatim and does no endian fix-up itself. `tools/verify_pak.py`
validates the whole chain pak → GPU → PC → decode against source PNGs.
Host test: `scripts/test_dxt_endian.cpp` (2511 checks, incl. the white
paper anchor example).

### 8.2 Resource pools do not exist on 360

`IDirect3DDevice9::CreateTexture`: **"UnusedPool [in] — Unused; use 0"**.
Same for `D3DXCreateTextureFromFileInMemoryEx`: **"Pool [in] — Unused; use 0"**.
Both callers now pass 0 instead of `D3DPOOL_DEFAULT` (cosmetic but avoids
debug asserts and matches the docs).

### 8.3 Sampler and constant register mapping (verified correct)

* `d3d9gpu.h`: pixel shader samplers s0..s25 = texture fetch constants
  0..25; `SetTexture(0, ...)` + `register(s0)` in ps_3_0 = the same unit.
  Vertex shader textures start at fetch constant 16 (unused by us).
* `SetVertexShaderConstantF(0, ...)` ↔ `register(c0)` (VS base 0).
  `shader_constant_allocation_policy.htm`: register-semantic user constants
  are never truncated — writing them by index with Set*ConstantF is safe;
  compiler literals are allocated from the TOP down, so c0/c1 can't collide.
* `D3DXGetVertexShaderProfile/GetPixelShaderProfile`: **vs_3_0 / ps_3_0 are
  the only profiles on 360** — our runtime `D3DXCompileShader` profiles are
  correct; `ppConstantTable = NULL` is documented as allowed.

### 8.4 Present / BeginScene / LockRect behavior (verified)

* `BeginScene`/`EndScene`: "deprecated and does nothing" on 360 — harmless.
* `Present`: EDRAM does NOT persist across frames (debug builds fill it
  with random values) — full Clear+redraw every frame is mandatory (we do).
* `LockRect` on DEFAULT-pool textures: allowed; the locked bits are TILED
  ("The Windows trick of row*pitch+col*texelsize does not work"). Pixel
  addressing requires XGAddress2DTiled* / XGTileTextureLevel — implemented.
* Sampling a NULL texture: on 360 this asserts (PC returns black) — every
  draw path must bind a real texture first.
* `D3DPRESENT_PARAMETERS`: `Windowed` must be FALSE; `BackBufferHeight`
  must be even; `FullScreen_RefreshRateInHz` must be 0 or 60;
  `BackBufferCount`/`SwapEffect`/`hDeviceWindow` are ignored — our
  1280x720 config is compliant.
* Texture max dimensions: 8192x8192 (largest pak texture: 1600x736).

## 9. v2.3.1 hotfix — compile errors found by the real VS2010 build

The first Xbox build of v2.3 failed in `SpriteBatch.cpp` (29.08.2026 build
log). Two authoring bugs and two latent MSVC issues were found and fixed;
the whole tree was then re-verified on the host with g++ against faithful
transcriptions of the real XDK signatures (`scripts/xbox_stub/*.h`,
17/17 .cpp compile clean in `-fsyntax-only`).

1. `src/SpriteBatch.cpp` — the file header comment contained the literal
   sequence `vs_*/ps_*`; the `*/` inside it **closed the block comment
   early**, so 18 lines of prose were parsed as C++ (the avalanche of
   C2143/C2059/C2017 "promoted/there/column_major" errors). Reworded; note
   added to never put a `*`+`/` pair inside a comment.
2. `include/SpriteBatch.h` — `SpriteBatch::SetupRenderState()` was defined
   in the .cpp but **not declared in the class**, producing C2039 plus a
   cascade of C2065 for every member used inside it (m_device,
   m_currentTexture, m_vertexDecl, m_vertexShader, m_pixelShader). Added
   the private declaration.
3. `src/TextRenderer.cpp` / `src/PakLoader.cpp` — v2.3 replaced the D3DPOOL
   argument of `D3DXCreateTextureFromFileInMemoryEx` / `CreateTexture` with
   the literal `0` (per the XDK docs: "Unused; use 0"). MSVC has **no
   implicit int→enum conversion**, so both sites would have failed the next
   build with C2664. Now written as `(D3DPOOL)0`.
4. Host verification against the real XDK headers also confirmed (no code
   change needed): `D3DVOID` = `HRESULT` unless `D3DCOMPILE_USEVOIDS`;
   `Direct3DCreate9` returns a non-null dummy and `CreateDevice` is a
   **static** member of `Direct3D` (our two-step call compiles and works);
   `XAUDIO2_PROCESSOR` is a plain `UINT32` typedef; `XAUDIO2_VOICE_STATE`
   has `BuffersQueued`; `CreateSourceVoice`/`CreateMasteringVoice` arities
   match our calls; `XGTileTextureLevel` takes 10 args as called.

## 10. v2.6.1 hotfix — C++11 brace-init broke the real VS2010 build

The first XDK build of v2.6 failed with 100+ errors in `PakAssets.h`
(lines 162–196): every `return PakHotspot{ -38, -7 };` produced the triple
`C2143 (missing ';' before '{')` + `C2275 (illegal use of this type as an
expression)` + `C2143 (missing ';' before '}')`. The same brace-init was
about to fail in `PakMapButtonOf` (`PakMapBtn{...}`) once the count
cooled below the error cap.

Root cause: brace-init (`T x{a, b}`) is C++11 syntax. The XDK compiler is
MSVC 10.0 (C++03); it parses `PakHotspot` in expression position → C2275.
Our host checker missed it because g++ `-std=c++11` accepts the form.

Fixes applied:

1. `include/PakAssets.h` — `PakHotspot`/`PakMapBtn` now have real
   constructors `T(i32 px, i32 py)` (with explicit narrowing casts, so no
   C4244) and all 39 call sites use `PakHotspot(x, y)`.
2. `scripts/check_xdk_syntax.sh` — added a **strict `-std=c++03` pass**
   (with `-Dnullptr=0`, the only C++0x-ism this tree uses; VC10 itself has
   nullptr) so brace-init and any other C++11-only syntax now fails the
   host check exactly like the real build.
3. `src/Game.cpp` — silenced warning C4244 (f64→f32) in
   `ProcessPowerOut()` with an explicit `static_cast<f32>`.

Rule of thumb for this codebase: aggregates get constructors when they are
constructed with arguments; never write `T{...}` anywhere in `src/` or
`include/`.
