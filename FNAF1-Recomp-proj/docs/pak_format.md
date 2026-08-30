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
  u32 version      = 1
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

sound entry (32 bytes, numSnd times)
  u32 nameOffset, dataOffset, dataSize
  u32 format          ; 0 = raw PCM, 2 = passthrough
  u32 sampleRate, channels, loopStart, loopEnd

name pool             ; every name, NUL-terminated, packed in entry order
data blobs            ; textures then sounds, each 16-byte aligned
```

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
  channel count from the `fmt ` chunk.
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
