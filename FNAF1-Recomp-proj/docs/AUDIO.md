# Audio & render wire-up — FNaF 1 Recomp (Xbox 360)

Everything in this document is data-driven: sound names and animation frame
tables were extracted from the original Clickteam executable with
**ctfak-cpp** (tools: Events Listing, JSON export, Frame Layout Renderer).
Each entry cites the originating event group of the office/title frame.

## v2.6 — sound pipeline fix (the "noise" bug)

The original 52 sounds (ctfak "Sound Dumper" on the real exe) are:

- **all PCM** (`WAVE_FORMAT_PCM`), but with everything varying:
  - `data` chunk offset **44..172** (13 files carry LIST/INFO chunks),
  - sample rates **11025 / 22050 / 44100**, channels **1..2**,
  - **XSCREAM.wav is 8-bit unsigned** (the only one; playing it as 16-bit
    is pure noise, and it is the jumpscare sound).
- Additionally, paks written by a big-endian tool may store the 16-bit
  payloads **byte-swapped** — playing BE PCM as LE is pure noise on every
  sound.

`PakLoader::NormalizeSounds()` (runs once in `Load()`) now turns any
variant into plain little-endian PCM16:

1. blobs starting with `RIFF....WAVE` are chunk-walked (sizes may be LE
   or BE) and the entry is re-pointed at the `data` payload with the real
   rate/channels/bits;
2. byte order is decided by comparing the mean |amplitude| of each
   stream read both ways — the wrong order moves the near-uniform low
   bytes into the high position and inflates the amplitude several-fold.
   Measured on the real bank: **50/51 sounds separate by 3.1x..88x**
   (XSCREAM2 near-ties and abstains). The pak votes as a whole, so one
   edge-case sound cannot flip the decision; the literal white-noise
   loops (`static`, `static2`) do not vote;
3. 8-bit payloads (XSCREAM by name+size when headerless) are expanded to
   16-bit into owned buffers (XAudio2 2.x has no 8-bit format);
4. unsupported codecs keep `format=1` and are skipped by AudioSystem.

`tools/fix_pak_sounds.py` does the same OFFLINE on the pak (with a
per-sound report and `--force-swap/--force-native/--sounds-dir` overrides)
— the runtime fix makes it optional.

## v2.7 — Xbox 360 PCM byte order (the "noise is STILL there" bug)

v2.6 normalized every sound to **little-endian** PCM16 — correct on PC,
still pure noise on the console. The XDK doc **"Audio Data and
Endianness"** (`audio_overview_xaudio2_endianness.htm`):

> On Xbox 360, PCM audio whose WAVEFORMATEX format contains a
> wBitsPerSample value of 16 or 32 should byte-swap its audio data.
> If wBitsPerSample is 16, each pair of bytes in the audio data should
> be exchanged.

The 360 XAudio2 mixes on the big-endian PPC and does no conversion of its
own. `NormalizeSounds()` therefore has a **pass 4** (only compiled for
`_XBOX`/`_M_PPCBE`): every 16-bit payload is byte-swapped to BIG-endian
before playback. The normalization target is now platform-dependent:
LE on PC, BE on the 360. Everything else (RIFF peel, majority vote,
8-bit expansion) is unchanged. The offline `fix_pak_sounds.py` remains
PC-targeted — do NOT use its output as "already fixed" on the console;
the runtime pass 4 handles the swap either way.

## v2.7 — render fixes from the console screenshots

1. **White bars under the title/HUD text images** — `DrawInstance` sampled
   UVs 0..1, pulling the 64-aligned padding rows into the draw rect (the
   "New Game" image is 203x33 inside a 64-tall texture = 48% padding).
   Now clamped to `orig/aligned` like `DrawTex`.
2. **Purple slab over the desk / camera view** — the office frame's
   'flip up'/'flip down' objects (img_156/img_162) are SOLID VIOLET
   (123,43,127) Clickteam zone-marker fills, not bar art. Both replaced
   by the real bar img_420 ('flip panel', 600x60 @ (554,668), hotspot
   (299,30)).
3. **"Demo" watermark on the title** — the 'demo' object 572 on the title
   frame is a leftover of the shared MFA project: in this build it is
   event-gated off (the 'DEMO?' counter is never set anywhere, so it stays
   0 and the watermark never shows — see docs/EXE_VERSION.md). Watermark
   no longer drawn. v2.7.4 correction: the source exe is the FULL Steam
   game, not the demo.
4. **Office HUD collisions** — "Night N" moved under the clock (demo
   frame parks the word at rows 74..88 inside the clock digits'
   59..97); usage meter bars to the frame-data position (120,657);
   power digits y 646.
5. **Office atmosphere** — original layer 2 restored: animated static
   (obj 42) + vignette img_11 over the scene, under the HUD.

The desk fan (img_57/59/60 @ (868,400)) is an original office object.
The desk pumpkin (img_628..635 @ (734,485)) IS in the frame data, but it
is gated there by the 'Date & Time'/'month'/'day' objects — a
Halloween-only easter egg; the real game's desk has no pumpkin on
regular days (verified against a real-game screenshot). v2.7.1 removed
the pumpkin from the default render.

## v2.7.1 — office HUD matched to a real-game reference

The user supplied a full-game screenshot (Night 7). Layout truths that
differ from the raw frame data (counters are event-driven):

- the clock reads "H AM" on ONE row: hour digits right-aligned against
  the AM image (img_251 @ (1198,31)); the 'time of day' counter
  anchor (1185,59) is demo-leftover layout — it collides with the night
  row;
- "Night N" is the small row right under it (word @(1148,74) + digits
  on the same line);
- "Power left: NN%" is one compact line (digits right after the label,
  same rows 632..646);
- the usage bars sit on the "Usage:" line (y 664);
- no desk pumpkin (see above).

Lesson (again): counter positions in the frame dump are where the
COUNTER OBJECTS sit, not where the digits end up after the events run —
when a real-game reference exists, trust the reference.

## Sound playback (AudioSystem, XAudio2)

`AudioSystem` plays normalized 16-bit PCM from `fnaf1.pak`
(`snd_*` entries). 16 concurrent source voices, per-sound Stop,
infinite loop supported. The debug line at boot prints the normalization
report: `Pak sounds: riff=N swapped=N 8bit=N`.

## Flow wiring (main.cpp)

| Moment | Sound(s) | Source |
|---|---|---|
| Boot -> disclaimer | (silent + static) | title frame String obj 0 |
| Disclaimer -> title | `static2` loop + `darkness music` loop | title group 2 |
| Menu navigation / select | `blip3` | title groups 28-31 |
| Night card -> office | `ColdPresc B` + `Buzz_Fan_Florescent2` + `BallastHumMedium2` loops | office group 14 |
| Phone Guy (2.5 s into night) | `voiceover1c` .. `voiceover5` by night | office groups 361-365 |
| MUTE CALL (B) | stops the voiceover | MUTE CALL img_481 blinks |
| Door close | `SFXBible_12478` | office groups 95-104 |
| Camera up/switch | `CAMERA_VIDEO_LOA_60105303` + `static` loop | office groups 129/143 |
| Bonnie/Chica move | `deep steps` | office groups 198-243 |
| Freddy move | `Laugh_Giggle_Girl_1` | office groups 279-282 era |
| Foxy stage 3 (running) | `run` + `running fast3` loops | office group 39 |
| Foxy bangs door | `DOOR_POUNDING_ME_D0291401` + `knock2` | office groups 270/323 |
| Power out | stop loops, `powerdown`, then `circus` loop | office groups 285/269 |
| Jump scare (any) | `XSCREAM` | office groups 228/322/408 |
| 6 AM | `chimes 2` + `CROWD_SMALL_CHIL_EC049202` | "the end" frame |
| Game over | `static2` loop | frame "died" |

## Jump scare rendering (GameRender, real frame tables)

Fullscreen 1600x720 sequences from object **Active 3** (office frame):

| Animatronic | Animation | Frames (img handles) |
|---|---|---|
| Freddy | anim 65 | 519,485,521,489,490..518 (31 frames @25fps + shake) |
| Foxy | anim 52 | 413,242,415,243,396..412 (25 frames) |
| Bonnie | anim 34 | 225 (fullscreen scare) |
| Chica | anim 43 | 227 (fullscreen scare) |
| Power-out flicker | anim 51 | 241x3,340,244..250,280,282 |
| IT'S ME flash | obj "Active 21" | 525,543,520,544 |

## Menu rendering (exact data coordinates, 1280x720 title frame)

- bg `img_431` (+ rare flickers 440/441/442), static cycle [18,20,12,13,14,15,16,17]
- new game `img_448` @(275,420), continue `img_449` @(275,492)
- night word `img_475` @(174,512) + night number counter @(263,535)
- stars `img_432` @(200/277/352,338), 6th night `img_443`, custom night `img_526`
- disclaimer text (String obj 0): "WARNING! This game contains flashing
  lights, loud noises, and lots of jumpscares!" — centered at boot

## v2.7.4 — the "Steam screenshot effect" from real data (no shaders)

The grainy analog look on the Steam screenshots is NOT a shader. The
Clickteam MMF2 runtime has no shader support at all; the whole look is:

1. **Darkness + vignette are baked into the pre-rendered room art** —
   img_39 measures ~17/255 mean luminance with darker edges. There is no
   vignette object in the frame data.
2. **One fullscreen animated gray-noise sprite** (obj 'static', frames
   img_12..img_20, 8 opaque grayscale frames at ~12 fps) drawn over the
   scene with Fusion semi-transparency.
3. The original transparency values were recovered from the object data
   in this update: `inkEffect=1` (semi-transparency), and Fusion stores
   the transparency COEFFICIENT (0 = opaque, 255 = invisible):
   - office/title/camera static: coeff 100 -> runtime alpha 155/255 =
     **0.608** (v2.7.3 and earlier used a guessed 0.20 — that is why the
     recomp office looked cleaner than the real game);
   - 'mute call' button: coeff 50 -> alpha 205/255 = **0.804**;
   - 'blip flash' overlays: coeff 0 -> fully opaque (night-card flashes).
4. **img_11 ('frame' @ (0,-1)) is 100% transparent (A=0)** — it draws
   nothing and is not a vignette; the office draw of it was removed.

So the authentic recipe, verified against the data: dark pre-render +
noise sprite at (255-coeff)/255. A real shader post-process (ps_3_0
grain/vignette RT pass) is possible on the 360 if the user ever wants
the grain OVER the HUD, but the original does not do that.
