============================================================================
FNAF1-Recomp -- docs/ACHIEVEMENTS.md
In-game achievements (v2.14)
============================================================================

WHAT THIS IS
------------
Ten in-game achievements mirroring `achievements.xml`, unlocked at the same
gameplay moments the original retail title would, but implemented in-game
because this recomp cannot ship a signed Xbox LIVE SPA (see "System API +
two build flavors", below).

The achievements (id / gamerscore / trigger):

| id | Name             | GS  | Trigger                                                      | Secret |
|----|------------------|-----|--------------------------------------------------------------|--------|
| 1  | One Night …      | 20  | survive Night 1                                              | no     |
| 2  | Two Nights …     | 20  | survive Night 2                                              | no     |
| 3  | Three Nights …   | 30  | survive Night 3                                              | no     |
| 4  | Four Nights …    | 30  | survive Night 4                                              | no     |
| 5  | Five Nights …    | 50  | survive Night 5                                              | no     |
| 6  | Overtime         | 50  | survive Night 6                                              | yes    |
| 7  | No Tampering     | 100 | Custom Night complete with all AI == 20 (Night 7)            | yes    |
| 8  | No Running       | 30  | Night 4 without Foxy sprinting (FOXY_STAGE_4)                | no     |
| 9  | No Laughing      | 30  | Night 5 without Freddy reaching the East Hall                | no     |
| 10 | No Hiding        | 20  | caught by an animatronic (any jumpscare)                     | yes    |

The table lives in `src/Achievements.cpp` (`kAchievements`), not parsed from
the XML — `achievements.xml` is kept as the human-readable spec and its icon
file names are recorded in the `icon` field for a future icon pass.

STATE & PERSISTENCE
-------------------
The unlock state is a 10-bit mask kept in `Achievements::m_unlocked` and
written via `Progress::SaveAchieve` (key `unlocked=N`) — a SECOND file next to
the night-flow save, so the hidden Delete-key progress wipe (which resets only
`fnaf_save.ini`'s `[freddy]` section) never clears achievements. The storage
backend follows the same `FNAF_LIVE_SAFE` switch as the night-flow save:
`fnaf_save:\fnaf_ach.ini` in the system build (reusing the already-chosen
XContent device, so the device selector still appears only once), or
`save\fnaf_ach.ini` in the Live Safe build.

Per-night "prevent" flags (`foxyRan` / `freddyEast`) reset in
`Achievements::BeginNight`, which `main.cpp` calls next to every
`game.Init(night)`.

TRIGGER WIRING (main.cpp)
-------------------------
- `OnNightComplete(night)` -> `Achievements::OnNightComplete(night, perfect)`
  unpacks ids 1..7 (7 only when `perfect` = all four AI levels are 20) plus the
  Night-4 "No Running" and Night-5 "No Laughing" checks.
- `OnJumpscare(anim)` -> `OnJumpscare()` (id 10).
- `OnFoxyStageChange(FOXY_STAGE_4)` -> `OnFoxyRan()`.
- `OnAnimatronicMove(ANIM_FREDDY, ROOM_EAST_HALL)` -> `OnFreddyEast()`.

DISPLAY
-------
- Title menu: press **Y** to open the achievements list.
  - System build (default): Y opens the **Xbox Guide achievements list**
    (`XShowAchievementsUI`) — the system shows the list.
  - Live Safe build (`FNAF_LIVE_SAFE`): Y opens the in-game screen
    (`GameRender::RenderAchievements` — text list with unlocked count + total
    gamerscore; secret achievements render "???" until earned), closed with
    **Y**/**B**.
- On unlock, `Achievements::Unlock` raises a transient toast drawn over any
  screen by `GameRender::DrawAchievementToast` (4 s) as the in-game fallback;
  in the system build the Guide's own "Achievement Unlocked" toast also fires
  (from the SPA data) via `XUserWriteAchievements`.

SYSTEM API + TWO BUILD FLAVORS
------------------------------
Two build flavors, controlled by `FNAF_LIVE_SAFE` (used in BOTH
`src/Achievements.cpp` and `src/Progress.cpp` for the storage backend):

- **Default (no macro) — "обычная"**: touches the Xbox system. Unlock writes
  the profile via `XUserWriteAchievements` ("ACH n -> 0x…", works on
  devkit/LIVE and RGH/JTAG), Y opens the system list via `XShowAchievementsUI`,
  and saves/achievements persist through XContent
  (`fnaf_save:\fnaf_save.ini` / `fnaf_ach.ini` + the device-selector UI).
- **`FNAF_LIVE_SAFE` defined — "Live Safe"**: does NOT touch the system.
  Saves/achievements persist to local files `save\fnaf_save.ini` /
  `save\fnaf_ach.ini` next to the .xex; no `XUserWriteAchievements` /
  `XShowAchievementsUI` / XContent calls; the in-game screen + toast are the
  whole experience.

Toggle in `src/Achievements.cpp` (same switch also gates `src/Progress.cpp`):

    // #define FNAF_LIVE_SAFE   // uncomment for the local "Live Safe" build

Leave it commented out (the default) for the system/profile build.

`XUserWriteAchievements` only records "achievement N earned"; the name,
description, gamerscore, icon and the Guide's "Achievement Unlocked — 10G"
toast come from the title's SPA game-config. Without a signed SPA, the system
list/toast still show but may lack localized text/GS — which is why the in-game
mask in `fnaf_ach.ini` (or `save\fnaf_ach.ini`) remains the persistent source of
truth and the in-game UI is kept as fallback. `xapilib.lib` is already in the
project's linker inputs, so nothing else changes to switch flavors.

ICONS (future work)
-------------------
The PNGs under `achievements_pics\` are not rendered yet; the screen is
text-only. To add them, load each `game:\achievements_pics\<icon>` via
`D3DXCreateTextureFromFileEx` and draw through `SpriteBatch`.