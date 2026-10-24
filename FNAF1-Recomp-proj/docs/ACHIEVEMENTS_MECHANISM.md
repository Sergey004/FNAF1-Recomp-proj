============================================================================
FNAF1-Recomp -- docs/ACHIEVEMENTS_MECHANISM.md
How achievements are added post-launch: the XDK rules + the Minecraft LCE
precedent (v2.79 research, applied to our pipeline)
============================================================================

WHAT THIS IS
------------
The research behind the "how do we add achievements for FNAF2/3/4/SL to an
already-shipping title" question. Sources: the XDK doc dump
(/home/user/FNAF1-Recomp/xbox-docs/md/*) and the leaked Minecraft Legacy
Console Edition source (/home/user/FNAF1-Recomp/MinecraftLCE-7thGen/),
the one 7th-gen title that kept adding achievements for years on a single
title id. Every claim below carries its citation.

THE XDK RULES (the system's own words)
--------------------------------------
1. The achievement DEFINITIONS live in SPA data (SPAFILE = XDBF container,
   sections XACH/XCXT/XGAA/XITB/.../XSTC/XTHD), compiled by the Game
   Configuration tool from a .gameconfig project (= the same
   XboxLiveSubmissionProject XML that XLAST edits; our
   FNAF1-Recomp-proj.xlast IS one):
     "You also must compile the .gameconfig file to produce a .spa file...
      This binary data will be linked into a binary section of your main
      executable. A .spa file can also be included as part of a game add-on
      ... The console uses local SPA data ... and will load newer versions
      of the compiled SPA data as they become available."
     -- Achievement_Unlocked_Game_Configuration.md:84-99
   We embed it exactly this way: the vcxproj links the spa as an additional
   section NAMED BY THE TITLE ID (AdditionalSections 464E4131=...spa,RW;
   docs/ACHIEVEMENTS.md:104-115). There is no runtime "register config"
   call; the system reads the section at launch.

2. The retail rule: new achievements may ONLY ship inside a content
   package (PDLC), never in a title update:
     "Achievements can only be added to a shipped title as part of a
      content package and cannot be added in a title update."
     -- XUserWriteAchievements.md:73
     "New achievements may only be added to a title as part of a content
      package. It is against LIVE Marketplace policy that they be added
      initially in a title update."
     -- profiles_achieve_overview.md:316-328
   THIS IS LIVE POLICY, NOT A TECHNICAL LIMITATION: the actual mechanics
   are version-based (rule 3), and Minecraft shipped achievement waves
   inside title updates for years (below). On an RGH/devkit setup with no
   LIVE submission, the policy simply does not apply; what applies is:
   replace the XEX (or add a PDLC package), and the console re-caches the
   spa at the next title launch.

3. Version discipline: the console caches every spa it sees and always
   loads the HIGHEST version; each new config must be a strict superset:
     "...each new version is a superset of any previous version" /
     "If the version number of the spa.bin included in the PDLC is lower
      than the version of the SPA data that is linked into the title's
      executable, the system will not load the spa.bin from the PDLC."
     -- Achievement_Unlocked_Game_Configuration.md:157-183, 553-590
   Additions only: "new achievements ... can be included as part of a game
   add-on. Modifications of existing items ... can only be shipped as part
   of a title update ... deleting or changing existing achievements [is]
   simply never allowed." (same file:200-207). The version lives in the
   .gameconfig project version (our .xlast projectVersion attribute).

4. Retail caps (irrelevant at our scale, listed for completeness),
   profiles_achieve_overview.md:87-253:
     Standard title:  5-50 achievements / 1000G at launch,
                      <=10 ach / 250G per quarter, <=50 / 1250G DLC total,
                      max 99 achievements / 2250G lifetime.
     Arcade:          max 62 achievements / 800G.

5. The write path (unchanged by any of this):
     XUserWriteAchievements(count, XUSER_ACHIEVEMENT{dwUserIndex,
     dwAchievementId}, overlapped) -- ids come from the GENERATED spa.h
     ("the defined achievement values ... are contained in spa.h ...
      therefore the logic to award the achievements must not be dependent
      upon the values contained in spa.h" -- profiles_achieve_overview.md:
     325-328; i.e. treat ids as data). Repeat writes of an earned id
     return success and change nothing. Requires a signed-in profile
     (ACCESS_DENIED otherwise); achievements persist and sync even when
     earned offline (profiles_achieve_overview.md:42-43). Guide list UI =
     XShowAchievementsUI; enumeration = XUserCreateAchievementEnumerator
     (xbox.h:1677-1748).

THE MINECRAFT LCE PRECEDENT (how the real game did it)
------------------------------------------------------
Everything below is from the leaked 7th-gen source tree.

1. The achievements NEVER lived in DLC. Every wave went through the same
   cycle: edit the gameconfig XML -> recompile the spa -> re-embed it in
   the XEX -> ship the new XEX inside the next title update:
   - Minecraft.Client/Xbox/GameConfig/Minecraft.gameconfig -- the source
     XML (XboxLiveSubmissionProject, titleId 0x584111F7,
     classification="TitleUpdate"); the achievement table is
     <Achievements nextId="22"> with ids 1-12, 14-21 (NOTE: id 13 is
     skipped -- gaps in the numbering are fine) at lines 3507-3528;
     names/descriptions are localized string-table entries, icons are
     <Image id=...> PNG files in the same directory.
   - Minecraft.Client/Xbox/GameConfig/Minecraft.spa -- the compiled XDBF
     (magic 'XDBF', sections XACH/XCXT/XGAA/XITB/XMAT/XPBM).
   - Minecraft.Client/Minecraft.Client.vcxproj:1244 (and every 360 config):
     <AdditionalSections>584111F7=$(ProjectDir)xbox\GameConfig\Minecraft.spa,
     RO;media=...</AdditionalSections> -- the spa rides INSIDE the XEX as a
     read-only section named by the title id. Exactly our setup.
   - The per-TU submission packages are Xbox/Title Update/TitleUpdate/
     Minecraft_TU12/13/14/15/17/18.xlast, and the build script
     (TUAutobuild.cmd + tubuild.plx:99-103) copies the UPDATED gameconfig
     next to each build -- the gameconfig travelled with every TU so the
     Live side matched the dashboard data. That submission half is the only
     part we do not need (no LIVE).
   - The growth is visible in the tree: the 360 spa.h carries 20
     achievements (ids 1-12,14-21), PS3's carries 28, PSVita/PS4's 50
     (Minecraft.Client/{PSVita,Orbis}/GameConfig/Minecraft.spa.h) -- the
     same title id, an append-only table, new ids appended per wave.

2. The game code side (what a title must contain):
   - An append-only internal award enum (Console_Awards_enum.h `eAward`;
     Minecraft.World/Achievements.cpp:109 -- "The order of these
     achievements is very important, as they map directly to data stored
     in the profile data. New achievements should be added at the end.").
   - A runtime mapping table from the internal award number to the spa
     achievement id: Xbox/Xbox_Minecraft.cpp:432-451 -- twenty
     ProfileManager.RegisterAward(eAward_X, ACHIEVEMENT_NN,
     eAwardType_Achievement) lines, ids straight from Minecraft.spa.h.
   - An award gate: gameplay must not award while cheats/creative/trial
     are active (Consoles_App.cpp:7615-7622 CanRecordStatsAndAchievements,
     ProfileManager.CanBeAwarded) -- our equivalents are the in-game cheat
     toggles and the dev unlocks.
   - Session dedup + local persistence: a per-session bitmask so a repeat
     trigger does not re-write (LocalPlayer.cpp:788 m_awardedThisSession),
     and the award state machine caching unlocked bits in its own profile
     data (the 4J_Profile lib: AwardNumberToGamerConfigID ->
     AwardAchievement -> the XDK write). Our equivalents: the module-state
     masks persisted to fnaf_ach.ini / fnaf2_ach.ini.
   - The Guide list UI: XShowAchievementsUI(0) (UI menus; our
     Achievements::ShowSystemUI).

HOW OUR PORT MAPS ONTO IT
-------------------------
Our cycle = Minecraft's minus the LIVE submission:
  1. Extend FNAF1-Recomp-proj.xlast (append the new ids, add their strings
     + icon images, BUMP projectVersion).
  2. Compile -> new FNAF1-Recomp-proj.spa + regenerated
     include/FNAF1-Recomp-proj.spa.h (ids 1..N constants).
  3. Rebuild the XEX -- the vcxproj AdditionalSections line already links
     whatever spa sits in the project dir. Console re-caches the spa at
     the next launch (highest version wins).
  4. The game side just keeps writing ids. The FNAF2 add-on unlocks are
     ALREADY LIVE (Achievements::UnlockFnaf2, slots 0..9 -> spa ids
     11..20, mask fnaf2_ach.ini; docs/FNAF2_DLC_ACHIEVEMENTS.md S3b has
     the entry table) -- the writes fail harmlessly until the spa carries
     the ids, then start passing on their own; everything earned before
     the spa caught up is already in the local mask (IsUnlockedFnaf2
     gates rewrites).
  5. Future modules: FNAF3/4/SL take the NEXT id ranges (21..N) in the
     SAME single spa -- append-only, per-module INI masks, no new
     packages. After adopting a regenerated spa.h, extend the
     static_assert id pinning in src/Achievements.cpp to the new range
     (the pinning pattern: a regenerated header with shifted ids must
     fail THIS build loudly).

THE XDK TOOLING IS CLI-DRIVEN (verified 2026-10-24, from WSL2)
--------------------------------------------------------------
The whole compile/decompile cycle runs WITHOUT any GUI, straight from
bash (the tools sit in XDK bin\win32; note they print localized error
text in the OEM codepage -- garbled in WSL, cosmetic):

  spac.exe / spac2.exe  "Xbox LIVE SPA Compiler 2.0.50912.02"
      spac proj.xlast -o out.spa -h out_spa.h -forceoverwrite
        -> compiles the gameconfig XML into the SPA + the C header
           (the header constants proved IDENTICAL to our shipped
           include/FNAF1-Recomp-proj.spa.h; the only deltas were the
           generation timestamp and the project version counter).
      spac -u shipped.spa -o NAME
        -> DECOMPILES a spa back into the gameconfig XML
           ("464E4131.original.gameconfig") + every icon PNG. Verified:
           the decompiled XML of our shipped spa matches the on-disk
           .xlast EXACTLY (modulo the image paths that were re-pointed
           at achievements_pics\FNAF 1\ after the spa was built).
           NOTE: -o for -u ignores the directory name; files land in the
           CURRENT directory.
      other flags: -verbose, -nospa/-noh, -xsd (emit the project XSD),
      -config overrides, -noarcadeinfo/-arcadeinfo (arcade titles; our
      project ships an ArcadeInfo.xml from this same tool).
  blast.exe (xblast 2.0.21256.0)
      blast project.xlast /build [/build:DVD] [/build:Live] [/clean]
        -> the CLI packager for content packages / title updates from a
           .xlast project. Only needed for the real PDLC route; the
           base-spa fallback (our plan) never touches it.
  xlast.exe
      GUI-only (no CLI: -? opens the editor window). The editor is
      OPTIONAL now: the .xlast is plain XML, edits can be done directly
      (the per-game icon folders live under achievements_pics\FNAF 1..
      FNAF SL; the .xlast <Image><Path> entries reference them).

The round trip is therefore fully scriptable: XML edit -> spac -> new
spa + spa.h -> copy the header -> rebuild the XEX. The user decision
"the spa is built with the XDK tool, not our spaassembler" stands --
spac.exe IS the XDK tool; spaassembler/spaday remain analysis-only.

ENTRY CHECKLIST FOR THE PENDING FNAF2 WORK (S3b of FNAF2_DLC_ACHIEVEMENTS.md)
-----------------------------------------------------------------------------
- The 10 FNAF2 achievements carry the OFFICIAL XBOX ONE SET (names/GS/
  secrets) -- the full 50-achievement registry for every game, with the
  event conditions and the port-status marks, is docs/ACHIEVEMENTS_SETS.md.
- 10 achievements, ids 11..20 (spa id = slot + 10), names/descriptions in
  the XACHIEVEMENT limits (label <= 32 chars, description <= 100,
  xbox.h:1707-1709); icons as new images (imageId per icon); unlocked and
  locked description may share one string id (our existing pattern).
- projectVersion strictly higher than the deployed spa's.
- Nothing else: no content package, no manifest edit -- the vcxproj links
  the spa, the console caches it, the writes pass. The F2 code-side remap
  (the slot order changed with the Xbox One set) is the one code task that
  must land before/with the spa rebuild.
