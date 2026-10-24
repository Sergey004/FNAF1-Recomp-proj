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
3b. THE ENTRY TABLE — the OFFICIAL XBOX ONE SET (user-provided 2026-10-23;
    10 achievements, exactly 1000G, ids 11..20, spa id = slot + 10)
----------------------------------------------------------------------------
The code unlocks by SLOT (Achievements::UnlockFnaf2, mask fnaf2_ach.ini).
NOTE: the v2.62 slot order is DIFFERENT from this list — the code must be
remapped before the spa rebuild (see the slot map below; nothing has been
written to the profile yet — the spa still lacks ids 11..20 — so the local
fnaf2_ach.ini mask can simply be reset when the remap lands).

  id | slot | NAME (Xbox One)             | GS  | Secret | port trigger
  11 |  0   | One Night at Freddy's       | 70G |        | complete Night 1 (LIVE)
  12 |  1   | Two Nights at Freddy's      | 80G |        | complete Night 2 (LIVE)
  13 |  2   | Three Nights at Freddy's    | 90G |        | complete Night 3 (LIVE)
  14 |  3   | Four Nights at Freddy's     |100G |        | complete Night 4 (LIVE)
  15 |  4   | Five Nights at Freddy's     |120G |        | complete Night 5 (LIVE)
  16 |  5   | You Tampered                |200G |        | Custom Night with ALL AI
     |      |                             |     |        | at 20 (code fires the OLD
     |      |                             |     |        | slot 6 today -> remap)
  17 |  6   | You Soothed                 |135G |        | keep the music box wound
     |      |                             |     |        | ABOVE 50% for ALL of Night
     |      |                             |     |        | 4 (checked during play, not
     |      |                             |     |        | at the end) — NEW tracker
  18 |  7   | You Stayed                  |150G |  X     | complete Night 6 (code
     |      |                             |     |        | fires the OLD slot 5 -> remap)
  19 |  8   | You Tried                   | 40G |  X     | play ALL the minigames
     |      |                             |     |        | (per-minigame flags; the
     |      |                             |     |        | beatgame/turn saves are the
     |      |                             |     |        | seed) — NEW tracker
  20 |  9   | You Failed                  | 15G |  X     | experience a jumpscare and
     |      |                             |     |        | then... something more (the
     |      |                             |     |        | rare post-scare event; the
     |      |                             |     |        | exact trigger needs the dump
     |      |                             |     |        | check) — NEW

The v2.62 slots that DIE with this list: c1 preset (old 7), all-ten-
challenges (old 8), SAVETHEM discovery (old 9) — they are not in the Xbox
One set. The old names/GS below are kept only as history.

  [v2.62 PROPOSALS — SUPERSEDED by the Xbox One set]
  Night 1..5 10/10/20/20/40, Night 6 40, Custom 20/20/20/20 50,
  20/20/20/20 Challenge 30, Challenge Master 40, SAVE THEM 20 = 280G.

----------------------------------------------------------------------------
3c. READY-TO-TYPE STRINGS (v2.79 — for the XLAST entry; names <= 32 chars,
    descriptions <= 100, the XACHIEVEMENT limits; unlocked and locked
    descriptions share one string id, the same pattern ids 1..10 use)
----------------------------------------------------------------------------

  id | NAME                       | description (unlocked == locked)
  11 | One Night at Freddy's      | Survive the 1st night.
  12 | Two Nights at Freddy's     | Survive the 2nd night.
  13 | Three Nights at Freddy's   | Survive the 3rd night.
  14 | Four Nights at Freddy's    | Survive the 4th night.
  15 | Five Nights at Freddy's    | Survive the 5th night.
  16 | You Tampered               | Complete the custom night with all AIs
                                  | set to 20.
  17 | You Soothed                | Keep the music box wound above 50% for
                                  | all of night 4.
  18 | You Stayed                 | Survive the 6th night.
  19 | You Tried                  | Play all the minigames.
  20 | You Failed                 | Experience a jumpscare, and then...
                                  | something more.

Icon reminder: each id needs its own image entry in XLAST (imageId 11..20);
the art is whatever you put in — dump-derived FNAF2 art or placeholders,
replaceable by a later spa rebuild (the spa re-caches at every launch).

AUTHORING NOTE (user decision, 2026-10-23; refined 2026-10-24): the spa is
built with the XDK tooling — and it is fully CLI-driven, no GUI needed:
edit the .xlast XML directly (icons under achievements_pics\<game>\), then
`spac.exe proj.xlast -o out.spa -h out_spa.h -forceoverwrite` from the XDK
bin (verified end-to-end from WSL2; the round trip reproduces the shipped
spa byte-wise modulo the version counter). The in-house spaassembler/spaday
pipeline stays analysis-only — its output is not trusted by the real
console. The full mechanism and the rules (superset, projectVersion bump,
append-only) live in docs/ACHIEVEMENTS_MECHANISM.md.

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
