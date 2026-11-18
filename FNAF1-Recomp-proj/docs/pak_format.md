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
  u32 version      = 1     ; current writer; v2 remains readable as legacy
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
  v2 (36 bytes, legacy experiment): the same fields plus u32 `bits`
                 inserted AFTER `channels`. The current writer does not
                 emit v2; the loader still accepts an old v2 pak.

name pool             ; every name, NUL-terminated, packed in entry order
data blobs            ; textures then sounds, each 16-byte aligned
```

The current writer emits **v1**. A complete RIFF/WAVE blob already carries
its own bit depth, sample rate and channel count, so the later v2 `bits`
word was redundant and has been retired. The loader retains v2 parsing for
old artifacts; `verify_pak.py` selects the entry layout from the version.

## Sound format values

| value | meaning | playback |
|-------|---------|----------|
| 0 | complete RIFF/WAVE PCM, or a legacy headerless PCM blob | existing loader normalization, then XAudio2 |
| 1 | runtime-only flag: input needs decoding / is unsupported | not serialized in current paks |
| 2 | original compressed bytes; currently intentional MP3 | decoded by dr_mp3 in `AudioSystem` |

A fmt=2 RIFF container is offered to dr_wav instead. Ogg is not accepted
by the runtime decoder and therefore remains a pack-time conversion.
Compressed blobs are never submitted to XAudio2 directly: the decoder
produces native-endian PCM16, which the voice slot owns until the voice is
destroyed. `tools/verify_pak.py` validates both WAV chunks and retained
MP3 bytes.

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
* Every other game's bank uses the same v1 layout:
  - 8-bit PCM WAVs (FNaF 2's phone calls and footsteps) keep their complete
    RIFF header and are expanded to 16-bit by the loader. The original
    8000 Hz rate is preserved;
  - MP3s — Sister Location, FNaF World, FFPS and UCN ship them both with
    and without an ID3 tag. Recomp Pack detects MPEG content rather than
    trusting a filename extension and stores the exact compressed bytes at
    `fmt=2`. `src/AudioDecoders.cpp` decodes them with dr_mp3 on playback;
  - Ogg/Vorbis is still decoded at pack time because the game includes no
    Vorbis decoder.
* Nothing is resampled. Runtime MP3 decoding supplies the source rate and
  channel count directly to `WAVEFORMATEX`. Keeping compressed MP3s also
  reduced the four rebuilt paks from 716/1773/1090/834 MiB to
  416.4/1450.6/844.9/468.1 MiB (World/SL/FFPS/UCN respectively).
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
