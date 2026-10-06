============================================================================
FNAF1-Recomp -- docs/FNAF2_DLC_ACHIEVEMENTS.md
"More achievements from FNAF2+" via a DLC content package (research)
============================================================================

Question: how do we ship MORE achievements (FNAF2's, later FNAF3/4/SL)
on top of the base title's 10?

============================================================================
1. WHAT THE OFFICIAL RULES SAY (XDK docs, Achievements white paper)
============================================================================
- New achievements CAN be shipped "as part of a game add-on (PDLC)":
  "new additions to the game configuration, such as new leaderboards, new
  rich presence strings, and new achievements, can be included as part of
  a game add-on."
- The add-on's game config is classified **"Game Add-on"** in the
  configuration tool and compiles to a **spa.bin** that is included in a
  downloadable content package (not linked into the XEX).
- **Superset rule**: every new game config must be a SUPERSET of the
  previous one (add only — never remove/rework existing achievements,
  images or strings).
- **Version rule**: the console loads the HIGHEST-version SPA it can find
  (XEX section vs PDLC spa.bin). If the PDLC's spa.bin version is LOWER
  than the XEX's, it is silently ignored ("the system will not load the
  spa.bin from the PDLC, and the achievements will not be available").
- The runtime registration is still just XUserWriteAchievements with the
  ids from the new spa.h; the Guide shows the merged list.
- (Retail footnote: adding achievements/gamerscore through PDLC required
  Microsoft approval — irrelevant on an RGH/devkit setup.)

============================================================================
2. THE PIPELINE WITH THE TOOLS WE ALREADY HAVE
============================================================================
1. **Game Configuration tool**: create the add-on project as a SUPERSET —
   the 10 base achievements (ids 1..10, untouched) + the new ones
   (FNAF2's: ids 11..N). Classification: "Game Add-on"; Validation Base
   Version: the base project; project version strictly HIGHER than the
   base config's.
2. Compile -> **spa.bin** + a new **spa.h** (it defines ACHIEVEMENT_* for
   the new ids).
3. **XLAST -> Content Package Wizard**: create a content package offer,
   add the spa.bin to the package payload, build. XLAST produces the
   STFS content package ready for the console.
4. **Deploy**: place the built package into the console's content area
   for our TitleId (Content\0000000000000000\<TitleId>\<offering>\... —
   XM360/FSD handle the layout on RGH). XAM merges it at title boot
   (highest version wins).
5. **Code**: include the new spa.h and award the new ids when FNAF2
   unlocks happen (the same SystemWrite path as FNAF1's — the ids come
   from the add-on spa.h).

============================================================================
3. FALLBACK (SIMPLER, 100% WORKS OFFLINE)
============================================================================
Extend the BASE .xlast with the FNAF2 achievements (ids 11..20) and
rebuild the base spa + spa.h. One spa, one Guide list mixing all parts,
no content package needed. The retail caps (50 achievements / 1000G base)
are not a concern at our scale (400G + ~400G) and are lenient on RGH.
Use this if the XLAST content-package build or XAM's package discovery
misbehaves; upgrade to the real PDLC later — the ids stay the same, so
nothing in the save/achievement mask changes.

----------------------------------------------------------------------------
3b. THE CONCRETE ENTRY TABLE (v2.62 — the game side is LIVE, type these 10)
----------------------------------------------------------------------------
The code unlocks by SLOT (Achievements::UnlockFnaf2, mask fnaf2_ach.ini);
the spa id = slot + 10. Enter in XLAST exactly:

  id | suggested name            | GS  | unlock condition (the code beat)
  11 | Night 1                   | 10G | complete night 1
  12 | Night 2                   | 10G | complete night 2
  13 | Night 3                   | 20G | complete night 3
  14 | Night 4                   | 20G | complete night 4
  15 | Night 5                   | 40G | complete night 5 (beatgame)
  16 | Night 6                   | 40G | complete night 6
  17 | Custom Night 20/20/20/20  | 50G | the custom night with all ten AI at 20
  18 | 20/20/20/20 Challenge     | 30G | survive the "20/20/20/20" preset (c1)
  19 | Challenge Master          | 40G | survive all ten challenge presets (c1..c10)
  20 | SAVE THEM                 | 20G | discover the 8-bit chain (the 1/1000 roll)

(GS values are proposals — tune to taste; 280G add-on, base 400G + 280G is
under the 1000G cap.) After the rebuild the profile writes pass on their
own — the local fnaf2_ach.ini mask already tracks everything earned before
the spa caught up (the IsUnlockedFnaf2 gate prevents rewrites).

============================================================================
4. CODE-SIDE NOTES
============================================================================
- Achievement ids are data, not logic: FNaF2 unlocks map to ACHIEVEMENT_*
  constants; the fnaf_ach.ini-style bitmask stays per-game-module.
- The superset rule means the add-on spa.h REDEFINES ids 1..10 too — keep
  the static_assert pinning (Achievements.cpp pattern) when adopting it.
- The saved unlock bitmasks per part live in the part's own module state,
  so the profile writes never collide.
============================================================================
