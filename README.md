# FNAF Recomp — the classic FNAF series for Xbox 360

> ⚠️ **Use a separate/offline Xbox 360 profile for this project.**
> **This is an unofficial homebrew/clean-room reconstruction and recompilation project. Do not use Xbox Live with it.**

What started as a from-scratch reimplementation of FNAF1 on raw Xbox 360 XDK
(D3D9 + XAudio2, VS2010/PPC, C++03 — no lambdas, no nullptr) is now a
**MULTI-GAME shell**: one codebase hosts the whole classic series as
plug-in modules, each with its own assets bundle, renderer and game logic —
all dump-driven and tested on real hardware:

| Game | Ready | Implemented |
|---|---|---|
| **FNAF 1** | **~97 %** | everything: nights 1–7 + Custom Night, full dump-mirrored AI, cameras/doors/lights, jumpscares, power-out, Golden Freddy, hallucinations, phone call, saves, achievements. Left: the "died" noise screen, a few rare easter eggs |
| **FNAF 2** | **~35 %** | disclaimer, title (the "from Freddy's eyes" bg roll + glitch + audio), night loop (70 s/hour clock → 6 AM), panning dark office, hold-flashlight + battery (dump rules incl. BB steal), 12-camera monitor + map panel/buttons. Left: phone call, mask, vents, music box, the 9-animatronic AI (751 event groups), jumpscares/death, office sound loops, saves, 8-bit minigames |
| **FNAF 3** | **~15 %** | disclaimer, title (Springtrap scene + static bursts + titlemusic), the 2000-wide office with pan. Left: the whole game — vent/audio-only mechanic, phantom AI, minigames, saves, cutscenes |
| **FNAF 4** | **~10 %** | disclaimer, title (red sky + heading + menu), the 1300-wide bedroom with pan, streaming pak load. Left: the whole game — door/closet/bed mechanics, nightmare AI, saves |
| **Sister Location** | **~3 %** | dump + pak + startup flow pinned (Warning → Elevator/HandUnit voice → title); the streaming loader is built. Left: everything visual and logical |

One boot shows a **game selector** when more than one bundle is present.
Every game ships its own `.spa` (Xbox identity/achievements) and its own
`*.pak` (textures + sounds, built by our tool from that game's own dump).

Русская версия: [README.ru.md](README.ru.md).

> **Project rule:** the only source of truth is the **game dump**, taken with
> our own clean-room tool **[CTFAK-CPP](https://github.com/Sergey004/ctfak-cpp)**.
> Wikis, third-party ports and "I remember it being this way" are not
> sources — they are checklists of what to look up in the dump. Every
> deliberate deviation from the dump is labelled as such in the code and in
> the changelog.

## Status (v2.32)

- **FNAF1 — fully playable, 1:1**: nights 1–7 + Custom Night, the full
  animatronic AI taken from the dump's event groups, jumpscares, power-out
  sequence, Golden Freddy (1/100 poster roll → office → kill screen +
  force-close), XContent saves/achievements, loose-save import; sounds and
  animation speeds verified against the dump (`fps = speed × 0.6`).
- **FNAF2 — playable skeleton**: per-game disclaimer, the title with the
  dump's "video from Freddy's eyes" background roll, the panning dark office
  with a hold-flashlight and battery, the 12-camera monitor with the map,
  the 70 s/hour night clock. Phone call, mask, vents, music box and the
  animatronic AI are the next stages.
- **FNAF3 / FNAF4 — test levels**: titles and offices render from their
  dumps (FNAF4 boots through the streaming loader — its pak does not fit
  the 512 MB UMA pool eagerly).
- **Core**: `AppModule` contract, per-game renderers, boot selector, soft
  pak scan, streaming pak loader, per-game disclaimers and title ambience.
  Full session history: [CHANGELOG.en.md](CHANGELOG.en.md) /
  [CHANGELOG.ru.md](CHANGELOG.ru.md).

Full history: [CHANGELOG.en.md](CHANGELOG.en.md) /
[CHANGELOG.ru.md](CHANGELOG.ru.md).

## Architecture

```
CORE (app shell — main.cpp + shared systems)
  D3D9 60 Hz loop · SpriteBatch/render targets · TextRenderer
  PakLoader (eager + streaming) · AudioSystem (XAudio2 dB mixer)
  InputSystem (XInput) · fades · XContent saves · achievements · debug console
────────────────────────────────────────────────────────────
AppModule (contract: Name/PakName/Load/Tick/Render/WantsExit)
────────────────────────────────────────────────────────────
FNaF1Module (active)   FNaF2Module   FNaF3Module   … (FNAF4/SL/UCN)
```

- The core never includes game headers; a module never owns the device — it
  receives `AppServices{audio, pak, batch, text}`.
- Registry: `include/AppRegistry.h`. The full stage-by-stage migration plan:
  [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Building and running

1. **Toolchain**: XDK build — VS2010 + Xbox 360 XDK (`FNAF1-Recomp-proj.vcxproj`).
   Cross-platform is not a goal; the PC paths in the code are stubs.
2. **New files** must be added to the `.vcxproj` manually (the project tree is
   synced between machines; the file list is simply `src/` + `include/`).
3. **Assets**: `fnaf1.pak` must sit next to the XEX (see "Asset pipeline").
   Without it you get the system "Missing fnaf1.pak" message box.
4. **Running**: Xenia for fast iteration, or a console (RGH/JTAG — the game
   runs from HDD). When several `*.pak` bundles are present, boot shows a
   **game selector** (D-pad select, A launch, B = FNAF1). Command-line
   arguments: `1..7` picks the starting night, `fnaf2`/`fnaf3`/`fnaf4`/`sl`
   force a module (dev).
5. **Two builds**: default — system (saves/achievements in XContent);
   `FNAF_LIVE_SAFE` — the live-safe variant (save at `game:\save\freddy`,
   local achievements).

## Controls (console scheme)

| Action | Button |
|---|---|
| Look around the office | Left stick ←→ |
| Door lights | LB / RB |
| Doors | LT / RT (analog) |
| Camera tablet | A |
| Camera navigation | D-Pad |
| Pause | Start |
| Back | B |
| Freddy's nose (office) | Y |
| DEV menu | Start+B |
| Sprite browser | LB+RB (in menus) |
| Perspective tuner | L3+R3 (Y — reset knob) |

## Asset pipeline (CTFAK-CPP)

Tool: **[ctfak-cpp](https://github.com/Sergey004/ctfak-cpp)** — a clean-room
C++ reimplementation of a Clickteam Fusion runtime dumper/repacker. The full
cycle for any game in the series:

```bash
# 1) dump assets (images/sounds/packed data):
./build/ctfak-cpp -path <game>.exe -tool "Dump Everything"         -closeonfinish
# 2) events (per-frame Groups/ON/DO) — the source of game logic:
./build/ctfak-cpp -path <game>.exe -tool "Events Listing"           -closeonfinish
# 3) structure (application.json: objects, frames, animations):
./build/ctfak-cpp -path <game>.exe -tool "Export Structure as JSON" -closeonfinish
# 4) GPU-ready pak + asset table (DXT textures + PCM, big-endian 'FNAF'):
./build/ctfak-cpp -path <game>.exe -tool "Recomp Pack"              -closeonfinish
```

Results land in `build/Dumps/<Game>/`:

- `Events/*.txt` + `ALL_EVENTS.txt` — the event program (conditions/actions/
  parameters);
- `JSON/application.json` — objects/frames/animations;
- `Images/`, `Sounds/`, `Packed Data/` — raw assets;
- `RecompPack/<game>.pak` + `pak_manifest.json` + `asset_mapping_<game>.hpp`.

**Asset tables** (`include/assets/asset_mapping_*.hpp`): each lives in its own
namespace (`fnaf1::assets`, `fnaf2::assets`, …, `sisterlocation::assets`)
with `ASSET_COUNT`; a game module includes only its own table — no symbol
collisions. They are the ground-truth {index, filename, w, h, alpha} used for
verification and for the per-game asset tables of stage 4.

**Streaming**: `PakLoader::LoadStreaming()` ("sliding over the file" — only
tables stay resident, blobs are read at their `dataOff` on first use, and
`PreloadAsync` pulls the next room in the background) will be activated by
the SL module; SL's pak is ~1.5 GB, which cannot fit the 512 MB UMA pool
eagerly. FNAF1/2/3/4 stay on the eager `Load()`. The games run **from HDD**,
so the XDK's DVD stack (XFileCache, ReadFileScatter, physical sort keys)
does not apply.

## Saves

- The save file is the original extension-less **`freddy`** (INI:
  `[freddy]` with `level`/`beatgame`/`beat6`/`beat7`). The system build uses
  an XContent content named `freddy`; Live Safe uses `game:\save\freddy`.
- At boot the system build detects a loose `freddy` next to the XEX and
  offers to import it into XContent ("Do you want to import the save found
  in the game folder?" → "Import complete. Please restart the game." + exit).
- Achievements are separate (`fnaf_ach.ini`); by default they go through
  `XUserWriteAchievements`.

## Documentation (`docs/`)

| File | Contents |
|---|---|
| `ARCHITECTURE.md` | core+modules, dumps of every game, migration stages |
| `AI_MECHANICS.md` | the animatronic AI, 1:1 with the dump's event groups |
| `CAMERA_FINDINGS.md` | the canonical camera-feed table |
| `AUDIO.md` | sound bank, channels, action numbers |
| `OVERLAY_MAP.md` | overlays/statics of every screen |
| `PERSPECTIVE.md`, `PERSPECTIVE_TUNER.md` | the office parabola and its tuner |
| `TABLET_FLIP.md` | the tablet raise/lower animation |
| `RESTORE.md`, `XDK_NOTES.md`, `pak_format.md` | restoration, XDK notes, pak format |

## Versioning

`v2.XX` is bumped in the banners of `src/main.cpp` and in both changelogs at
the same time. Rule for behavior disputes: read the event groups in
`Events/*.txt`, not the wiki; a deviation from the dump gets its own
"deliberate deviation" entry.

## Disclaimer

Unofficial fan reimplementation for personal console use. A legally purchased
copy of the original game is required — all assets are extracted from it. Do
not distribute the assets or built images.
