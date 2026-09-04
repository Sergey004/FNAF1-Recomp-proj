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
written to `fnaf_save:\fnaf_ach.ini` (key `unlocked=N`) — a SECOND file in the
same `fnaf_save` XContent root, so the hidden Delete-key progress wipe (which
resets only `fnaf_save.ini`'s `[freddy]` section) never clears achievements.
`Progress::LoadAchieve`/`SaveAchieve` reuse the existing XContent device, so
the device selector still only appears once.

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
- Title menu: press **Y** to open the achievements screen, **Y**/**B** to close
  (`GameRender::RenderAchievements` — text list with unlocked count + total
  gamerscore; secret achievements render "???" until earned).
- On unlock, `Achievements::Unlock` raises a transient toast drawn over any
  screen by `GameRender::DrawAchievementToast` (4 s).

SYSTEM API + TWO BUILD FLAVORS
------------------------------
`XUserWriteAchievements` records "achievement N earned" on the signed-in
profile — this works on devkit/LIVE **and on RGH/JTAG dashes**. What it does
NOT carry is the display metadata (name/description/gamerscore/icon and the
Guide's "Achievement Unlocked — 10G" toast): those come from the title's SPA
game-config. So on a recomp without a signed SPA:

- the in-game mask in `fnaf_ach.ini` is the persistent source of truth, and
  the achievements screen + toast are rendered in-game;
- `XUserWriteAchievements` is still called per unlock (result logged
  "ACH n -> 0x…") so the earned flag reaches the profile where the dash can
  show it.

The call is behind a compile-time switch at the top of `src/Achievements.cpp`:

    #define FNAF_LIVE_SAFE    // defined  -> call XUserWriteAchievements
                              // undefined -> pure-local (no LIVE dependency)

Define it (the default) for the devkit/LIVE/RGH/JTAG build; comment it out to
build a strictly-local flavor that never touches the Xbox LIVE API.
`xapilib.lib` is already in the project's linker inputs, so nothing else needs
to change to switch flavors.

ICONS (future work)
-------------------
The PNGs under `achievements_pics\` are not rendered yet; the screen is
text-only. To add them, load each `game:\achievements_pics\<icon>` via
`D3DXCreateTextureFromFileEx` and draw through `SpriteBatch`.