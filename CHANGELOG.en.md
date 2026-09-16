# Changelog — FNAF1 Recomp (Xbox 360)

Notes on what is done and what is left. Versions match the code
comment tags (`v2.8`, `v2.14`, …, `v2.32`) and the historical notes.


---

# SESSION SUMMARY — 2026-09-12/13: the project became a MULTI-GAME shell

This session turned the FNAF1 recomp into a foundation hosting the whole classic series. Everything below was tested on a REAL Xbox 360 (RGH), not just Xenia.

**Dumps & tooling (CTFAK-CPP)**
- Full dumps taken for FNAF2/3/4 and Sister Location (assets + event listings + structure JSON) — the authority source for everything below.
- Restored the lost **Recomp Pack** tool (the .pak repacker existed in the source but was never registered/CMake-listed); generalized per-game pak naming (fnaf2/fnaf3/fnaf4/sisterlocation.pak); built GPU-ready paks for all games; per-game `asset_mapping_<id>.hpp` tables (namespaced, in `include/assets/`).
- Verified the tool on Fusion builds 288 AND 286 (SL), up to a 1.5 GB pak.

**Architecture**
- **Core + per-game modules**: `AppModule` contract (Name/PakName/Load/Tick/Render/WantsExit/PrefersStreaming), `AppServices{audio,pak,batch,text,input}`, module registry, soft pak scan at boot.
- **Renderers split per game**: GameRender = FNAF1 only; FNaF2/FNaF3/FNaF4Render — stateless, fed game state + clocks. Full-stretch 16:9 mapping (X 1.25 / Y 0.9375) like the stretched PC windows.
- **Streaming pak loader** (`LoadStreaming` + `PreloadAsync` worker thread) — built for SL's 1.5 GB, proven necessary on real HW by FNAF4's OOM crash.
- **Boot selector** with a soft pak scan; B in module mode returns to it (no more console kill).

**FNAF2 — playable skeleton**: per-game disclaimer → title (the dump's "video FROM Freddy's eyes" bg roll, static X-jitter, Sand Temple drone) → night card → panning dark office (flashlight LB-hold per groups 35/36, battery 7000, BB-steal flag) → camera monitor (12 feeds pinned from groups 100-123, map panel + buttons) → 70 s/hour clock → 6 AM.

**FNAF3 / FNAF4 — test levels**: titles and offices from their dumps (FNAF3: Springtrap scene + static bursts + titlemusic; FNAF4: red-sky title + 1300-wide bedroom, streaming loader, its title theme).

**FNAF1 fixes along the way**: Freddy's 4B entry rule per dump groups 394/395; Golden Freddy kill screen rebuilt from f14; office hallucination per groups 413-419; night-number HUD to (1237,89).

**Roadmap**: FNAF2 → FNAF3 → FNAF4 to FNAF1-level playability; Sister Location last (streaming monster, startup flow already pinned: Warning → Elevator/HandUnit voice → title).

---

## v2.35 — message boxes: software fallback removed (the box was fine)

- Real-HW log proved the v2.34 watchdog was wrong: `XMB: ret=0x3E5` (= ERROR_IO_PENDING — the box opens), then a ~10 s delay until `Hooked: 'HUD: XuiSceneCreate'` — that is XAM **cold-starting its XUI on the FIRST call**; the window itself appears fine afterwards.
- Reverted to the plain blocking wait (no watchdog, no `SoftPrompt`): `SysPrompt` keeps the return-code logging (`ret`/`res`/button) and every box routes through it (import question/done, missing pak, fatal). The one black frame presented before the first boot box stays (harmless).

The Xenia caveat is accepted: Xenia does not render the XMB; if emulator testing needs prompts later, a fallback can return behind a build flag.

- **Start+B DEV combo made pad-robust**: the pad reports the two edges a frame or two apart, so a lone Start fell into the pause handler and booted the game back to the title. A ~330 ms window after a B edge now accepts the Start edge (same-frame still works).
- **Clean exits to the dashboard**: `exit(0)` tears the XDK process down abruptly (kernel threads dying with code 0). `ExitToDashboard()` = `XLaunchNewImage(NULL, NULL)` now ends the missing-pak screen and the import self-close; **Golden Freddy intentionally keeps exit(0)** (it mirrors the original's abrupt close).
- **Golden Freddy flow restored (DEV item 4 now runs the REAL pipeline)**: the old DEV test drew the CAM 2B pose over the camera view ("just a camera frame"). It now spawns the actual sequence — giggle, he appears IN THE OFFICE (img 573), **IT'S ME flashes accompany the visit** (~0.2 s every ~1.1 s, the wiki's hallucination phase), the full-screen face (f14) and the intentional close. READMEs document the "crash" as a safe feature.
---

## v2.34 — message boxes: watchdog + software fallback (the "sound, no window" fix)

- **Root cause (real-HW import test)**: `XShowMessageBoxUI` played the system sound but never drew the box — it is called at boot before the first presented frame, and the docs only guarantee it fails loudly with `ERROR_ACCESS_DENIED` when another system UI already owns the screen; a silently-non-rendering/pending box leaves the old `while(!XHasOverlappedIoCompleted) Sleep(16)` hanging forever.
- **`SysPrompt` wrapper**: one place calls `XShowMessageBoxUI` (non-NULL `XOVERLAPPED`, per the docs), logs `ret`/`res`/button to printf + the debug console, waits with a **10 s watchdog** instead of forever.
- **`SoftPrompt` fallback**: a software-drawn prompt (title/text/hint, A = Yes, B = No, 30 s timeout) using our text+input stack — works on Xenia (no XMB) and on real HW when the system box failed to show.
- **Every message box routes through it**: import question (No/Yes), import-done (OK), the missing-`fnaf1.pak` error, and `ShowFatalError` (which falls back only when the D3D stack is up). Boot presents **one black frame before the first box** so XAM has a frame to overlay.
- Behavior is 1:1 when the system box works (real HW); the fallback only engages on actual failure. Version → v2.34.

---

## v2.33 — FNAF2: the Phone Guy call

- **The night phone call is live**: ~2 s into the office the game plays the dump's record once — `snd_call 1b`…`6b` by night (night 6 = the garbled call). Implemented via the module's audio (the game class stays engine-free); no mute (FNAF2 has none); re-arms on the next office entry. Version bumped to v2.33.

---

## v2.32 — FNAF3 + FNAF4 test renders (title + office)

Both games now render their screens in module mode (arg "fnaf3"/"fnaf4"), same pattern as FNAF2: per-game renderer + thin module:

- **FNaF3Render**: title (bg img_862; the FULL-STATIC title per the dump — the composite is covered by the static cycle [37,620,33,34,35,36]@99, the menu flashes through ~every 2.5 s as a labelled approximation until the flicker groups are pinned; menu items 592/301/625/826 + selector 833 at placement-table coords); office (2000×768 room img_203, pan 0..976, office static on top). Minigame rooms (BB, Mangle, Toy Chica, GFreddy, RWQFSFASXC, Marion) — later stages.
- **FNaF4Render**: title (red-sky Backdrop img_626, the heading image img_658 @ (589,-2), menu images 730/737/738/731 at placement coords, footer texts); the bedroom (1300×768 img_4, pan 0..276). Stars/DEMO hidden; the FNAF4 logic (left door / right closet / bed) — later stages.
- Roadmap fixed: FNAF2 → FNAF3 → FNAF4 to playable level; Sister Location last (the 1.5 GB streaming monster).
- **Full-stretch 16:9 like the stretched PC windows**: all three module renderers map the 1024-wide frames with a separate X scale (1.25) and Y scale (0.9375) — no pillarbox bars; camera feeds stretch to fill; FNAF1 keeps its own panorama.
- **FNAF2 title "video FROM Freddy's eyes" pinned to the dump** (groups 3,4,8-11): the game OPENS on the eye view (img_362); every 2 s a Random(50) re-roll maps 0/1/2 to the eye views (362/470/215) and the rest to the normal bg (321); the static layer jitters its X every ~1.8 s. Title audio corrected per dump: static2 (vol 50) + The_Sand_Temple_Loop_G drone (vol 100) — "In The Depths" was wrong.
- **FNAF3 reworked after the console test**: the scene is ALWAYS visible (Springtrap bg + menu), the static appears only as short glitch bursts (the dump has a burst timer on the static object); the invented static_sound audio layer removed (the dump plays only titlemusic) and the music volume halved — "очень громкий звук" fixed.
- **Per-game disclaimers**: every game now opens with ITS warning screen (frame 0 "Frame 17" per dump — same text, FNAF4's in red), ~3.5 s or any key; the title music starts at the TITLE, not during the warning.
- **Boot selector is back (v2.32)**: with more than one bundle present, boot shows a text list (D-pad select, A launch, B = default FNAF1); one pak boots straight. The argv override still wins for Xenia/debug runs.
- **Module ambience is live**: FNAF2 title plays static2 + "In The Depths" (the menu song), FNAF3 title plays titlemusic + static_sound, FNAF4 plays its base ambience over title AND bedroom; stops/starts follow the screens. Office/room loops come with each game's audio stage.

---

## v2.31 — FNAF2 game skeleton: night loop, flashlight, cameras

The FNAF2 state now lives in `FNaF2Game` (`include/FNaF2Game.h`) with counters named exactly as the dump's ("viewing", "lit?", "night", "battery life", "time of the night", "in danger", "mask", BB-steal flag), and `FNaF2Module` renders from it:

- **Night loop**: title (selector New Game/Custom via D-pad, A starts) → night card → office → 6 AM → title. Custom Night shows the selector but is locked (no progression yet).
- **Clock (dump-pinned)**: the "AM" counter hits 70 → hour change (group 484/485): 12→1, then +1; 70 s per hour, night 12AM→6AM ≈ 7 min; hour 6 = win screen.
- **Flashlight (group 35/36)**: HOLD (LB on pad / Ctrl on PC), blocked by battery ≤ 0, mask, monitor up, "in danger", and by Balloon Boy having stolen it. Battery 7000 at night start, −1 per lit frame (group 170). The office renders dark (world tinted to ~28%) with the flashlight bringing full brightness; the ceiling strips (507) and LIGHT buttons stay lit.
- **Camera monitor**: A raises/lowers (viewing 0↔1), D-pad cycles cams 1..12. The feed table is pinned from the office map-button groups (100–123): CAM01 [174], 02 [80], 03 [83], 04 [43], 05 [38], 06 [32], 07 [51], 08 [37], 09 [117], 10 [41], 11 [76] (Prize Corner), 12 [50] — the EMPTY variants; animatronic-presence frames come with the AI stage. A 0.12 s static burst plays on each switch; a text cam strip stands in for the real map buttons.
- Fixed the FNAF1 night-number HUD to the dump position — the counter sits at (1237,89), the digits were drawn at (1217,74) (and at the screen edge in the v2.7.13 build seen in the user's recording).
- Known approximations: night-card/6AM durations (2.5/5 s, pin from the frames), the timed title-glitch, 1600-wide feeds squeezed into the window (needs sub-rect crop in the batch).
- **Renderers are now per-game**: all FNAF2 drawing moved out of the module into `FNaF2Render` (`include/FNaF2Render.h` + `src/FNaF2Render.cpp`) — a stateless-per-frame renderer fed `const FNaF2Game&` + clocks. GameRender stays FNAF1-only; both sit on the same core trio (SpriteBatch/PakLoader/TextRenderer via AppServices). New files: FNaF2Game.h/.cpp, FNaF2Render.h/.cpp.

---

## v2.30 — FNAF2 title screen renders (module mode)

- **FNaF2Module draws the real FNAF2 title** from the dump (frame 1 "title", 1024×768): background img_321, the six-frame static cycle [332,334,328,329,330,331] at speed 99 = 59.4 FPS, the "Freddy glitch" frames (anims 12/13/14 = imgs 65/73/210), logo img_469, menu (new game 301 / continue 303 / selector 229 on New Game / night word 270 + counter digit / custom night 438 — VisibleAtStart=true per the dump). Deliberately hidden per the dump: demo, stars, 6th night (no FNAF2 save system yet). Frame coordinates map to the 720p screen as a pillarbox (scale 0.9375, 160 px bars), z-order follows the frame's instance order (static UNDER the logo/menu).
- **DEVIATION (labelled)**: the glitch trigger group was not located in the title events yet — a timed approximation (~every 7 s, one glitch frame for ~0.25 s) runs until the event is pinned.
- **Module mode in the core loop**: when the active module is not FNAF1, the loop owns the frame to the module (Tick + Render, no FNAF1 state machine/fades); B exits. A command-line arg naming a module ("fnaf2", …) overrides the pak-scan choice (Xenia/debugger args; per-game XEX builds will pick their module by construction).
- **Verified against the tool's Frame Layout composite** (frame_1_title.png + placement table): all menu/logo positions now use the hotspot-corrected left/top (logo (96,39), new game (86,437), continue (86,507), selector (33,512) — frame start has it on CONTINUE, night word (97,549), custom night (89,650)); the four TEXT objects (heading "Five Nights at Freddy's 2" word-per-line top-left, "v 1.033", "Press and hold delete to reset all data.", "©2014 Scott Cawthon") are drawn with the debug font at their composite positions (raw instance coords are junk; consolas-based glyphs later). The "Demo" watermark in the composite is a frame-renderer artifact — the events keep it hidden (DEMO? pinned 0).
- **FNAF2 office renders with panning** (frame 3 "Frame 1", 1600×768): A on the title switches to the office; the left stick pans the 1024-wide window across the 1600-wide room (0..576 world px, ~480 px/s). Drawn from the placement table: bg img_92, the "lights" strips (507), the wall piece (218), both wall LIGHT buttons (90/98), the desk row — table fan (293), plushies (601/604/603/606), mic (612), BB (610), toy bonnie (608), cupcake (555), golden fred (611) — with the viewport-space static over everything. Not drawn: all event-spawned objects (puppet, under-table Freddy, JJ, the "in office" poses, yellowbear, RWQFSFASXC), the mask/flip/camera overlays and the Halloween pumpkin.
- **AppServices gains `input`** (per-frame pad snapshot refreshed by the core) and the core now actually hands services to the active module (`module->Load`) on entering module mode — until then the stubs ran with empty services.
- Architecture decision recorded: separate XEX per game + shared static core.lib, each XEX shipping its own .spa (SPAFILE/XDBF with its own XACH achievements, via spaassembler); the in-binary selector is dropped (launcher.xex via XLaunchNewImage is optional later).

---

## v2.29 — streaming pak loader (built for Sister Location)

- **`PakLoader::LoadStreaming()`** — "sliding over the file": only the pak header + tables + name pool stay resident (a few dozen KB); each asset's blob is read at its table offset on first use. Textures upload to D3D on the first `FindTexture` and the staging buffer is freed (UMA: it then lives in the shared 512 MB pool); sounds are read + normalized per-sound on the first `FindSound` (RIFF peel / 8-bit expand / byte order — with a 4 KB **mini-vote at load time** replacing the whole-bank byte-order vote, and passthrough blobs like SL's mp3s correctly marked unplayable instead of submitted as garbage PCM).
- **`PreloadAsync(names, count)`** — a background worker thread (XDK: `CreateThread` + event + critical section; PC falls back to synchronous) runs the same ensure-ahead path, so room-to-room transitions can pull the next room's textures while the current frame renders. A `FindTexture` miss still loads synchronously, so the render can never see a half-loaded texture.
- **Scope (per decision): SL only.** FNAF1/2/3/4 stay on the eager `Load()` — their paks fit the 512 MB UMA pool comfortably; the eager path was refactored (`UploadTexture` extracted) but behaves identically. The SL module (stage 7) will boot through `LoadStreaming` and drive `PreloadAsync` from its room-to-room frame transitions. Dormant until then — FNAF1 boots exactly as before.
- **Soft pak scan at boot (`AppRegistry_ScanPaks`)**: probes every module's bundle (`game:\<PakName>`, fopen probe — reads only), prints a per-pak OK/not-found line to the log and the on-screen console, and parks the active module on one whose pak exists — FNAF1 has priority, then the first found. Missing paks never fail the boot: with only `fnaf1.pak` present the game starts FNAF1 exactly as before, and with nothing at all the existing missing-pak message box reports as usual. The pak is then loaded from the active module's `PakName()` instead of the hardcoded name.
- Rationale per the XDK docs (see docs/ARCHITECTURE.md): the games run from **HDD**, so `XFileCache` / `ReadFileScatter` / physical sort keys (all DVD-only) don't apply; a plain seek+read over an open file is the correct primitive.

---

## v2.28 — app foundation for FNAF 2 / FNAF 3 (core + modules)

- **The app is split into CORE + MODULES** (docs/ARCHITECTURE.md): the Xbox shell (D3D9 loop, SpriteBatch, XAudio2 mixer, XInput, fades, XContent wrappers) stays in main.cpp and the shared systems; each game becomes an `AppModule` (Name/PakName/Load/Unload/Tick/Render/WantsExit) talking to the core through `AppServices` (audio/pak/batch/text). The core never includes game headers; a module never owns the device.
- **Module registry** (`AppRegistry`): FNaF1Module (active — the game still runs directly from main.cpp at stage 1; `RequestExit()` prepared to replace the raw `exit(0)`), plus FNaF2Module / FNaF3Module placeholders that report their bundles (`fnaf2.pak` / `fnaf3.pak`) and render stub screens. Boot banner prints the active module.
- **FNAF2 and FNAF3 dumps taken with our CTFAK-CPP** — both exes read clean, the tool passed its first third-party-runtime test:
  - FNAF2: 27 frames (office 1600×768 with **751 groups**, "dream" 2500×768, "8bit" minigame hub, error/error 2, rare1 ×3, customize 80), 804 images, 66 wav, 452 objects.
  - FNAF3: 26 frames (office **2000×768** with 773 groups, cutscenes 190 groups, six 3072×2304 Atari minigame frames: BB/Mangle/Toy Chica/GFreddy/RWQFSFASXC/Marion, bad/good end), 1066 images, 70 wav, 577 objects.
  - Dumps live in `ctfak-cpp/build/Dumps/…`; per the dump-only-authority rule, stage 6/7 of the migration plan starts from these, not from any wiki.

---

## v2.27 — dump-verified Freddy 4B rule & Golden Freddy creepy screen

- **Freddy can't slip in from CAM 4B while you watch him** (groups 394/395 read from the dump): the 4B decision now requires the monitor up **and** `viewing != 42` (CAM 4B). The door-closed retreat to 4A additionally requires `viewing != 4` (CAM 4A) — so with the door closed, watching either east-hall camera parks him at the corner. (The wiki mentions only CAM 4B; the dump splits the rule across the two groups.)
- **Golden Freddy's kill screen rebuilt from the dump**: f14 "creepy start" is a full-screen dark face (img_545) with two img_547 twinkles at (510,192)/(804,196) — NOT the CAM 2B pose (img_571) we drew before. **His kill plays XSCREAM2 — a deliberate deviation from the dump** (the dump itself only uses XSCREAM2 on f15 "creepy end", the unreachable demo ending; its f14 is silent).
- **No screen shake for Bonnie/Chica**: the wiki claims the background shakes during their scares; the original has none — the jitter is the scare animation's own frames (user-verified against the original; the v2.18 verdict stands).
- **Office hallucination rebuilt to dump spec (groups 413-419)**: every ~20 s `Random(1000)==1` opens a 100-tick (~1.7 s) window; during it the IT'S ME overlay shows only on **~1-in-10 frames** (`Random(10)==1` per frame — the dump's rapid on/off chatter, not a continuous overlay), and **robotvoice (ch21) goes full volume** for the window (was: a whisper one-shot + continuous 1.5 s overlay). The overlay is drawn translucent (alpha 155/255, the static's coefficient-100 style — the dump does not export sprite-level ink). Per the dump this system is NOT tied to Golden Freddy — his sitting phase is silent (groups 420-425 play nothing), so the earlier golden-specific flicker/garble was removed.
- The v2.26 force-close stays for now, with a caveat: the v1.132 dump contains no force-close. f14 sits silently (~200 s failsafe), then lands on f5 "freddy" (XSCREAM + static + the Freddy flicker scare) and returns to the title. Say the word and we'll switch to that exact flow.

---

## v2.26 — scare looping, Golden Freddy force-close, Halloween sprite fix

- **Jumpscares loop like the original**: the scare frame cycles (Freddy 31, Foxy 25, Bonnie 11, Chica 16) now repeat for the whole scare instead of freezing on the last frame.
- **Golden Freddy force-closes the game**: per the original — instead of the normal Game Over screen, the game closes after his full-screen scare (~2.5 s), matching the wiki ("instead of being taken to the normal Game Over screen, the game will forcibly close"). Only avoidable by raising the Monitor in time.
- **Removed the mistaken office "lights" draw**: the img_608 string-lights image is the Halloween event decoration, not a door-light glow — it no longer appears when a door light is turned on.

---

## v2.24 — animation speeds pinned to the PC dump

- Confirmed the animation formula `fps = speed × 0.6` (÷60) against the original PC `application.json` (`frameRate: 60`), cross-checked with FNaF64 (which uses the same `speed_fps`). Pinned the speeds that were guessing instead of reading the dump:
  - **Static (noise)**: 24 FPS → speed 99 = 59.4 FPS.
  - **Desk fan** ("Active 6" anim 0, 3 frames): ~8 FPS → speed 99 = 59.4 FPS.
  - **REC light** ("Active 2" anim 0, `[7,5]`) and **cam-map blink** ("Active 9" anim 0, `[164,145]`): ~1.25 Hz → `speed 2` = 1.2 FPS via `CfAnimFrame`.
  - **Foxy sprint** ("Active 3" anim 51, 33 frames): linear stretch → speed 65 = 39 FPS (play once, hold last frame).
- Re-verified (already correct, no change): jumpscares (Freddy 65@50×31, Foxy 52@50×25, Bonnie 35@75×11, Chica 44@99×16), doors (50→30 FPS, 16 frames), IT'S ME (75→45 FPS, 4 frames).

---

## v2.23 — original lives/save system fix

- Removed the non-1:1 "lives" system (the original FNAF1 has none). A jumpscare no longer decrements lives or resets progress; death just returns to the title and Continue retries the same (unlocked) night.
- Fixed a progression bug: the night-5/6/7 stars (`beat5/beat6/beat7`) were also awarded on GAME OVER — dying on night 5/6/7 incorrectly unlocked the star. Now they are gated on `GAME_STATE_NIGHT_COMPLETE`.
- Save storage:
  - The save file is the original extension-less **`freddy`** (INI content) in both builds. The Live Safe build now reads/writes it at **`game:\save\freddy`** (absolute, next to the .xex) instead of the relative `save\` folder; the system build keeps the XContent content `freddy`.
  - `lives` removed from the `[freddy]` INI and from `GameProgress`.
- **Save import**: at boot the system build checks `game:\freddy` / `game:\save\freddy` for a valid loose `[freddy]` INI; when found it asks via an Xbox message box ("Do you want to import the save found in the game folder?"); on confirm it imports into the XContent save (`fnaf_save:\freddy`), shows "Import complete. Please restart the game.", then closes the game. No-op in Live Safe.

---

## v2.22 — Golden Freddy, camera audio, doors & remaining audio

**Doors + animation timer**
- Restored the open/close slide in normal (non-analog) mode. `DoorSystem::ToggleDoor`/`SetDoor` flip the logical `m_doorClosed` instantly (the AI reads it), while the visual position `m_doorAmount` is driven by a new `DoorSystem::Tick(dt)`, ticked from the main loop via `Game::TickDoors(1/60)`. Slide = 16 frames @ 30 FPS (~0.53 s), as in the original.
- Extracted the Clickteam animation timer into `include/CfAnimTimer.h` (`CfFramePeriod` / `CfAnimFrame` / `struct CfAnimTimer`), formula `fps = speed × 0.6` (`period = (100/speed)/60`). Static, wipe, flash, raise and the jumpscares now use it — `GameRender.cpp` no longer duplicates the pattern.

**Camera audio (1:1)**
- `onCameraChange(cam, reason)` now receives a `CAM_REASON_*` reason:
  - monitor up (`UP`) → `CAMERA_VIDEO_LOA` whir (`Snd::CAMERA_SWITCH`) + start `static`;
  - camera switch (`SWITCH`) → `blip3` (`Snd::BLIP`), static is not restarted
    (previously the whir played on EVERY switch — the "extra sound");
  - monitor down (`DOWN`) → `put down` (`Snd::PUT_DOWN`) + stop `static`.
- Removed the forced `onCameraChange(CAM_OFF)` from power-out (it would play "put down" on every blackout; `OnPowerOut()` already stops the static).

**Golden Freddy ("yellow bear")**
- Full sequence in `main.cpp` (`TickGoldenFreddy`): 1/100 poster roll on monitor drop → giggle #38 (`Laugh_Giggle_Girl_1` = `Snd::FREDDY_LAUGH_LONG`) on CAM 2B → appears in the office (img 573 @660,478) on monitor drop → ~5 s to raise the monitor, or he kills you.
- The kill reuses the jumpscare state with `ANIM_COUNT` as sentinel (`Game::DebugTriggerGoldenFreddy`): `RenderJumpscare` draws img 571 full-screen, `GetJumpscareDurationSec` gives ~2.5 s, `OnJumpscare` guards the name array.

**Remaining audio triggers**
- Kitchen oven `OVEN_DRAW` (ch10) — Chica in the kitchen, every ~80 s (groups 245-250).
- Camcorder `TAPE_EJECT` (ch6) on monitor-up (group 144).
- Power-out: `AMBIENCE2` (drone, group 271/286) + "Active 2" `COMPUTER_DIG` and `garble1/2/3`, cycling with Freddy's face flashes (`Game::GetPowerOutFaceState`).
- Freddy's nose honk `PARTY_FAVOR` — Y in the office (group 349).

**Remaining approximations**
- `robotvoice` (ch21) / `EerieAmbience` (ch18) still use a room-based heuristic in `TickAudioMixer`, not the exact `vol zone` (objInfo 136) overlap from the dump.
- The "creepy start/end" frames are shortened to ~2.5 s (the original "creepy start" lingers ~200 s before crashing).

---

## v2.21 — analog doors (DEV test feature)

- `DoorSystem::SetDoorAmount/GetDoorAmount` — continuous door position 0..1, logical "closed" (AI) = `amount >= 0.5`. DEV toggle `g_devAnalogDoor`; the original toggle logic is untouched. Also the DEV "lights as hold-button" mode.

---

## v2.20 — saves & achievements: two builds (system / local)

- **Inverted `FNAF_LIVE_SAFE` semantics** (it controls both `Progress.cpp` and `Achievements.cpp`):
  - **default (no macro) = "regular"** — touches the Xbox system: save via XContent (`XShowDeviceSelectorUI` + `XContentCreateEx`), achievements via `XUserWriteAchievements`, Y opens the **system** list (`XShowAchievementsUI`).
  - **`FNAF_LIVE_SAFE` = "Live Safe"** — does NOT touch the system: saves/achievements to a local `save\fnaf_save.ini` / `save\fnaf_ach.ini` next to the .xex, no XUserWriteAchievements/XShowAchievementsUI/XContent.
- **Storage in `Progress.cpp`** moved behind `StorageOpen`/`StorageClose` helpers (the XContent branch and the local `save\` branch with `_mkdir("save")`); `Load`/`Save`/`LoadAchieve`/`SaveAchieve` are shared by both.
- **Achievements**: added `Achievements::ShowSystemUI()` (system build → `XShowAchievementsUI(0)`; Live Safe → false). In `main.cpp`, Y on the title: `if (gi.yToggle && !g_ach.ShowSystemUI()) g_achScreen = true;`
- The in-game toast and achievement screen remain as a fallback in both builds.

---

## v2.19 — dB mixer + jumpscare fixes

**Audio: dB layer (Clickteam/DirectSound → XAudio2)**
- `AudioSystem` now stores channel volume in **dB**, not linear 0..1 amplitude (`m_channelVolumeDb[32]`, `SetChannelVolume(dB)`). `PlayOnChannel` plays at the channel's current level; `Play(...)` stays a linear-amplitude one-shot.
- Helpers in `AudioSystem.h`: `DbToAmplitude(dB)` (10^(dB/20)), `CFVolumeToDb(0..100)` (20·log10(v/100)), `AmplitudeToDb(...)`. Uses C89 `pow/log10`, no C99 `powf/log10f` (VS2010).
- All `SetChannelVolume` calls moved to dB via `CFVolumeToDb(orig 0..100)` — values can be changed 1:1 against the original events.

**Random / dynamic events**
- **Freddy's "got in" laugh** now **random** from #56/#57/#58 (random(1,3)) + `running fast3`, volume by distance (bathrooms 20/35, kitchen 30/40, E-hall 40/60, corner 60/75, right door 80/100) — groups 390-405.
- **Pirate song2** (ch13): every 80 s 1/30 while Foxy is in the cove; 15 while watching CAM 1C, 5 otherwise (groups 269/274/275).
- **Breaths** (ch14, #22-25): every 100 s 1/3 when Bonnie/Chica are at the door and the camera is up (groups 276-283).
- **Circus** (ch15, #29): every 100 s 1/30 (group 270).
- **Music box** (ch22, #30): when Freddy enters the kitchen (groups 399/400).
- **Phone call without the 2.5 s delay** — immediately on entering the office (groups 361-365), nights 6/7 silent.

**Jumpscare fixes**
- Removed the fake screen shake from `RenderJumpscare`/`RenderScareFlash` (the original scares are static; motion comes from the animation itself).
- Removed the IT'S ME overlay from the kill animations (it was baked into `RenderJumpscare` for everyone); IT'S ME now only through its own rare hallucination (`RenderItsmeFlash` / the DEV "IT'S ME" item).
- **Starts on frame 0**: `scareElapsed` increments AFTER the first frame, so Chica (60 FPS) no longer skips frame 0.

---

## v2.18 — random events (IT'S ME + Golden Freddy)

- **Footsteps by distance** (`OnAnimatronicMove`, group 198-244): Bonnie/Chica — `DEEP_STEPS` at 0.15 (far) / 0.30 (halls) / 0.40 (corners and doors), matching the original 10/30/40.
- **Freddy's laugh fixed**: movement now plays the `_1d/_2d/_8d` (#56/57/58) family in rotation, not `Laugh_Giggle_Girl_1` (#38 = Golden Freddy).
- **IT'S ME hallucination** — a rare flash: every ~20 s a 1/1000 roll (group 419); on success `WHISPERING` + a full-screen 4-frame cycle (`RenderItsmeFlash`, 12 Hz, Active 21 anim 0). Previously DEV-only.
- **Golden Freddy poster** (group 41/42/348): on monitor drop `random for pic` = random(1,100); when CAM 2B is empty and the roll < 2 — the "LET'S PARTY!" poster shows Golden Freddy's face (anim 75, handle 571). `GameRender::m_goldenRoll` (constructor -1, rolled in `RenderOffice`, read in `RenderCamera`). Bonnie/Chica in the frame take priority.

---

## v2.17 — DEV/debug menu

- **`Start + B`** opens the DEV menu from any state (B/Y closes):
  - **God mode** (A) — no power drain, no attacks (`Game::SetDebugGodMode`).
  - **Jump to night N** — instant start of any night 1..7.
  - **Force 6 AM** (`Game::DebugForceNightComplete`).
  - **Force power out** (`Game::DebugTriggerPowerOut`).
  - **Trigger jumpscare** — Freddy/Bonnie/Chica/Foxy + **Golden Freddy** (the distinct `Laugh_Giggle_Girl_1` giggle + the full-screen 571 flash).
  - **Sound test** — 22 sounds (`DEV_SOUNDS` in main.cpp).
  - **Unlock / Reset achievements** (`Achievements::UnlockAll/ClearAll`) + **Console ON/OFF** (the on-screen log; messages are always duplicated to the VS output via `printf`).
  - List of all 17 frames (storyboard).
- The DEV menu is also a **hidden title-menu entry** (press Up on the title → hidden position, A opens; `MENU_OPT_DEV` with no text/arrow).
- **Freddy "endo-head" flicker** (img_442) added to the title (`IMG_MENU_FLICK3`).
- New methods: `Game::SetDebugGodMode/DebugForceNightComplete/DebugTriggerPowerOut/DebugTriggerJumpscare`, `Achievements::UnlockAll/ClearAll`, `GameRender::RenderDevMenu`.

---

## v2.16 — audio mixer (1:1)

- **Channel volume** in `AudioSystem`: `PlayOnChannel` / `SetChannelVolume` (re-applied to a live voice), 32 channels — matching the original `Speaker` channels.
- **Office on channels** (`frame_3` group 15): `BuzzFan` (ch1), `ColdPresc B` (ch2), `BallastHum` (ch3), plus the proximity loops `robotvoice` (ch21, muted) and `EerieAmbience` (ch18, muted).
- **`TickAudioMixer`** every frame in the office: fan 25/10 by camera, hum muted by camera/light, phone 100/50/0 (viewing/mute), proximity ambience (robotvoice/EerieAmbience rise as animatronics approach the doors/office).

---

## v2.15 — transitions/fades + sound

**Transitions and effects**
- **Frame fades** — a `ScreenFade` transition machine in `main.cpp`: black overlay with alpha, 1:1 durations from `docs/FRAME_TRANSITIONS.md` (disclaimer 1010/1010, "what day" 1010 out, "next day" 1010 in/900 out, gameover 1010 in, newspaper 2000/2000; menu/office/jumpscare — hard cut). `GameRender::DrawFade(alpha)`, logic freezes during fade-out.
- **Disclaimer like the original** — auto-advance after ~40 s, skip with any button (A/Start/B) after a short lock, no "PRESS START", 1010 ms fade-in/out.

**Save**
- **Device selector** (`XShowDeviceSelectorUI`) moved to the correct async pattern (XOVERLAPPED + `XHasOverlappedIoCompleted` + `XGetOverlappedResult`); fixed the `NULL`-overlapped bug that made the dialog not appear. Shows once at load (after the `.pak` check).

**Sound (1:1)**
- **Newspaper no longer hangs** — a separate `adCounter` (the menu used to zero the shared `menuFrameCounter` every frame).
- **Ambience starts AFTER the fade**, not during the blackout (edge detectors `g_officeAmb` / `g_menuAmb` / `g_nightBlip`).
- **`blip3` on the night-number card** ("what day", group 2) — 1:1.
- **Phone only in the office** — `TickPhoneCall` gates on `GAME_STATE_PLAYING` (the call used to leak onto the newspaper).
- **`robotvoice`** — removed the wrong loud loop; per the dump it starts muted (vol 0) and only rises via proximity triggers (not ported).
- **Full sound map** in `docs/AUDIO.md` (106 events, 53 samples, `Speaker` act #1/#11/#12/#17 semantics, channel/volume map).

---

## v2.14 — achievements

- **In-game achievements** (`Achievements.h/.cpp`): 10 achievements from `achievements.xml`, unlock via triggers, progress in `fnaf_save:\fnaf_ach.ini`, title screen (Y) + toast.
- **Two builds** via `#define FNAF_LIVE_SAFE` in `Achievements.cpp` (inverted in v2.20): default — system (`XUserWriteAchievements`, works on RGH/JTAG), with the macro — purely local (`save\` next to the .xex).
- **Menu sound restored** (the lost `g_audio.Play` on the disclaimer→menu transition).

---

## v2.8 — Perspective (clean-room shader)

- **Ported the HWA shader** `RPanorama.fx` (parabola) instead of the wrong sine: render target + `SpriteBatch::BeginSceneCapture/EndSceneCapture/DrawPerspective`; ps_3_0 in `SpriteBatch.cpp`.
- **`Resolve()`** moved to the real 10-argument XDK signature.
- **Disabled auto depth-stencil** (not needed, frees EDRAM for the capture RT).
- The perspective tuner reworked for the shader (ZOOM/CENTER_Y/CURVE).

---

## Earlier (base)

- Xbox-only XContent save (`[freddy]` INI, `fnaf_save.ini`), Delete-key wipe, DEMO = dead code.
- Full port of the frames/sprites/HUD: office, cameras, power-out, jumpscares, perspective, tablet, night flow — from the CTFAK dumps (`docs/*.md`).

---

## Remaining for full 1:1 (fine-grained audio tuning)

The mixer is done (dB layer + channels); only the rarest/niche items remain:

1. ~~Footstep distance (`deep steps` 10..40 by room, mute when overlapping).~~
2. ~~Freddy's kitchen music (ch22 music box) and pirate song (ch13).~~
   ~~Oven (ch10)~~ — now ported (plays when Chica is in the kitchen, v2.22).
3. ~~Golden Freddy giggle #38 + jumpscare~~ — full auto-activation done in v2.22 (poster → giggle on CAM 2B → office appearance → creepy kill).
4. ~~Phone call without delay~~ — removed (v2.19).
5. ~~Camcorder/tape-eject (ch6) on monitor-up (group 144)~~ — wired in v2.22.
6. **Ch16/ch24 proximity grading** — done by room on entry (v2.19); the "walk into the office" (`yellow bear`/Freddy inside) remains an approximation, as does the exact `vol zone` overlap for ch18/ch21.

We do not generate the SPA or sign the title (external tool/signing).
