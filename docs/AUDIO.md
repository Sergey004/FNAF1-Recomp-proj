# Audio & render wire-up — FNaF 1 Recomp (Xbox 360)

Everything in this document is data-driven: sound names and animation frame
tables were extracted from the original Clickteam executable with
**ctfak-cpp** (tools: Events Listing, JSON export, Frame Layout Renderer).
Each entry cites the originating event group of the office/title frame.

## Sound playback (AudioSystem, XAudio2)

`AudioSystem` plays raw 16-bit PCM blobs from `fnaf1.pak`
(`snd_*` entries written by ctfak-cpp "Recomp Pack").
16 concurrent source voices, per-sound Stop, infinite loop supported.

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
