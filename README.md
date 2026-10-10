# FNAF Recomp — the classic FNAF series for Xbox 360

> [!WARNING]
> **Use a separate/offline Xbox 360 profile for this project.**
> **This is an unofficial homebrew/clean-room reconstruction and recompilation project. Do not use Xbox Live with it.**
>
> **"Crash" during Golden Freddy's kill is a FEATURE.** After his
> full-screen face appears, the game intentionally closes the title the same
> abrupt way the ORIGINAL PC release does — it looks like a crash but is a
> safe easter egg: only the game quits in to BSOD, the console is never harmed, and it is avoidable by raising
> the Monitor in time. All other exits (incl. missing pak and the save
> import restart) go to the dashboard through `XLaunchNewImage`.

What started as a from-scratch reimplementation of FNAF1 on raw Xbox 360 XDK
(D3D9 + XAudio2, VS2010/PPC, C++03 — no lambdas, no nullptr) is now a
**MULTI-GAME shell**: one codebase hosts the whole classic series as
plug-in modules, each with its own assets bundle, renderer and game logic —
all dump-driven and tested on real hardware:

| Game | Ready | Implemented |
|---|---|---|
| **FNAF 1** | **100 % (the most well-tested one of all, can be completed)** | everything: nights 1–7 + Custom Night, full dump-mirrored AI (three-pass audit), cameras/doors/lights, jumpscares with the dump's exact timings, power-out (the real music box jingle + the dark-Freddy kill), Golden Freddy, hallucinations, phone call, pad rumble, all rare easter eggs (cove "IT'S ME", 1/10000 creepy, door jam). The death pipeline is dump-exact: scare → the "died" screen (fullscreen static + the one-pass white blip flash + the static loop on ch1, 10 s) → the backroom "gameover" (10 s, the 1/10000 creepy roll re-fires every second) → title. Controls match the official console port: LS pan, LB/RB lights, LT/RT doors, A raises the monitor and confirms the highlighted cam, D-pad moves the cam pre-selection, B exits camera mode / mutes the call, Y honks the nose. Saves (XContent + loose import) and achievements (official XLAST spa) are **HW-verified on a signed-in profile**. Left: nothing — only the documented deliberate deviations (Golden Freddy's shortened creepy screen + force-close, the power-out "steps" window; the EERIE/robotvoice room test is documented as exactly equivalent to the dump's vol-zone overlap) |
| **FNAF 2** | **100 % (code, no game flow)** | the complete frame flow: dump-exact title, New Game → the HELP WANTED newspaper → the night card (the REAL per-night art cells, rare 1/1000) → the office, 6 AM (the REAL 18-cell 5→6 roll + the save) → the panning dream → "it's me"/"err" → the next night; the endings (paycheck/pink slip/robots-scrapped) writing beatgame; the custom night setup with the ten verbatim challenge presets (the sliders BECOME the office AI); death → the Game Over face → (1/1000) the 8-bit chain: the SAVETHEM overworld (5x5 wrap grid), TAKE CAKE, GIVE GIFTS GIVE LIFE, Foxy's party, the turn rotation loader. The **freddy2 save** (level/cine/turn/beatgame/beat6/beat7/c1..c10, XContent + fallback, X-hold wipe) and the **add-on achievements** (slots 0-9 → spa ids 11..20, fnaf2_ach.ini, profile-write ready). The minigames' movement engine (grid-step + gate + sensors) is the reusable 8-bit model for FNAF3/4/SL. Left: NOTHING in code — the one-time .xlast task (enter the 10 add-on entries per docs/FNAF2_DLC_ACHIEVEMENTS.md §3b) and the Mangle office-crawl visual stub (labeled; her kill works) |
| **FNAF 3** | **100 % (code, no game flow)** | disclaimer (2 s, the 1/1000 rare boot), title (Springtrap scene + static bursts + titlemusic), the full night loop (FNaF3Game): hour clock (40 s night 1 / 60 s), the Springtrap room graph (10 rooms + 5 vents + monitor-gated attack stages), vent sealing (with the progress bar), the audio lure with the adjacency table, the five phantoms + Golden/Shadow Freddy, the maintenance panel (meters, reboots, the vent-error blackout + hallucinations + the 204 office foreground state), 15 cams with the dump feed table, jumpscares 778/792, the GOT YOU 2 window scare. **v2.63 — the whole dump flow, slot-resolved**: the jump values turned out to be STORYBOARD-SLOT indexed (the dumped names were file-indexed and wrong; resolved through the chunk-8747 handle list) — death now → the static frame (stare + flash, 5 s) → the Game Over screen (5 s / A → title, 1/1000 → rare2), the 6 AM win → next-day → the **load glitch screen** (cutscene+1, 5 s) → the **retro cutscenes frame** (the 5x5 pizzeria room grid: 250 ms / 30 px steps, the per-room barrier table, Shadow Freddy figures leading the way, rain/rat/scanline ambience, and the Purple Guy who hunts you down and takes the scene's animatronic apart — scene N after night N; scene 5 opens the back room where he hides in the Springtrap suit and dies) → the **end chooser** (cutscene != 5 → the next night card; == 5 → bad end / **good end by the Marion trigger**). The **six Atari minigames** (frames 19-24) on one platformer engine (100 ms walk/fall ticks, 60 ms rise, jump counters 7/9/10, feel-sensors vs the rect kit, balloon bounce floors, the 1024x768 viewport over the 3072x2304 world): **BB's Air Adventure** (the CAM 08 toy; the balloon pickup unlocks balloons everywhere), **Mangle's Quest** (the arcade cabinet, night 2; the cake pickup + four follower kids), **Chica's Party** (collect → feed four guests), **Stage01** (the keypad; zip pads), the **Glitch Minigame** (the night-5 dark room; the S-view teleports + the 1-in-2 glitch re-roll), **Happiest Day** (the CAM 03 puppet toy; all four kids fed → the goodend trigger → the float finale); the **extras menu** (animatronics viewer, minigame replays gated by goodend, the jumpscare player gated by beat6, the four cheats fast/ventproof/hyper/nocams that the office obeys). The **freddy3 save** carries the full dump key set (level/cine/beat6/4thstar/goodend/bb/cake/k1..k4/cheats). Console: X on the right cam starts a secret. Labeled deviations: pixel-perfect backdrop collision → the rect kit (coarse per piece), the arcade/cupcake/keypad cam picks are approximations (the BB/puppet cams are dump-pinned), a few minigame item positions (marked ~) come from the room kit |
| **FNAF 4** | **100 % (code, no game flow)** | disclaimer, title (red sky + heading + menu with the tint selector), and the full night loop (FNaF4Game): five positions with the walk darks, the four threat lanes (Bonnie/Chica doors + breathing listen + shut-door visits + linger bedroom attacks; Foxy's closet with the forced flashlight and the bite; Freddy's bed counter with minis), Fredbear from night 5, the paranoia black-flash pipeline, 60 s hours with the per-night AI tables. **v2.64 — the whole dump flow, slot-resolved** (the same storyboard-slot discovery as FNAF3): the real **what night** card (INI night, the shadow pick forces nights 7/8, ambience + the 2.1 s clock beat), the real **night win** (the "6 AM" digits settle 2/2.5/3/3.5 s with the flicker, the fn4 writes, the 10 s route: night ≤ 5 → Cutscenes, 6 → the ending, 7 → the **lockbox**, ≥ 7 challenge/8 → title), death → **game over** (7 s → title), **Fun with Plushtrap** (the 9-position hall graph: in chair → stage 1 → far left/right → stage 2 → close left/right → stage 3 → got you; the A-hold flash; the darkness accumulator drives the 2 s move rolls; flash at stage 1 = he jumps back to the chair; flash at stage 3 = the win; the per-night clocks 90/60/45/30 s; the view anims are the dump's own tables incl. the 21-cell jumpscare), **Fun with Balloon Boy** (the same engine: 45 s clock, the bb1b-3b voice roll, the 500 ms stage-3 extra move, exits to the extras), the **lockbox** (after night 7; A = the unlock), the **extras** (10 rows: animatronics/making-of/jumpscares viewers, the Plushtrap and BB replays ("minigame play"), the **shadow nights 7/8**, the cheat toggles (house map/fast nights/radar/blind mode/insta foxy/mad freddy/all nightmare — the fast-nights cheat shortens the hours), exit), the **house cutscenes** (the walkable 5120x3840 world with the boy follower and the screen-follow pages; scene = the night, 0 → the card, 1-4 → the Plushtrap intro, > 4 → title), the **ending** typewriter hold, the test/demo skip screens. The **fn4 save** carries the dump key set (night/scene/beat5..beat8/s1..s6/test + the cheats). Labeled stop-gaps: the cutscene/ending dialogue letters are sprite images whose strings did not survive the dump (the talk box renders without invented text), the minigame timer/lockbox/captions ride debug font, the cutscene scene scripts run as a timed walk (the dump's 202 per-scene trigger groups simplified) |
| **Sister Location** | **~25 %** | the streaming loader streams the 1.5 GB pak room-to-room; **v2.65 wave 1 live**: the dump flow plays on the slot semantics (Warning/Legal splash → title → the Elevator ride → the Circus Control hub per night ⇒ vent hops → the walk rooms) with the real W-hold movement model (the crawl latch, the walk/quick walk speeds, the step sounds), the Ballora Gallery dance (walk = noise: distance builds, walking > 5 s puts her in the approach, flash side pan of her music, kill over 600), the Funtime Auditorium flash beacon (2 s refill, attracts Foxy, silhouettes by distance bands), the vent crawl 100-tick notches × 10, Baby's Room night end (Jingle_4b + INI write + tv show + Girl Voice chain), the per-night Script Event presets and the death-resume presets (n2=81/n3=111/n4=57/n5=512), the fnaf_sl save ("sl": current/intro/beat1/beat3/keycard/endsceneno/104). Left for wave 2: the HandUnit scripted beats per room, the P&S face-button task, the breaker task, Under Desk, the scooping chain, the extras/custom night (frame 31), the 8-bit minigame, the Perspective-eased room tween curve port, the real room art pins |

One boot shows a **game selector** when more than one bundle is present.
Every game ships its own `.spa` (Xbox identity/achievements, not yet) and its own
`*.pak` (textures + sounds, built by our tool from that game's own dump).

Русская версия: [README.ru.md](README.ru.md).

> [!NOTE]
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
  force-close), XContent saves/achievements (not work on real hardware, for now), loose-save import; sounds and
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

## Asset pipeline (CTFAK-CPP) (Temporarily available)

Tool: **[ctfak-cpp](https://github.com/Sergey004/ctfak-cpp)** — a clean-room
C++ reimplementation of a Clickteam Fusion runtime dumper/repacker. The full
cycle for any game in the series:

```bash
# 1) dump assets (images/sounds/packed data):
./build/ctfak-cpp  <game>.exe 
Type "1" for dump assets 
# 2) GPU-ready pak + asset table (DXT textures + PCM, big-endian 'FNAF'):
./build/ctfak-cpp  <game>.exe 
Type "10" for pack assets 
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
not distribute the assets or built images. (In other words, Build it yourself)
