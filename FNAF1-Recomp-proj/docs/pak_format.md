# fnaf1.pak -- asset container for the FNaF 1 recompilation

`fnaf1.pak` is the asset pack consumed by `PakLoader.cpp` at runtime.
It is built directly from the original game's embedded data with the
clean-room extraction tool **ctfak-cpp** (tool #10, "Recomp Pack"):

```
ctfak-cpp -path FiveNightsatFreddys.exe -tool "Recomp Pack" -dumps <outdir> -closeonfinish
```

The tool parses the Clickteam Fusion executable, decodes every image and
sound, and repacks them GPU-ready. No intermediate files are required.

## Layout (all integers big-endian)

```
header (32 bytes)
  u32 magic        = 'FNAF' (0x464E4146)
  u32 version      = 2     ; v1 kept for the untouched fnaf1.pak
  u32 numTex
  u32 numSnd
  u32 numMus       = 0
  u32 numFnt       = 0
  u32 stringsOffset   ; absolute offset of the name pool
  u32 dataOffset      ; absolute offset of the first data blob

texture entry (36 bytes, numTex times)
  u32 nameOffset      ; absolute, NUL-terminated string
  u32 dataOffset      ; absolute
  u32 dataSize
  u32 format          ; 0 = DXT1, 2 = DXT5, 3 = A8R8G8B8
  u32 origW, origH    ; real image size
  u32 alignW, alignH  ; data size basis (both rounded up to 32)
  u32 mip             ; 1

sound entry
  v1 (32 bytes): u32 nameOffset, dataOffset, dataSize, format,
                 sampleRate, channels, loopStart, loopEnd
  v2 (36 bytes): the SAME eight u32 plus a u32 `bits` inserted AFTER
                 `channels` — nameOffset, dataOffset, dataSize, format,
                 sampleRate, channels, bits, loopStart, loopEnd

name pool             ; every name, NUL-terminated, packed in entry order
data blobs            ; textures then sounds, each 16-byte aligned
```

**v2 exists because the bit depth was being lost.** The loader used to
assume 16-bit, and FNaF 2's phone calls (`call 1b`..`call 6b`) are 8-bit
8000 Hz PCM — played as 16-bit they are pure noise. In a v2 pak the
loader reads `bits` and expands 8-bit payloads to 16-bit at load.

The name-pool offset MUST be computed from the v2 entry size
(`numSnd * 9 * 4`): computing it with the v1 size shifts the pool and
every name in the pak (that bug shipped once; `verify_pak.py` now
parses the table the way the loader does and would catch it).

## Sound format values

| value | meaning | playback |
|-------|---------|----------|
| 0 | raw PCM16 (headerless; 8-bit sources were expanded to 16 at pack time) | plays |
| 1 | runtime-only flag: unsupported codec | skipped in silence |
| 2 | passthrough — compressed bytes the packer could not decode | skipped in silence |

The console's XAudio2 2.x decodes raw PCM only (plus XMA2/WMA through
the XDK's own paths, which this port does not use), so a rebuilt pak
must contain nothing but `format = 0`; `tools/verify_pak.py` asserts it.

## Texture encoding

* Source pixels are the game's decoded RGBA images.
* Any image containing a transparent pixel becomes **DXT5** (format 2),
  fully opaque images become **DXT1** (format 0).
* Both dimensions are padded to multiples of 32 with edge replication;
  `alignW`/`alignH` describe the padded size and `dataSize` always equals
  `alignW * alignH * bitsPerPixel / 8` (`4` bits for DXT1, `8` for DXT5),
  so the console loader's `memcpy` into the locked rect is always safe.
* All multi-byte words inside DXT blocks are stored **big-endian**
  (16-bit color endpoints byte-swapped, index payloads byte-reversed).
  NOTE (v2.3): that is NOT the byte order the Xenos texture unit expects.
  Per the XDK docs ("Big-Endian vs. Little-Endian": DXT =
  GPUENDIAN_8IN16 = adjacent byte-pair swap; ATG white paper "Xbox 360
  Texture and Render Target Layouts": anchors appear as `a1 a0` in 360
  memory), the GPU layout is the PC block with an 8-in-16 swap applied.
  `PakLoader.cpp` therefore converts every DXT block from the pak's
  big-endian-word encoding into the GPU's 8-in-16 layout before
  `XGTileTextureLevel` (see `ConvertDxt1Block` / `ConvertDxt5Block`).
  `tools/verify_pak.py` validates the same end-to-end chain against the
  source PNGs. Padding regions replicate the edge pixels, so linear
  filtering never bleeds in black.
* Texture names are `img_<original image handle>`, e.g. `img_39`.

## Sounds

* FNaF 1's 52 bank sounds are all RIFF/WAVE PCM; the loader strips the WAV
  header and stores raw PCM with `format = 0` plus the sample rate and
  channel count from the `fmt ` chunk. **fnaf1.pak stays v1** — its one
  8-bit sound (`XSCREAM`) is caught by the loader's name+size fallback, so
  the file does not need a rebuild.
* Every other game's bank carries sources the console cannot play:
  - 8-bit PCM WAVs (FNaF 2's phone calls and footsteps) — expanded to
    16-bit at pack time, the source's 8000 Hz sample rate is kept as is
    (it is the ORIGINAL rate; resampling would only add artefacts);
  - MP3s — Sister Location, FNaF World, FFPS and UCN ship them both with
    and without an ID3 tag (the headerless ones used to be misnamed
    `.wav` by the extension sniffer). They are decoded to PCM16 at pack
    time through the platform's own media tool (ffmpeg, then sox), so no
    third-party decoder code lives in this repository and the result is
    bit-exact against the reference decoder.
* Nothing is resampled: a sound keeps the sample rate and channel count
  of its source, so the conversion itself adds no artefacts. The pak
  therefore grows (MP3 47.6 MB -> ~176 MB of PCM across the four games)
  — the price of the console having no MP3 decoder.
* Sound names are `snd_<internal name without extension>`,
  e.g. `snd_Buzz_Fan_Florescent2`.

## Current contents

605 textures (245 DXT1 + 360 DXT5, 149.0 MB) and 52 PCM sounds (92.8 MB),
241.8 MB total. `pak_manifest.json` lists every entry; `asset_mapping.hpp`
is the regenerated ground-truth `{handle, name, w, h, alpha}` table.

## Key texture names

| name      | content                        | size      |
|-----------|--------------------------------|-----------|
| `img_39`  | office parallax background     | 1600x720  |
| `img_431` | title/menu screen              | 1280x720  |
| `img_11`  | camera monitor frame           | 1280x720  |
| `img_18`  | video static overlay           | 1280x720  |
| `img_164` | camera floor plan ("YOU")      | 400x400   |
| `img_103` / `img_119` | left / right door  | 223x720 / 248x720 |
| `img_129` | door & light button            | 62x120    |
| `img_167` | camera map button              | 60x40     |
| `img_481` | mute call button               | 121x31    |

The full semantic list lives in `include/asset_mapping.h`.
