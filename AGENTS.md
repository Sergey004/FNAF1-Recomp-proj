# AGENTS.md — FNAF1-Recomp workspace

Xbox 360 recompilation project (UK/EN code + RU user). The user writes in Russian; "делай"/"Продолжи" = proceed.

## Main project (git root = workspace root)
- **Main directory: `/home/user/FNAF1-Recomp/FNAF1-Recomp-proj`** (git root; the working copy is its subfolder `FNAF1-Recomp-proj/`).
- VSCode work happens in `/home/user/FNAF1-Recomp/FNAF1-Recomp-proj/FNAF1-Recomp-proj/`:
  - `src/` — game loop `main.cpp`, `Game.cpp` (state machine), `GameRender.cpp` (all FNAF1 screens), `AnimatronicAI.cpp`, per-game modules `FNaF2/3/4Render.cpp`, systems (`PowerSystem`, `DoorSystem`, `Progress`, `MenuSystem`, `PakLoader`, `SpriteBatch`, `AudioSystem`).
  - `include/` — headers; `Types.h` (GameState enum, tick/timer constants, room/camera ids), `assets/` (per-game `asset_mapping_<gameId>.hpp`).
  - `docs/` — the project's decoded knowledge base (see "Read before touching" below).
  - `CHANGELOG.en.md` + `CHANGELOG.ru.md` — bilingual history, newest entry at top; ALWAYS update both on a change. `CHANGELOG.md` is just an index.
- Workspace root also holds: `ctfak-cppnew/ctfak-cpp/build/Dumps/<game>/` (the game dumps — source of truth), `xenia-canary/` (PC emulator for testing), `junk/` (original retail exes), `spaassembler/` + `fnaf1.spa*` (achievements/title data), `xbox-docs/`, `xbox_includes/`. These are reference material, not edited.

## Build
- **No Linux build exists.** The project is VS2010 + Xbox 360 XDK (Direct3D9 + XAudio2, C++03). The user builds on their Windows machine ("Release Xbox 360" config) and runs on RGH hardware or Xenia. After code edits: tell the user to build; never claim a build was verified locally.
- **Never edit `.vcxproj` / `.vcxproj.filters`** (user manages them by hand; batch edits broke the VS view once). Add code to existing files or provide file lists instead of touching project files.
- No lint/test/CI. Verification = user on console/Xenia; debug console, DEV menu (Start+B), sprite browser (LB+RB), perspective tuner (L3+R3) exist in-game.

## Ground rules
- **Dump is the only authority**: original behavior = the CTFAK dump at `ctfak-cppnew/ctfak-cpp/build/Dumps/Five Nights at Freddys/` (`Events/frame_3_Frame 1_events.txt` = all gameplay logic, `FrameLayout/*.txt` = instance tables, `JSON/application.json`+`banks.json`, `Images/*.png`). When the user overrides the dump (visual preference, verified oddity), label it explicitly in a code comment as a deviation.
- **No external reference sources** in docs/changelog/code comments (no wiki links, no "per other ports").
- Version string lives in `src/main.cpp` (boot banner + debug console) and in the changelog headers; bump all on a change.
- Open TODOs live in `~/.zcode/cli/memories/projects/project-39fa1fc199e9d456/memory/fnaf1-recomp-state.md` — refresh it after finishing work.

## Code conventions
- C++03 (VS2010): no C++11 features (e.g. braced-init rejected — use `PakHotspot(x, y)` form); `Snprintf` wrapper instead of `snprintf`; big static tables in cpp files; comments in English (changelog bilingual).
- Use the Edit/Write tools for file changes (no python/sed/awk in Bash for project edits); Read before Edit.

## Gotchas
- **Dump timer units are milliseconds**; the dump's `(~Xs)` annotations assume 50 Hz ticks and are 20× wrong for this game (e.g. `timer 4970` = 4.97 s, `timer 1000` = 1 s). This misled past sessions — always multiply annotations by 20 in your head.
- Logic tick = 60 Hz (`tickDelta = 1/60`, accumulator +1/60 per frame). A past half-speed bug (1/30) made all timers run 2× slow; do not reintroduce.
- Counters: the movement events use the "activity" counters (bonnie/chica/fox/freddy activity); the customize AI counters are separate globals read only on night 7.
- XDK quirks: `exit(0)` kills the process (Golden Freddy keeps it on purpose); other exits use `ExitToDashboard()`; message boxes are async (pump frames while waiting); save/load via XContent with a local fallback.
- `fnaf1.pak` textures are referenced by raw image handle `img_<N>`; maps of handles live in `include/PakAssets.h`.

## Read before touching
- Any AI/movement change: `docs/AI_MECHANICS.md` (event-group evidence) first.
- Any render/layer change: `docs/OVERLAY_MAP.md` (layer map, static/overlay truth), `docs/PERSPECTIVE.md` (office/feed bend), `docs/CAMERA_FINDINGS.md`.
- Frame flow: `docs/FRAME_TRANSITIONS.md`. Architecture: `docs/ARCHITECTURE.md`. Saves/achievements: `docs/SAVES_XCONTENT.md`. Audio: `docs/AUDIO.md`.
- Naming: `GameRender.cpp` is FNAF1-only; other games have their own renderers — don't put FNAF1 logic there.