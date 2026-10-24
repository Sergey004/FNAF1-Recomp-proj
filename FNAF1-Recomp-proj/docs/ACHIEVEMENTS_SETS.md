============================================================================
FNAF1-Recomp -- docs/ACHIEVEMENTS_SETS.md
The 50-achievement event registry (10 per game, the Xbox One sets, 1000G
each). v2.79 spec -- the basis for the achievement_id -> event_id ->
condition table and for the future per-module unlock code.
============================================================================

SOURCES AND AUTHORITY
---------------------
- The achievement lists (names, gamerscore, secret flags) are the XBOX ONE
  re-release sets, user-provided from the console listings
  (xboxachievements.com / Exophase, 2026-10-23). The descriptions are the
  console strings. The port conditions are ours, derived from the lists.
- FNAF1 is the exception: "У F1 те же ачивки, никто их не менял" -- the F1
  set (names) is the same and is ALREADY in the shipped spa (ids 1..10,
  the ORIGINAL 360 gamerscores 20/20/30/50/50/50/100/30/30/20 = 400G).
  The Xbox One re-release re-valued F1 to 1000G (60/70/80/90/100/110/200/
  120/120/50) -- the USER will re-assign the F1 gamerscores later, in the
  same XLAST session as the F2..SL entry. When that lands, sync the code
  mirror in one pass: src/Achievements.cpp kAchievements (GS values for
  the in-game records; the id static_asserts are GS-independent), the
  doc comments that quote "400 G", and the totals below.
- The id assignment: F1 keeps 1..10; each later game appends a range of 10
  (append-only, the Minecraft rule -- docs/ACHIEVEMENTS_MECHANISM.md):
  FNAF2 = 11..20, FNAF3 = 21..30, FNAF4 = 31..40, SL = 41..50. All five
  sets live in the ONE base spa (50 achievements; 4400G with today's F1
  values, 5000G once the user re-assigns the F1 gamerscores; the retail
  2250G lifetime cap is a LIVE-policy number, lenient on RGH -- see the
  mechanism doc). Where the two user listings disagree on the ORDER of the
  non-night achievements, the FIRST listing (the console dashboard order)
  fixes the ids; the GS+name pair is the real anchor.

THE EVENT MODEL (implementation law)
------------------------------------
Four event kinds feed all unlocks:
  NightCompleted(game, night)          -- a night survived AND saved.
  ChallengeCompleted(game, challenge, difficulty)
  MinigameCompleted(game, minigame, result)
  DeathOrJumpscare(game, event_id)     -- one UNIQUE death/scare event.
Rules (apply to every game):
  1. Fix the EVENT first, then let the achievement check it. Do not weave
     unlocks into animation/sound/UI logic.
  2. Successful completion is separate from defeat: a final frame, a room
     transition, and "the shift ended" are not the same event.
  3. Accumulative achievements store FLAGS (minigame_seen[id],
     animatronic_deaths[id], custom_challenge_completed[id], ...), persisted
     per game module (the fnaf_ach.ini pattern: fnaf2_ach.ini, fnaf3_ach.ini,
     ...), so progress survives sessions.
  4. Difficulty/configuration checks read the ACTUAL run parameters at
     award time, not a menu selection made earlier.
  5. Unlocks are IDEMPOTENT: one UnlockAchievement(id) funnel per module,
     deduped by the local mask (IsUnlocked) before the system write, exactly
     like UnlockFnaf2 today.
  6. Conditional per-night flags (music box, cove, ...) RESET at night
     start and are evaluated at night end (or continuously where noted).

FNAF 1 -- ids 1..10 (ALREADY IN THE SPA, 400G -- nothing to do)
---------------------------------------------------------------
Same ten names as the Xbox One set; the port triggers are live and
HW-verified (src/Achievements.cpp kAchievements + Game.cpp). Recorded here
only as the reference of what each name means:
  One/Two/Three/Four/Five Nights at Freddy's -- NightCompleted(FNAF1, 1..5)
  Overtime                -- NightCompleted(FNAF1, 6)
  No Tampering            -- ChallengeCompleted(FNAF1, CustomNight, 20/20/20/20)
  No Running              -- Night 4 ended, Foxy never left Pirate Cove
                             (flag foxy_escaped_pirate_cove must be UNSET)
  No Laughing             -- Night 5 ended, Freddy never reached CAM 4A/4B
                             (flag freddy_reached_cam_4a_or_4b must be UNSET)
  No Hiding               -- the first jumpscare/death plus the follow-up
                             event (the original's "You Failed"-class beat)

FNAF 2 -- ids 11..20 (1000G) -- spa entry per docs/FNAF2_DLC_ACHIEVEMENTS.md S3
------------------------------------------------------------------------------
  id  NAME (GS, secret)          event / condition
  11  One Night at Freddy's 70G  NightCompleted(F2, 1)            [LIVE, slot 0]
  12  Two Nights at Freddy's 80G NightCompleted(F2, 2)            [LIVE, slot 1]
  13  Three Nights at Freddy's 90G NightCompleted(F2, 3)          [LIVE, slot 2]
  14  Four Nights at Freddy's 100G NightCompleted(F2, 4)          [LIVE, slot 3]
  15  Five Nights at Freddy's 120G NightCompleted(F2, 5)          [LIVE, slot 4]
  16  You Tampered 200G          NightCompleted(F2, custom) with
                                 custom_ai_all_20 (read from the RUN config)
                                 [code fires the OLD slot 6 today -> REMAP]
  17  You Soothed 135G           music_box_above_50_entire_night4 -- the
                                 wind level stays strictly above 50% for
                                 ALL of Night 4; the flag RESETS the moment
                                 it touches <= 50%; checked continuously
                                 [NEW tracker]
  18  You Stayed 150G SECRET     NightCompleted(F2, 6)
                                 [code fires the OLD slot 5 today -> REMAP]
  19  You Tried 40G SECRET       all minigame_seen[id] flags set (one per
                                 UNIQUE minigame; the beatgame/turn save
                                 fields are the seed) [NEW tracker]
  20  You Failed 15G SECRET      post_jumpscare_event_completed -- a
                                 jumpscare followed by THE specific follow-up
                                 event. NOT any death; the second half of the
                                 description is intentional -- verify the
                                 exact trigger against the dump first.

FNAF 3 -- ids 21..30 (1000G) -- spa entry + module code are TODO
----------------------------------------------------------------
  id  NAME (GS, secret)          event / condition
  21  One Night at Freddy's 50G  NightCompleted(F3, 1)
  22  Two Nights at Freddy's 60G NightCompleted(F3, 2)
  23  Three Nights at Freddy's 70G NightCompleted(F3, 3)
  24  Four Nights at Freddy's 80G  NightCompleted(F3, 4)
  25  Five Nights at Freddy's 90G  NightCompleted(F3, 5)
  26  Nightmare 110G             NightCompleted(F3, nightmare)  (night 6)
  27  Aggressive Nightmare 200G  ChallengeCompleted(F3, nightmare,
                                 aggressive)  (night 7; do NOT alias it to
                                 plain Nightmare)
  28  Trapped 40G                Springtrap REACHES THE OFFICE (the defeat
                                 event), not an arbitrary jumpscare
  29  The End 100G               NightCompleted(F3, week 1 complete) -- the
                                 first week's shift sequence
  30  True End 200G SECRET       the good ending via the minigame chain:
                                 store per-stage flags (k1..k4/cake already
                                 exist in the freddy3 save), award on the
                                 goodend ROUTE, not on watching the final

FNAF 4 -- ids 31..40 (1000G) -- spa entry + module code are TODO
----------------------------------------------------------------
  id  NAME (GS, secret)          event / condition
  31  One Night at Freddy's 60G  NightCompleted(F4, 1)
  32  Two Nights at Freddy's 70G NightCompleted(F4, 2)
  33  Three Nights at Freddy's 80G NightCompleted(F4, 3)
  34  Four Nights at Freddy's 90G  NightCompleted(F4, 4)
  35  Five Nights at Freddy's 100G NightCompleted(F4, 5)
  36  6th Night 125G SECRET      NightCompleted(F4, 6)
  37  Nightmare 150G SECRET      NightCompleted(F4, nightmare)  (night 7)
  38  20/20/20/20 200G SECRET    NightCompleted(F4, night 8) with all four
                                 AI at 20 (read from the RUN config)
  39  "Fun" 75G SECRET           minigame_wins[Plushtrap] AND
                                 minigame_wins[BalloonBoy] -- WINS only; a
                                 start or a loss does not count
  40  Too Bad 50G SECRET         the required set of UNIQUE animatronic
                                 deaths (animatronic_deaths[id]); dedupe
                                 identical scares under different names --
                                 the exact id list needs the dump check

SISTER LOCATION -- ids 41..50 (1000G) -- spa entry + module code are TODO
-------------------------------------------------------------------------
  id  NAME (GS, secret)          event / condition
  41  Controlled Shock 50G       NightCompleted(SL, 1)
  42  Breaker Room 60G           NightCompleted(SL, 2)
  43  Parts and Service 70G      NightCompleted(SL, 3)
  44  Spring Lock Maintenance 80G NightCompleted(SL, 4)
  45  Ice Cream 50G              the ice-cream DELIVERY to the child
                                 completes -- picking the item up is not
                                 enough
  46  Meet The Cast 100G         ALL required unique death/scare events
                                 (the registry below)
  47  Look Like You 90G SECRET   the special Night-5 route following Baby's
                                 instructions, verified against the original
  48  Exotic Butters 150G SECRET survive until morning in the secret night
  49  You Won't Die 150G SECRET  all custom_challenge_completed[id] on
                                 V. Hard (all_custom_challenges_completed)
  50  Fazbear's Fright 200G SECRET golden_freddy_mode_completed -- its OWN
                                 flag, never the generic custom-night flag

THE MEET THE CAST REGISTRY (SL, candidates -- verify per route, mark only
CONFIRMED unique events; several conditions may lead to the same scare):
  Night 2:        death_bidybab, death_ballora, death_freddy
  Night 3:        death_foxy, death_bon_bon
  Night 4:        death_minireena
  Night 5:        death_baby_code, death_baby_flash, death_baby_wrong_route,
                  death_baby_instructions
  Minigame:       death_minigame_fall
  Secret night:   death_ennard_power, death_ennard_door, death_ennard_vent,
                  death_ennard_camera
  Custom Night:   per-animatronic events (Ballora, Freddy/Bon-Bon, Yenndo,
                  Foxy, Bonnet, Bidybab)
The final required SET is whatever the dump proves; the achievement fires
when the last flag of that set lands.

OPEN ITEMS (need the dump / the routes before coding)
-----------------------------------------------------
- F2 "You Failed": the exact post-jumpscare event.
- F3 "Trapped": the office-reach defeat event id; "True End": the exact
  minigame chain that counts as the route.
- F4 "Too Bad": the unique death id list.
- SL "Meet The Cast": the confirmed unique-event set; "Look Like You": the
  instruction route. (Several of these live in wave-2 SL content.)
- F2 code remap BEFORE the spa rebuild: slots 5/6/7/8/9 change meaning
  (see FNAF2_DLC_ACHIEVEMENTS.md S3b); reset the local fnaf2_ach.ini mask
  when the remap lands -- nothing was ever written to the profile.
- New per-module masks follow the fnaf2_ach.ini pattern (fnaf3_ach.ini,
  fnaf4_ach.ini, sl_ach.ini) on the same XContent/fallback backend; each
  module gets its own UnlockFunnel(id) with IsUnlocked dedup.
