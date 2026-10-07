# Changelog — FNAF1 Recomp (Xbox 360)

Notes on what is done and what is left. Versions match the code
comment tags (`v2.8`, `v2.14`, …, `v2.32`) and the historical notes.


---

## v2.65 — Sister Location wave 1: the flow plays on the streaming pak

The "monster" (the 1.5 GB game) is playable through its core loop (files
this wave: **include/SLGame.h, src/SLGame.cpp, include/SLRender.h,
src/SLRender.cpp — ADD THE FOUR TO THE .vcxproj BY HAND**):

- **The go-to router + the load frame** (docs/SL_MECHANICS.md §2): gameplay
  arms the global `go to` counter; the dump's frame-4 "load" consumes the
  code every 200 ms through the route table (1 Elevator / 3 Hub / 5,8 vent
  hop / 6,11 Breaker / 7 Ballora / 10 Funtime / 12 Baby / 13 Scooping /
  30 bathroom), and the same router IS the death-resume path (the
  game-over resumes the night with the Script Event presets 81/111/57/512).
- **The walk rooms** (docs §3-4): no avatar — "you are the camera" with
  the pan tween; W-hold arms the 15-tick crawl latch (Shift = the fast/
  loud variant), the quick-count ticks into progress; **Ballora's dance**
  (she joins at progress 400, the walk builds `distance`, standing decays
  it, 5 s of continuous cranking makes her approach, her music pans the
  side cue, death over 600) and **Funtime's flash beacon** (Space = flash:
  the 2 s refill, flashing ADDS +50 to him, the three distance-band
  silhouettes, death over 500 flashing / 600 walking, the backwards-exit).
- **The ride + the hub** (frames 2/3/5/7): the elevator ride state machine
  (the movement anim, the doors-open beat), the Circus Control desks with
  the availability-gated vent picks, the vent crawl (100 held ticks per
  notch, 10 notches to pass, the duct-fast/slow sounds).
- **The win chain** (frames 6/12/24/25): Baby's Room ends the night on the
  fade → win night (the Jingle_4b + the INI `current += 1`) → the tv show
  hold → the Girl Voice interlude (the line per night) → the next night.
- **The fnaf_sl save** ("sl" section: current/intro/beat1/beat3/keycard/
  endsceneno/104) through the same storage backend; the Warning-frame
  Delete wipe.
- **Wave-2 leftovers, all labeled**: the HandUnit scripted beats per room,
  the P&S face-button task + the breaker task, Under Desk (Bidybab
  eye-match), the Scooping/final chain, the extras + the custom night
  (frame 31, 501 groups — the FNAF1-style office), the 8-bit platformer
  (frame 26, 12800 px), the Perspective-ease port for the room tween,
  the real room art pins (wave 1 draws the dump's own Backdrop cells +
  debug-font captions where the frame art rows were not captured), and
  SLSfxPan is a no-op (AudioSystem has no per-channel pan yet — the Ballora
  side cue rides volume; labeled).
- Version → v2.65. Files: include/SLGame.h + src/SLGame.cpp +
  include/SLRender.h + src/SLRender.cpp are NEW — add all four to the
  .vcxproj.

---

## v2.64 — FNAF4 completed: the slot-resolved flow, Fun with Plushtrap/BB, the lockbox, the extras, the house scenes

Everything the dump has is now wired (all in the existing files — nothing to add to the .vcxproj):

- **The storyboard-slot resolution applied to FNAF4** (the v2.63 lesson): the real slot order is 0 level, 1 game over, 2 what night, 3 night win, 4 title, 5 intro plushtrap, 6 plushtrap game, 7 lockbox, 8 game over 2, 9 extras, 10 load extras, 11 disclaimer, 12 Cutscenes, 13 ending, 14 test, 15 nightmare jumpscare, 16 demo, 19 BB game. The big corrections: the night win's "night 7 → plushtrap" edge actually goes to the **LOCKBOX** (V7), and the cutscene exits printed as "night win" actually go to the **intro to plushtrap** (V5) — the real per-night chain is 6 AM → Cutscenes → the Plushtrap intro → the Plushtrap game → the night card → the office.
- **The flow screens**: the real **what night** card (Night := INI night; the shadow counter forces nights 7/8 — the shadow nights are entered from the extras; ambience + the 2 s clock beat → 2.1 s → the office); the real **night win** (the four "6 AM" digit slots flicker Random(10)/250 ms and settle on the 2/2.5/3/3.5 s beats, the alarmclock4 loop, the fn4 INI writes night+1 + beat5/6/7/8 (beat8 only with no cheats) + the s1..s6 challenge stars, the 10 s route); **game over** (7 s → title, shadow cleared); **game over 2** (4 s → the night card, or the lockbox on the extras replays); the intro holds (Deep_Ambience, 6 s / A); the nightmare jumpscare hold (distortion, 5 s → the disclaimer slot); the test/demo skip screens.
- **Fun with Plushtrap** (frame 7) — the full 63-group engine: the 9-position hall graph (in chair → hall stage 1 → the far left/right fork → hall stage 2 → the close left/right fork → hall stage 3 → got you) advanced by the darkness accumulator ("becoming active": +1 per dark tick, the move roll at ≥ 400+Random(100) every 2 s, the darkness resets on the flash), the A-hold flash (the flashlight sfx on the edge, the view states 0/1/2/3 per the dump's Active.alt0), the per-position view anims (the dump's own tables: the chair 879, the hall 748, the jump-back 14, the four room poses 15-18, stage 2/3 with the "drop" sfx), the fork rolls (Random(2)+1 per second), the per-night clocks (90/60/45/30 s), the win = flashing him at hall stage 3 ("great!" + reward), the caught = flashing at "got you" (the 21-cell jumpscare anim 23) or the clock running out, and the 150-frame win view → the night card.
- **Fun with Balloon Boy** (frame 18) — the same engine with its config: the 45 s clock, the aggressive Random(5) roll, the 500 ms extra move at stage 3, the bb1b/bb2b/bb3b voice roll (1/3 per 2 s, always at stage 3 — the taunt), the BB reward flag (the s3 challenge star) and the exit to the lockbox/extras (BB is extras-only).
- **The lockbox** (frame 9, slot 7): reached after beating night 7; A plays the unlock (unlock2 sfx), the lid float (10 frames) and the 20 s auto-end → title.
- **The extras** (frame 10, the title's 4th row): the 10 dump rows — the animatronics/making-of/plushtrap-making viewers (6 picks: Nightmare Freddy/Bonnie/Chica/Foxy/Fredbear/Plushtrap), the jumpscare player (scream2/3), **fun with plushtrap** and **fun with BB** (the "minigame play" replays), the **shadow nights** (night 7 / the 20-20-20-20 night 8 — the card forces Night := 7/8), the **cheats row** (house map / fast nights / radar / blind mode / insta foxy / mad freddy / all nightmare — fast nights shortens the hours to 30 s, labeled factor), the challenges row and the exit.
- **The house cutscenes** (frame 12, 5120×3840): the walkable world (100 ms / 25 px steps, the 5×5 page screen-follow at the 384/512 mid-lines), the boy follower, the scene number = the INI scene (0 = the title intro, = the night for the between-night plays), the exit routing (scene 0 → the card; 1-4 → the Plushtrap intro; > 4 → title) and the black-line wipe. **Labeled stop-gap**: the dump's dialogue is drawn per-letter as sprite images and no string table survives the dump — the talk box renders and advances without invented text, and the per-scene trigger scripts (202 groups) run as a timed walk.
- **The ending** (frame 13): the typewriter talk box + the advance, same labeled stop-gap on the text content.
- **The fn4 save**: night, scene, beat5..beat8, s1..s6, test and the cheat toggles — loaded at module boot, written on the night-win/game-over beats and the cheat flips, through the same storage backend.
- Version → v2.64. **FNAF4 = 100 % code-side** (README ×2 updated). Next per the roadmap: **Sister Location** (the SL_MECHANICS.md digest is the plan; the streaming loader + the Perspective easing port are staged).

---

## v2.63 — FNAF3 completed: the slot-resolved flow, the retro cutscenes, the six Atari minigames, the extras

Everything the dump has is now wired (all in the existing files — nothing to add to the .vcxproj):

- **The transition decoder bug found and fixed**: the dump's jump VALUES are STORYBOARD-SLOT indexes, not frame-file indexes — ctfak resolves them naively, so every printed transition name was wrong (that's also how the old "in-place night restart per g606" reading crept in). A temporary handle-list print was added to ctfak (chunk 8747), the real storyboard order extracted, and the whole flow re-derived: office win → V4 next day → V13 load → V12 cutscenes → (Next = file order!) → the frame-17 chooser → V2 what day → V6 wait → V0 office; office death → V3 static → V5 gameover → V1 title / V19 rare2; boot 1/1000 → V16 rare1; card 1/1000 → V18 rare3; minigames return to V2. The extras menu is slot 24 (the title's 4th row), the six minigames are slots 15/17/20/21/22/23.
- **The flow screens landed**: the death static (frame 4: stare loop + the flash flicker, 5 s) → the **Game Over screen** (5 s / A → title, 1/1000 → rare2) replacing the in-place restart; the 100 ms wait screen between the night card and the office; the rare1/2/3 screens (imgs 225/228/252 + the "crazy garble" loop); the **load glitch screen** (frame 18: "long glitched2", the crawling line-jump dots, 5 s) which increments the cutscene counter on entry; the end chooser (frame 17) now routes by the counter (≠ 5 → the next night card; == 5 → bad end / good end by the saved goodend flag) and the ending holds got their dump durations (15/15/59 s — the old 150/590 were the ~Ns trap).
- **The retro cutscenes frame** (frame 16, 190 groups): you play the scene's animatronic walking the retro pizzeria as a 5x5 room grid (one 1024x768 room at a time, 250 ms / 30 px steps, the per-room open-direction table from the barrier groups, the wrap-around leave zones), with the Shadow Freddy figure of each room drifting onward (g96-105), the rain drops, the scurrying rat, the scanlines, the checkered floor and the stage/curtain room, and the 5 s controls hint. The **Purple Guy** arms after the (2,5) blocked-up ERR, homes onto you at 30 px/100 ms with the "crazy garble" loop, and the collision plays the take-apart (anim [386,400] @10) + the scene's parts sprite (Freddy 401 / Bonnie 410 / Chica 420 / Foxy 452) and the death fade (100 frames) → the chooser. **Scene 5** (after night 5) opens the back room (1,5): the Purple Guy patrols, four passes and he bolts for the Springtrap suit ("run" → "scare" ×4 → "insuit" → the staged crush with "laugh"/"crush"/"insuit") → the chooser → the endings.
- **The six Atari minigames** (frames 19-24) on one shared platformer engine (the FNAF3 counterpart of the FNAF2 grid engine): 100 ms walk/fall ticks (15/20 px, fall 10 px), the 60 ms rise tick (20 px) with the per-game jump counters (7/7/9/7/2 and 10 from the balloons), the W-release cut, sensors vs a per-game rect kit (the dump's platform images at their layout positions — pixel-perfect backdrop collision is not recoverable without pixel data, labeled), the 1024x768 viewport scrolling in pages over the 3072x2304 world at the 384/512 mid-lines, the 200-frame win counter, and the per-game music (mb4b/mb5/mb8/mb9/mb1/mb2) + the jump/land/collect/get/feed sfx. **BB** (office CAM 08 toy / slot 15): 7 balloons + the balloon pickup ("get this") writing `bb` (balloons in every game) + the big cake feeding the kid (`k1`); **Mangle** (the arcade cabinet, night 2 / slot 17): the cake pickup writing `cake`, four follower kids, the crying-kid gather; **Toy Chica** (slot 20): collect food → feed the four guests → the exit at four feeds + `k2` (with the freeze cutscene); **GFreddy Stage01** (slot 21): the zip pads + `k3`; **RWQ** (the night-5 dark room / slot 22): the S-view teleports (5 fixed spots) + the dump's 1-in-2 glitch re-roll per 100 ms + `k4`; **Marion / Happiest Day** (the CAM 03 puppet toy / slot 23): the party cakes, the exit — and the **goodend trigger**: with all four kids fed (k1..k4) the big cake fires the `goodend` INI write and the float finale (the child's anims 12→14, the ghost kids + the balloons rising, win at 1300 frames) — that is the good/bad ending split.
- **The extras menu** (frame 25, the title's 4th row): five rows — the **animatronics viewer** (7 entries, the dump's viewer anim imgs 393/395/438/1015/1014/828/1008 + names), the **minigame replays** (gated by goodend; "extras game?" sends the wins back to the menu), the **jumpscare player** (gated by beat6; the six anims, scream3), and the **cheats** (gated by both; fast nights / ventproof / hyper / nocams — the office obeys them: 20 s hours, the seal blocked, +2 AI, the monitor locked) with the cooldown, and the select sfx everywhere.
- **The freddy3 save now carries the full dump key set**: `level`, `cine`, `beat6`, `4thstar` (night 6 with no cheats), `goodend`, `bb`, `cake`, `k1..k4`, `fast`, `vents`, `hyper`, `nocams` — loaded at boot, written on the dump's beats (the next day, the endings, the minigame pickups, the cheat toggles).
- **Labeled deviations**: the pixel-perfect backdrop collision → the per-game rect kit (coarse per piece); the arcade/cupcake/keypad secret cam picks are approximations (the BB CAM 08 / puppet CAM 03 cams are dump-pinned; the console triggers the secrets with X on the cam, the dump double-clicks); a few minigame item positions marked ~ sit on the room kit where the layout rows were not captured; the gameover caption is debug font (the frame's art rows were not captured); the cutscene props (party tables/gift) do not block movement (cosmetic only); the minigames' Escape (B) abandons to the entry point instead of the dump's End application.
- Version → v2.63. **FNAF3 = 100 % code-side** (README ×2 updated). Next per the roadmap: FNAF4 (~30 %) to playable + minigames, then SL.

---

## v2.62 — FNAF2 completed: the dump's frame flow, the freddy2 save, and the 8-bit minigames

Everything the dump has beyond the office/title now runs (all in the existing files — nothing to add to the .vcxproj):

- **The save**: file **"freddy2"** (the dump's own Ini file) through the same storage backend (XContent container + the local fallback). Keys verbatim: `level` (the 6 AM screen saves the NEXT night), `cine`, `turn`, `beatgame`, `beat6`, `beat7`, `c1..c10`. Loaded at boot (the 6th-night row rides beatgame, custom rides beat6), written on the dump's beats (6 AM, endings, the dream `cine`, the minigame rotation `turn`), and the title's X-hold wipe clears it.
- **The flow screens** (frames 8/2/4/5/13/14/15/9/10/11/12/16/17/18/6/20/22): the HELP WANTED newspaper after New Game; the real night card (429.png + the rare 1/1000 branch); the post-night static (stare loop, 1-in-10 → the rare app-end loader → the boot selector, the dump's End application); the 6 AM screen (seven-segment art + the crowd cheer + the save) routing to the DREAM after nights 2/3/4 or the endings after 5/6/7; the panning 2500px dream (the static flicker, the blackout fade, Robot at 30 s, and the cine==0 ? "it's me" : "err" exit — the first-run dream counter); the three ending screens (paycheck / pink slip / robots-scrapped, each writing beatgame — that's what unlocks the 6th night); the three rare screens.
- **Death rerouted per the real game**: the scare now leads to the frame-6 "Game Over" face (10 s / any key → title) instead of the in-place restart — the dump's only route INTO frame 6 is its gameover edge, and the 1/1000 roll from there opens the 8-bit chain. (The old in-place restart remains for the danger pipeline's own restart semantics.)
- **The custom night setup** (frame 12): ten AI sliders 0-20 (D-pad row, left/right ±1, coin), the ten challenge presets verbatim (20/20/20/20, New and Shiny, Double Trouble, Night of Misfits, Foxy Foxy, Ladies Night, Freddy's Circus, Cupcake, Fazbear Fever, Golden Freddy) with the `doing custom` arm + the `c<N>` beaten flags, the all-20 flag (beat7), the 1987 combo (1/9/8/7), READY → night 7 and the sliders BECOME the office AI (g622).
- **The 8-bit minigames** (the shared engine — the movement model FNAF3/4/SL minigames reuse): grid-step motion gated by `10 + add_to_timer` frames with facing, sensors vs obstacle rects, the SAVETHEM hub (20 px/200 ms steps, the 5x5 wrap-around room grid with per-room dressing, Puppet chasers, the Golden Freddy cameo, the 1/101 Purple Guy on a right-wrap, "you cant" → the loader, the SAVETHEM letter voice); **TAKE CAKE TO THE CHILDREN** (feeding resets the kids' sadness; at 20 s the purple car → Purple Guy → the step-gate rot +10/s → +25/pass → the scripted attack); **GIVE GIFTS, GIVE LIFE** (the Puppet: 4 gifts → center → 4 lives with the volume ladder → the attack); **Foxy's party** (2048px, the camera snaps at x=1024, the phase/cycle machine, the second visit brings the Purple Guy and the fatal trigger); the rotation loader (`turn` +1 per second, reset ≥5: 0/1→bars, 2→cake, 3→gifts, 4→bars, 5→party) and the bars screen.
- **Labeled stops**: the per-night card anim cells, the 5→6 digit roll and the minigame walk/attack cells are anim banks the dumper did not split — those ride debug-font/approximation art (the attack = the white flash into static, img 23/361); the minigame background-collision is a walkable-rect clamp (the dump needs pixel data); the challenge-name text for the mode row is debug font.
- **v2.62 add-on pass (same release)**:
  - The night-card cells and the 5→6 roll WERE in application.json after all — the card now draws the real per-night art (objInfo 45: night 1..7 → 429/430/431/436/437/426/425) and the 6 AM screen plays the real 18-cell roll (objInfo 224 @ 6 fps, ending on the "6"), with the crowd cheer moved to the roll's end (the dump's anim-finished beat).
  - **The FNAF2 add-on achievements are wired in-game**: the mask lives in **fnaf2_ach.ini** (bits 0..9 = spa ids 11..20), `Achievements::UnlockFnaf2(slot)` persists + attempts the profile write through the same pipe (a harmless failure until the .xlast carries the ids). Slot map (name the XLAST entries to match): 0-4 nights 1-5, 5 night 6, 6 custom all-20, 7 the 20/20/20/20 challenge, 8 all ten challenges, 9 the 8-bit chain discovered. `AppServices` grew `ach` so modules reach the core's owner.
  - **FNAF3 got its freddy3 save** (level/cine per its next-day dump) through the same bridge, and two real bugs were fixed: the vent seal NEVER completed (the progress was never ticked — now it closes the vent at 50+Random(50) frames with glitch2), and the phantom stare counters did not reset on re-arm (an instant re-scare after 20 s — now fresh stares); the vent map also draws the sealing progress bar.
- **v2.62 second pass ("правь всё что надо")**:
  - **The minigames' walk cycles and attacks are REAL now** (application.json): the TAKE CAKE Freddy waddles [412,417], the party Foxy [722,726] (3 fps), the Puppet 760; the scripted attacks draw the "attack animation" object's actual fullscreen lists — 15 (Foxy, 14 cells @36 fps), 20 (the car/man, 15 @30), 21 (the Puppet, 13 @24) — replacing the flash stopgap; MgAttack stores the anim value.
  - **FNAF3's GOT YOU 2 finally kills**: the window scare (groups 510/512) — monitor up + a 1 s beat + Random(2), or instantly under the blackout — never fired before (nothing set gotYou = 2); the 1 s timer resets on entering GOT YOU 2.
  - **FNAF3's cameras draw the dump's own feed pieces**: the empty/Springtrap cells (17/30, 14/43 … 45-49/50-54) from the FeedImg table now composite over the img 104 base, fit-boxed at native aspect from the pak texture dims (shadow-Freddy's 61/60 keep their full-frame overrides).
  - **FNAF3's office foreground state**: objInfo 123 switches 203↔204 by the vent error (groups 337/338/344) — the office now draws the 204 vent-error foreground with the hallucination flicker (the dump's coefficient-50 alpha, group 346).
  - **FNAF3's end screens landed** (the third pass): the "COMING SOON! Fazbear's Fright" newspaper after New Game (frame 8, img 0 — the real art); the post-night route now goes next-day → **rare2** (img 228 + the "crazy garble" loop, 5 s) → title per the dump's transitions; night 5's 6AM plays the **ending chooser** (frame 17 — the anim cells are not dumped, a captioned hold) writing beatgame; the **bad end** (img 346 + mb2), the **good end** (img 172 + the "ending" song, the dump's 590 s hold — B skips) and **the end 2** (img 123 + mb2) are wired with their beatgame writes; the nightmare (6th-counter) run routes to the good end per the dump. The full good/bad split arrives with the minigames wave.
- **FNAF2 code-side 100 %** (the fourth pass): the minigame kids got their REAL sadness cells (objInfo 393: content [692,685] @3 fps / crying [519,482] @6 fps / past-20 [693,696] @18 fps) and the crying kid 2 plays the real 8-cell murder reaction (objInfo 394 anim 14 @ speed 1) when the car stops; the CRT scanline flicker is dump-driven (every 500 ms the alpha coefficient := 200+Random(100)); the Active 16 scene table was VERIFIED cell-for-cell against application.json (89 values, zero mismatches — the v2.59 transcription was exact). The only thing outside the code: entering the 10 add-on achievements into the .xlast per the concrete table now in docs/FNAF2_DLC_ACHIEVEMENTS.md §3b (ids/names/GS/unlock beats, the game side is live and will start passing writes after the spa rebuild). The Mangle office-crawl visual states (Active 20) remain a labeled stub — its kill path works through the danger pipeline.
- Version → v2.62. **FNAF2 = 100 % code-side** (the .xlast entry table is the user's one-time XLAST task). FNAF3 ≈ 50 %.

---

## v2.61 — FNAF2 title dump-exact + FNAF3/FNAF4 night loops

- **FNAF2 title reworked to the dump** (frame_1_title.txt + 71 groups):
  - The invented "Freddy glitch" (imgs 65/73/210 every ~7 s) is REMOVED — those images are THE PUPPET (full body / head), the title dump never fires the static object's anims 12-15 (they are office cam events). That was the "Puppet appearing out of nowhere" on the title.
  - The debug-font stacked words are gone — img 469 IS the real stacked "Five Nights at Freddy's 2" art (verified the PNG), already drawn at (96,39).
  - The static draws with the dump's alpha coefficient 50+Random(100) per 1.8 s (the old draw was OPAQUE — the eye-view background drowned); point-sampled like FNAF1's overlay, no X jitter (the dump sets only the alpha).
  - The background got FNAF2's "lamp": alpha coefficient Random(250) re-rolled every 6 s (group 5 — the mirror of FNAF1's v2.57 title lamp).
  - "blip flash 2" (img 68, the fullscreen band flash) added: visible only in the 1-in-3 six-second windows, alpha 200+Random(50).
  - Menu rows fixed to the dump layout: Continue is img **303** at (86,507) — the old draw used img 449, FNAF1's own continue art; 6th Night (90,582), Custom (89,650), the selector rides {442,512,587,655}. Stars (img 593) for beatgame/beat6. The "Night N" row (img 270 + digits at 185,567) shows while Continue is selected (groups 51/52).
  - The footer is the real art now: img 294 "v 1.033" (26,738) + img 631 "Press and hold delete to reset all data." (335,736); "(c)2014 Scott Cawthon" stays debug-font text. No Demo tag (full game, DEMO? = 0).
  - Continue is ALWAYS visible per the dump rows (boot optionCount = 2); after 6 AM the count is 2 + unlocks and the selector opens on Continue when a night was played (dump groups 43/44).
- **FNAF1 title static is more transparent** (user call): coefficient 130+Random(60) → alpha ≈ 0.25..0.50 (was 80+Random(70) → 0.42..0.69).
- **FNAF3 night loop** (new FNaF3Game, office frame 773 groups): the hour clock (raw-ms timers — 40 s/hour night 1, 60 s nights 2+), AI 0/2-5/7 per night; the Springtrap room graph (10 rooms + 5 vents + the four attack stages that only advance while a screen is open), vent sealing with the bounce table, the audio lure (charge 7, 1/7 fizzle, the adjacency table, Random(100) completion), the five phantoms + Golden Freddy's walk + Shadow Freddy's cam overrides, the maintenance panel (audio/camera/vent meters, the per-AI decay, reboots, the vent-error blackout with hallucinations), 15 cams with the dump feed table, jumpscares 778/792 and the IN-PLACE night restart (group 606 reloads frame 3 — no game-over screen in this build). Console map (no cursor): LB = monitor, D-pad = map highlight, A = cam, X = lure (room map) / seal (vent map), RB = room↔vent map, Y = the maintenance panel, LS/LT/RT = office pan.
- **FNAF4 night loop** (new FNaF4Game, bedroom frame 480 groups): the follow-state machine over five positions (center/left door/right door/closet/bed), the four threat lanes — Bonnie/Chica at the doors with the listen-for-breathing defense, the shut-door visits (3 s), the linger bedroom attacks; Foxy's closet counter (3+R5, watched-drain) with the forced flashlight and the non-fatal bite; Freddy's bed counter (+= AI/4 s, −1/s watched, minis at 10/20/30, the attack at 60); Fredbear from night 5 (replaces the closet, kills through the scare), the paranoia black-flash pipeline (5/4/3/2 s by total danger), 60 s hours with the per-night/hour AI tables, the night-win clock → title. View art comes from the dump's anim tables: halls 89/88 and 255/375 (open/shut), the closet 422 + Foxy stages 304/286/288/290 (Fredbear 266), the bed 511 + states 492/423/386/391, the walk darks 45/160/57, the flash 99. Console map: D-pad walks (up = to bed, down/B = out), A (hold) = peek, X (hold) = door, standing still at a door = listening.
- **FNAF1 closed to 100 %**:
  - **The "died" screen is implemented** (frame 4): after the scare the fullscreen static now plays with its MISSING pieces — the one-pass blip flash ([23,23,23,4,25,6,8,9,10,21,22] @ 45 fps: three pure-white frames flashing into noise, then the object destroys itself) and the **static loop on ch1 at vol 100** (was: silent 1.6 s burst). The phase holds **10 s** (group 4's timer 10000 ms), then the backroom "gameover" holds **10 s** with StopAll on entry (group 1) and the 1/10000 creepy roll re-firing EVERY second (group 5 — the port rolled once at the end). The jump value 7 from "died" resolves to the gameover slot (the printed "wait" name is the FNAF1 name-printing bug) — the chain scare → died → gameover → title is now the dump's own.
  - **Controls match the official console port** (the memory's target scheme): LS pan, LB/RB lights, LT/RT doors, Y honk were already right; the camera model changed — A raises the monitor, in camera mode the D-pad moves a PRE-SELECTION over the cam strip (green plate at low alpha; the console's highlight art was never dumped) and A confirms the switch, B exits camera mode. A no longer lowers the monitor; B no longer raises it.
  - **The vol-zone "heuristic" caveat is closed by proof**: the dump tests bonnie/chica sprites overlapping the invisible "vol zone" (objInfo 136) × foxy progress, volumes 0/30/50/75 — the port's additive presence units reproduce that table row-for-row, and a sprite sits in the doorway zone exactly when its AI room is the corner/door room. Documented in the ladder's comment; no behavioral gap.
- **Build note**: FNaF3Game.cpp/.h and FNaF4Game.cpp/.h are NEW files — add them to the .vcxproj (like FNaF2Game back in v2.31).
- Version → v2.61.

---

## v2.60 — FNAF2 title navigation (FNAF1-style) + Puppet office warning

- **The title menu navigates like FNAF1**: up/down moves the selector (img 229) through the four dump rows — New Game (301), Continue (449), 6th Night (298), Custom Night (438) — wrapping inside the visible list; A confirms. Visibility rides the session unlocks until the FNAF2 save system lands: Continue appears after any night (caps at 5 per the dump), 6th Night after beating night 5, Custom after night 6. A mid-wave cursor experiment (mouse-parity) was tried and REMOVED at the user's call — console games use buttons, no pointers.
- **Puppet presentation**: the office now shows the "danger 1" face (img_494/495, angry 496/497 at gauge ≤200) while the box gauge is low and the Puppet is out — the monitor already had "danger 2". The Puppet still emerges by the dump's stage rolls at gauge 0 and never re-boxes.
- **Image bugs from the first HW run**: the on-screen debug console no longer draws over the FNAF2+ titles (it stays on FNAF1's flow, where it is the log sink, and returns everywhere via the DEV menu's Console item — Start+B); the title's stacked words are spaced 64 world-px apart so the debug font stops overlapping (real fonts remain a later wave).
- Plumbing: `GameInput::lookDirY` (stick Y, deadzoned) and `aHeld` (A level) added for future use.
- Version → v2.60.

---

## v2.59 — FNAF2 real feed views (scene selector) + DLC-achievements research

- **The cameras now show WHO is there.** The dump's cam-view groups (g44-136) were transcribed into a scene selector: `FNaF2Game::ComputeSceneValue()` mirrors the "Active 16" value per (viewing, lit?, presence) with the dump's group order (last match wins); the renderer maps the value to the feed image through Active 16's own anim table. Value 0 = "no matching view" and the feed KEEPS its previous image — the dump's own stick-behavior (cam 1 lit-empty, cam 3/4/2 unlit-empty cases). The office follows the same selector: dark office (35) keeps the panned world; lit hall views (36/55/56/58/73/76/84/93/97/99) replace the room image full-frame, with Freddy-under-table, toy-Bonnie and BB sprites, the danger darkening and the mask overlay on top.
- **DLC achievements research** (docs/FNAF2_DLC_ACHIEVEMENTS.md): new achievements may ship in a game add-on — a "Game Add-on" classified config compiles to spa.bin inside a content package (XLAST Content Package Wizard); the add-on config must be a SUPERSET of the base and carry a HIGHER version (the console loads the highest-version SPA). Fallback for offline: extend the base .xlast with FNAF2's ids (11..20) and rebuild the base spa — ids stay the same either way.
- Version → v2.59.

---

## v2.58 — FNAF2 wave 1: the playable office core

FNAF2's office (751 event groups) was fully decoded into docs/FNAF2_MECHANICS.md, and the port implements it:

- **AI for 11 characters** (old/toy Freddy/Bonnie/Chica, old Foxy, Mangle, BB, Puppet, Golden): the 5 s opportunity rolls (old Foxy weighted by dark-charge, strict `<` for Mangle/BB/Golden), the per-night/hour AI schedule with caps (15/17/10), arming gates (stage pairs, watched-stage freeze, Mangle unwatched-only), and the movement graphs one node per arming — office entry only with the monitor up and the office unoccupied.
- **Danger pipeline**: time allowed 100/80/60/55/50/50/45 frames per night, got-you stages (mask during the window = saved), the darkening overlay with the flicker ramp, the box race 50%/s kill vs 10% mask escape, monitor-up-too-long rule, forced-drop semantics via the encounter stages.
- **Mask**: on/off machine with the FENCING sounds and breathing loop; closes the monitor/flashlight; per-character defenses (Puppet immune); the toxic bar is cosmetic, exactly like the dump.
- **Music box**: gauge 0..2000, wind on CAM 11 (+5/frame, windup2 ticks), per-night drain (2..6 per 50 ms), night-1 freeze during 12-1 AM, the Puppet's emerge stages -> walk -> office -> kill, danger faces at 400/200.
- **Flashlight/Foxy**: battery per night (7000..3000, -1/frame), hall-light freeze on entries, Foxy's dark-charge (doubles while masked in a clear office) and the light-push retreat at 100+night frames.
- **Jumpscares**: attack animation values 12..21 per attacker (frame lists verbatim from application.json) + Xscream3, then the night restarts in place (the dump's own death flow — no game-over screen in the chain).
- **Audio**: the dump channel set (buzzlight/CMPTR/fansound/deepbreaths/stare/melody/garble/jackinthebox/popstatic/With_S2) driven through the game's volume hooks; cam-switch blip, flip, FENCING, windup2, error, ventwalk, metalrun wired by name.
- **Input**: RB = mask, X = wind (CAM 11), LT/RT = vent lights (with the BB error), LB = flashlight, A = monitor, D-pad = cams.
- Version → v2.58.

---

## v2.57 — the title's Freddy "lamp" + point-sampled static

- The missing effect from the title: the background brightness breathes. Dump group 5 re-rolls the Active 2 alpha coefficient to `Random(250)` every ~6 s, mapped via the project's coefficient rule (on-screen alpha = (255 − coeff)/255 — the same one the title static uses) — the Freddy backdrop breathes dim↔bright with deep dips like the original. No clamps invented this time (my earlier version narrowed the range against the dump's full swing).
- **Static is point-sampled now** (`SpriteBatch::DrawPoint`): the dump's coarse 1024px noise frames keep their grain when stretched to 1280 — the original's Clickteam default is nearest-neighbour, and our linear sampler was softening the noise. Applies to every `DrawStaticOverlay` user (title, office static-out, camera-switch interference).
- **Window-hash fix (real-HW bug)**: the title rolls used the LOW bits of a Knuth multiplicative hash, whose sequence CLUMPS — on the console the Freddy variant flashed exactly once and then nothing for a minute ("акк раз и нету"). All four title rolls (bg variant, lamp, static alpha, band) now run through a murmur3-style finalizer (`TitleWinHash`) that spreads every window independently; simulated spread matches the designed odds (~15% lit, lamp dips ~1/min, band ~1/18 s).
- Version → v2.57.

---

## v2.56 — FNAF2 rendering: one mapping, feeds pan like the office

- FNAF2's world is 1024x768 with wide 1600x768 office/feed art; the target look is the PC's fullscreen stretch. All drawing now goes through ONE transform (kScaleX = 1280/1024, kScaleY = 720/768) — including the title's debug text, which used to sit at leftover pillarbox coordinates while the sprites stretched.
- Camera feeds are no longer squashed flat into the window: a 1600-wide feed pans with the office's pan (same window), a 1024-wide one stays pinned; the "needs a scissor" TODO for feeds is gone.
- Monitor map/buttons/HUD untouched (already frame-coords-aware via the same path).
- Version → v2.56.

---

## v2.55 — loose save self-deletes after a successful import (scope-tight)

- After `ImportSave()` copies the loose `freddy` into the save storage, the LOOSE source at `game:\freddy` (next to the .xex) is removed — otherwise every boot re-offered the same import. `game:\save\freddy` is NEVER touched: it is the Live Safe build's own save home. A failed delete logs a non-fatal warning.
- Real-HW (user's console): **[achievements verified]** — the profile write works and the Guide shows all ten from the embedded SPA; and the full save chain (import → profile container → reload) round-trips.
- Version → v2.55.

---

## v2.54 — save wipe by holding X on the title (confirmed)

- The X button was untouched on the title — **hold it 5 s** there for a system confirmation box (the standard SysPrompt flow, "No" focused): "Delete the whole save? All nights and stars will be reset." → "Yes" wipes the progress save (nights + stars). This version **replaces** the older hidden cheats (USB-keyboard Delete, LT+RT pad hold) — they are removed, the X hold + box is now the single wipe path. The achievements bitmask survives as always. Log on the debug console: `SAVE WIPED (X hold)`.
- Plumbing: new `GameInput::xHeld` level (XInput button 0x4000, guard-defined for old XDK headers), `PollTitleXWipe` next to the two older wipe paths.
- Version → v2.54.

---

## v2.53 — the dump-alignment wave (audit-driven)

A full audit of the port against `docs/DUMP_ATLAS.md` surfaced mostly-cadence and input-routing drift. All fixed:

- **Saves truly fixed as a data bug**: New Game no longer wipes the beat stars (the original only wipes them on the hold-Delete cheat; New Game writes just `level=1`).
- **Golden Freddy is rare again**: arm = the dump's silent global **1/100000 per second** roll (was fused into the 1/100 monitor-drop poster roll — thousands of times too common); while armed CAM 2B always shows the Golden poster (dump rule).
- **Freddy 4A→4B now needs the right door light OFF** (his corner step; the port used to advance regardless).
- **The x20 timer artifact cleaned**: kitchen clatter re-roll 4 s, ITSME arm 1/1000 *per second*, pirate 4 s (Foxy stage 0), circus 5 s, breaths 5 s (intruder inside + cams up), rare pounding 10 s 1/50 at 10..50.
- **Doors/lights per the dump's input law**: buttons only work with the monitor down; 10-tick click cooldown; lights mutually exclusive; a door mid-slide ignores the toggle; the error stinger answers only a close-attempt onto an occupied doorway (and only Bonnie@left / Chica@right); power-out force-opens go through the animated slide+motor; the doorway pose lights up only at the door zone and the `windowscare` stinger fires on its first reveal.
- **Audio ladders per the dump**: door-light hum = silent-off / loud-on with the 1/10 strobe; dread EERIE = 0/30/50/75 (+Foxy≥2) and 100 when Freddy is in; `robotvoice` obeys night≥4 + the corner zones + the 1+5r / 1+20r office-vs-cam stairs at 100 ms rolls; hop steps mute while you watch the mover's room and get the missing 20-tier; kitchen audio follows cam-6 watching (0/10/20/75 + tune 0/5/50) with the 300 s camp replay; the flip-up now pairs the whir with blip3+tape-eject (our invented monitor static loop removed); the phone mute button only exists at +20..+40 s.
- **Foxy**: sprint sound = one-shot `run` on the 3→4 trigger, and the sprint frames show on the CAM 2A feed (the office no longer shows Foxy sprinting through it).
- **Numbers**: usage can reach 6 (no clamp), a door bills only when the slide lands, and the night is 90+89·5 = 535 s (the dump's minute counter resets to 1).
- Scare timings (v2.49) untouched; Continue still reaches nights 6/7 (deliberate console-UX deviation, noted in the atlas).
- Version → v2.53.

---

## v2.52 — full dump atlas (documentation only)

- The whole FNAF1 dump is now read end-to-end (all 435 office event groups, every frame's events + layouts, `application.json`/banks.json global tables). Captured forever in a new **`docs/DUMP_ATLAS.md`**: the dump-reading rules (raw-ms timers; the misprinted jump-target names + the corrected storyboard slot map), the corrected frame graph, per-frame digests, the office alterable/channel registries, and the object/animation/sample tables.
- The same corrected routing was folded into `docs/FRAME_TRANSITIONS.md` (its mermaid graph + edge list were built on the printed names, which turned out misaligned — e.g. New Game really lands on the `ad` newspaper frame, 6 AM exits route by the advanced night number, Golden and 1987 both end at `creepy end`).
- No code changes.
- Version → v2.52.

---

## v2.51 — the title matches the dump's lamp/static behavior

- **The lamp flicker on the Freddy backdrop** (groups 4, 8-11): every 1.6 s the bg variant re-rolls — the dump maps Random(100) → 97/98/99 = imgs 440/441/442 (≈ a blink every couple of minutes), which read as *almost never*; per user intent the port makes the pips visible: ~15% of 1.6 s windows lit (440 often, 442 the brightest pop rarely) — labeled DELIBERATE deviation from the dump's odds.
- **Title static breathes gently** (group 1 's re-roll, damped): the original re-rolled coeff 50+Random(100) every 1.8 s; we keep the cadence but accent around the old fixed 0.61 (coeff 80+Random(70) → alpha ≈ 0.49..0.69) so the noise shimmers without stealing the show from the Freddy flap (user note: "it's Freddy who should blink, not the static").
- Version → v2.51.

---

## v2.50 — storage device picked once, up front (HDD auto-answer)

- **The storage device is now chosen ONCE at boot, before any message box** (`Progress::PrimeStorage`, called right after the first presented frame). Per the XDK docs, `XShowDeviceSelectorUI` silently returns the only suitable device when there is exactly one (an Xbox 360 Slim-era box with just the internal HDD — no MU slots): no UI, no mid-flow picker. Everything downstream (import, saves, achievements) then reuses the cached device id — the selector never runs right after a message box again, which was the whole ACCESS_DENIED-storm failure mode. The mount log now also prints the device id (`device=0x…`).
- Technical detail: the selector block moved from `XContentMount` into its own `PickStorageDevice()`; the build accidentally shipped broken mid-refactor once — the mount function header is restored and `Progress.cpp` reads clean.
- Real-HW verified (v2.50 run): prime picks the HDD silently (`device=0x00000001`), import + save/load round-trip through the profile container (`create=0x0 disp=0x2`, `flush=0x0 close=0x0`). A soft log line now covers the "freshly created container has no fnaf_ach.ini yet" read (ERROR_FILE_NOT_FOUND is expected there, not a failure).
- Version → v2.50.

---

## v2.49 — exact jumpscare timings from the event dump

- **Full kill-path audit of the office frame + the "freddy" kill frame** (all timings now documented in `docs/AI_MECHANICS.md` §9a). While at it, a dump-tool caveat was confirmed and recorded: jump actions there print the WRONG frame names (misaligned lookup) — the raw storyboard VALUE is the truth (office 0 / died 1 / freddy 2 / next day 3 / what day 4 / title 5 / wait 6 / gameover 7 / creepy end 14).
- **Scare durations now match the original exactly** (they were 2-2.5x too long): Bonnie 1.1→0.65 s, Chica 1.1→0.65 s (alt3 exit counters), Foxy 2.3→0.85 s (anim 52 to the end), Freddy 2.7→1.05 s (anim 65 to the end), the power-out dark face →0.85 s. Right after, the existing static burst + game-over flow continues.
- **XSCREAM onset matches the dump**: Freddy's office kill screams at scare-anim frame 7 (group 409 ≈ 0.233 s); Bonnie/Chica when the alt2 kill counter hits 1 — 9 ticks = 0.150 s (groups 228/229, was 10 ticks); Foxy, Golden and the power-out dark kill scream instantly, as before.
- **Golden Freddy aligns with the dump's "creepy end"**: XSCREAM2 and the forced close after 1.0 s (was 2.5 s). The close itself is still our deliberate deviation (the dump really does "End application" there).
- **Power-out now warns you**: after the music box cuts, Freddy's footsteps (`deep steps`) play for a short 1.5 s dark window before the dark-face kill — the "footsteps prepare the player" cue (documented FNAF behavior; the local dump has a plain black gap there, this addition is an explicit user choice).
- Version → v2.49.

---

## v2.48 — save-mount leak fixed; power-out gets the real kill screen and the music box; dev unlock-all writes the profile

- **The power-out kill is now the real one** (from the "freddy" kill frame, obj 152): pitch black with Freddy's dark face flickering — 21 full-screen frames (326, 307, 348, 308…325) at 36 FPS, first pass once, then looping from frame 5 (backTo 5). The old code wrongly reused the in-office lunge (anim 65). New sentinel `ANIM_FREDDY_DARK` rides the normal jumpscare state, so the hold-until-render rule, XSCREAM and the Game Over handoff behave as for the other kills.
- **The music box actually plays now**: at the face phase the real "music box" jingle sounds (dump groups 272/273) — the old callback looped *circus* there by mistake (`OnMusicBoxStart` played `Snd::CIRCUS`). No buzz / pitch-black interlude: when the jingle stage ends, the dark Freddy kill fires IMMEDIATELY and cuts the jingle mid-note (name-keyed stop, no fade anywhere — the original only ever hard-stops sounds or sets channel volumes instantly).
- **Scream timing per the dump (fixing "XSCREAM at scare start everywhere")**: Freddy screams when his lunge anim reaches frame 7 (~0.23 s in, group 409), Bonnie/Chica when their kill-pan lands (~0.17 s, groups 228/229/231); Foxy, Golden and the power-out dark face scream instantly (groups 323 / kill-path). The delay is driven off the render clock.
- **Save picture on the console**: the container now gets the title icon as its thumbnail — `XContentSetThumbnail` with `achievements_pics\game_icon.png` (64x64, 10 KB < the 15,616-byte PNG limit) right after the first successful mount per boot, so the storage device picker / content list shows the game icon instead of a blank tile. (The picker header's game icon itself comes from the embedded .spa.)
- **Save fix — the mounted-container leak**: when the in-container `fopen` failed after a successful mount, the root stayed mounted (the flush/close was skipped) and every later `XContentCreateEx` in the same boot answered 0x20 SHARING_VIOLATION / 0xB7 ALREADY_EXISTS (seen in the v2.47 HW log). Now StorageOpen unmounts immediately on that path, and the mount retries once after `XContentClose` when it sees those codes. The failing fopen also logs `GetLastError()` now.
- **Achievements: dev "Unlock all" now writes the profile too** (one batched `XUserWriteAchievements(10, …)`) — the Guide list then shows all ten with the embedded-SPA names/scores/icons, which is the one-glance verification that the config is picked up. Boot also logs the slot-0 sign-in state (`ACH p0 signin=N`: 0 = nobody, 1 = local, 2 = LIVE) so a silent no-profile run is visible.
- Version → v2.48.

---

## v2.47 — achievements wired to the official game config (spa.h / embedded .spa)

- **The achievement ids are now read from the generated `include/FNAF1-Recomp-proj.spa.h`** instead of hardcoded literals: the `kAchievements` table, `OnNightComplete`/`OnJumpscare` and the profile write all use `ACHIEVEMENT_ONE_NIGHT_AT_FREDDYS` … `ACHIEVEMENT_NO_HIDING`. The config (`FNAF1-Recomp-proj.xlast`) is compiled by XLAST and the resulting `.spa` is linked into the XEX as a resource section named by the title id (`464E4131`) — the console reads it at launch, so the Guide-side name/gamerscore/icon all come from it; there is no runtime "register the config" call to make.
- **"Four Nights at Freddy's" corrected to 50 G** per the config (the hand-rolled table had 30) — the in-game total is now 400 G. The old `achievements.xml` mirror is deleted: the .xlast is the single authority. The docs table updated to match.
- **Save-mask pinning**: ten `static_assert`s hold the ids at the contiguous 1..10 range — a regenerated spa.h with shifted ids fails the build loudly instead of silently re-reading an existing `fnaf_ach.ini` bitmask. The console has no achievement-visibility flag, so the in-game `secret` marks stay hand-picked (6/7/10); the `SPASTRING_*` ids in the header stay unused (the config's strings are placeholders) — filling them in the .xlast is the documented path to localizing the in-game list later.
- Version → v2.47.

---

## v2.46 — the animatronic brains audited against the events: tick-rate fix, Foxy/Freddy/door corrections, panning, Customize screen

- **Full AI audit (3 dump passes: frame 3 events + customize + title)**. Verified ALREADY matching the original (no work): opportunity timers 4.97/4.98/3.02/5.01 s (groups 188-191), the Random(20)+1 <= AI dice, the per-night AI tables 1-6 (305-310, incl. Night 4 Freddy 1+Random(2)), the 2/3/4 AM boosts (335-337), the full Bonnie/Chica room graphs (199-211/232-242 — room-for-room), the door retreat rooms (Bonnie→1B dining, Chica→4A), Freddy's 4B/4A rules (394/395), his wait for BOTH others to leave the stage (389), his 1000−100×AI delay (398), Foxy's tablet cooldown 50+Random(1000) (329/313), the 1500-tick lurk fallback and 100-tick run (315-320), the bang drain 10+50×N (324), and the per-night extra power drain 6/5/4/3 s (342-345 — already implemented).
- **THE root-cause fix — the game ran at HALF speed**: `tickDelta` was 1/30 while the frame accumulator gains 1/60 and every system inside is denominated in 60 Hz ticks → hours took 180 s instead of 90, power drained half-rate, all AI intervals twice as long. Now 1/60 (the application frameRate is 60).
- **Foxy sprint trigger corrected (group 40)**: the run starts when the player watches **CAM 2A** (the hall he sprints down) at progress 3 — the old code used the Cove; at stage 3 the Cove just renders the empty garage (groups 64/65).
- **Freddy inside the office (group 406)**: the 1 s kill roll now gates on `fox progress < 5` (Foxy's kill owns the night); entering his zone kills BOTH door lights (406/408-412, new AI_EVENT_FREDDY_IN_OFFICE) and starts the "whispering2" loop (group 405; stopped on the kill and at power-out). Watching Freddy's current cam now RESETS his delay counter (group 401) instead of the old monitor-down gate.
- **Doors mid-transition (groups 214/215/243/244)**: an animatronic at a door now waits while the door is mid-slide — enter only when fully open, retreat only when fully closed; the old code decided instantly on the analog amount.
- **Panning per the events**: the office view now STARTS at the pan clamp edge (looking toward the LEFT door — the follower spawns at X=640 of 640..960; 800=center exists only in the kill close-ups; the port started centered); stick speed is 120/300 px/s by deflection (2/5 px per frame, groups 83-88; was a flat 480); the camera-feed drift is the linear ping-pong with ~1.7 s end dwells ('screen follow 1', groups 2-12; was a smooth 18 s cosine).
- **Monitor static-out (groups 194-198, 219-222)**: when Bonnie/Chica move while their room is on screen, the monitor glitches — 300-tick static storm (feed hidden per group 197) with a Computer-Digital/garble sample.
- **The title's rolling band (the "blip flash 2" object)**: the subtle band rolling down the menu is now the real anim [430,435,436,434,438,439,437,22] at speed 10, visible in 1-in-3 six-second windows (groups 5-7) with the alpha flickering 0.22-0.61 every 1.6 s (group 4). Also the title static alpha tier re-rolls every 1 s (Timer 1000 = ms, group 14).
- **Customize screen (Night 7) implemented** (frame 12, groups 5-17): four panels/captions/arrows/numbers (the NIGHT 35x75 strip — the AI counters' own font) per the frame layout; console mapping: up/down = row, left/right = -1/+1 clamped 0..20, A = START, B = back; Night 7 in the menu now opens it, and START copies the four levels into the night (group 311). The 1987 combo (1/9/8/7) plays the creepy screen and returns to the title (the Golden Freddy kill keeps its force-close). Levels persist across a death retry within one run (the original's global counters).
- **Gamma/sRGB audit** (dark-tones hypothesis): no SRGB/gamma render states exist anywhere in the port — it is a naive 8-bit passthrough, exactly like Clickteam; the earlier "feeds look darker" measurement showed the original screenshot sits ON the art's own brightness (median 11-14 vs art 15), so no color conversion is needed; console-side output range (dashboard reference levels) is the remaining variable.
- **Camera static: fixed, slightly transparent** (user decision after on-HW testing): the group-13 flicker decode (0.10..0.41) reads as almost no noise on the dark room art, the full 0.61 was too dense — the camera now uses a dedicated fixed CAM_STATIC_ALPHA = 0.48 (the title keeps its 0.61). The event truth stays documented in docs/OVERLAY_MAP.md (v2.45 addendum) as an explicit labelled deviation.
- **Unlock chain verified & one gap closed**: beat night 5 → the $120 paycheck writes `beatgame` → the 6th Night row + star 1 appear (title reads the INI); beat night 6 → `beat6` is written right on the 6 AM screen (frame 6, night counter 6→7) → Custom Night row + star 2; beat Custom Night with 20/20/20/20 → `beat7` → star 3. New Game WIPES all three flags (a new save starts clean) — the port previously kept the stars; now it matches.
- Version → v2.46.

---

## v2.45 — camera static: the alpha is event-driven, not the serialized coeff 100

- **Real-HW finding (v2.44 console screenshot)**: the camera static drowned the feed — dense gray noise everywhere, while the original's monitor shows the room through a light breathing grain.
- **Root cause — event group 13**: it runs EVERY FRAME and act #65 (set alpha coefficient) feeds the static object (obj 42) its own alterable[0], which act #31 re-rolls as `150 + Random(50) + tier*15` (tier = Random(3), re-rolled on game start and every ~20 s by groups 15/14). The live coefficient is ~150..229 → alpha = 1 − coeff/255 ≈ 0.10..0.41 (avg ~0.25). The serialized ink coeff 100 (0.61) only applies before the first event tick — the old fixed 0.61 was 2-3× too dense. Cross-check: the title's "blip flash 2" is 95% transparent in-game despite opaque object data — only an event-set alpha coefficient explains it (same act #65).
- The dense white-out on every cam switch is separate (group 16 blip flash, opaque frames) and stays as is.
- The port now re-rolls the camera static's alpha every frame with the same formula (title static untouched — approved look, different ink effect).
- **CAM map buttons got their plates back**: in the frame data every map button is TWO stacked instances — a 60x40 plate (img_167 gray; img_166 green for the selected cam, hotspot 29,19 — the `PakMapButtonOf` table IS the plate instance table) UNDER the 31x25 white "CAM xA" text (img 165-177). The map outlines (img_164/145) carry no plates, so the port drew bare texts with no backing, and put the blinking plate OVER the selected cam's text (the name vanished under it). Now: all gray plates → the selected cam's green blink → all texts on top, like the original.
- Version → v2.45.

---

## v2.44 — HUD counter fonts decoded from the exe: clock / night / power / usage per the counter objects

- **The digit sets were WRONG**: CTFAK's application.json leaves every counter's picture list empty (`counter.frames: []`), so the port's strip-to-counter assignments were guesses. The real lists are decoded from the original exe's OBJ_INFO counter objects ("CNTR" system blocks; glyph order in every set: `0 1 2 3 4 5 6 7 8 9 - + . e`):
  - clock 'time of day' (objInfo 123 @ (1185,59)): **249, 252-264 (24x30)** — the old 457-470 strip actually belongs to the hidden 'lives left' counter;
  - 'night number' (objInfo 11 @ (1237,89), GLOBAL — the same object on the title menu): **187+191-203 (14x17)** — the old VAR14 372-385 belongs to the hidden 'not using tablet' counter;
  - 'power left 2' (objInfo 105 @ (221,646)): **52, 81-87, 123, 128, 146, 147, 148, 150, 186 (18x22, non-contiguous handles)** — the bold font of the original "Power left: 99%";
  - 'usage meter' (objInfo 108 @ (120,657)): displayType 4 — value 1..5 draws ONE whole-meter picture **[212, 213, 214, 456, 455] (103x32)**; the old tinted-rect cells are removed (the green/yellow/red colors are baked into the art).
- **The "12 ... AM" clock gap fixed**: `GetHourString()` returns "12 AM" and the old code measured the whole string — the " AM" tail (space, A and M are not strip glyphs) added ~57px of phantom width, pushing the hour digits far left of the AM image. Only the digit prefix is strip-drawn now.
- **Counters are right-aligned** per the frame data: clock cells end at x=1185 (beside "AM" at 1200, baseline y=59), power digits end at x=221 (baseline y=646), the night digit ends at x=1237 (baseline y=89 — one row with the "Night" word, which closes the v2.32/v2.35 position dispute: the inline look AND the dump anchor at once). img_208 "%" is a FIXED image at (224,632), not positioned relative to the digits.
- **Title menu**: the "Night N" digit is the same global counter → XSMALL right-aligned at (263,535) baseline, replacing the VAR25 guess (the 'loaded level' font). Power-out "0%" uses the same right-aligned power layout.
- Version → v2.44.

---

## v2.43 — import without a restart: the same boot reads the imported save (before the disclaimer)

- **Real-HW finding (v2.42 run)**: after a message box closes, XAM keeps refusing ANY new system UI for a long while — the device selector stayed `ACCESS_DENIED` through all 40 retries (10 s) and the "Import complete" box was denied too. Back-to-back system screens are a dead end on this console.
- **Import flow reworked**: question box (the only system UI in the flow) → Yes → import → **no second box, no dashboard exit** — the import lands BEFORE the disclaimer and `RefreshMenuFromProgress` reads the fresh progress in the SAME boot. The "Import complete. Please restart" box is removed entirely (it was never needed: the read happens after the import by boot order).
- **Selector cooldown**: once the device selector exhausts its retries, subsequent `StorageOpen`s in the same boot skip the UI (60 s cooldown) instead of stalling 10 s each — the import boot stays fast; the selector is retried again a minute later.
- Verified on real HW: with the loose save already imported and deleted, boot is clean — no prompt, and the progress + achievements are read from the fallback storage (`SAVE: storage = local fallback` ×2, `Achievements: unlocked=0x3FF`).
- Version → v2.43.

---

## v2.42 — import chain: device-selector retry after the box, honest storage logs

- **Frame pump verified on real HW (v2.41 run)**: the import question box now appears and completes (`XuiSceneCreate` hooked → `res=0x0 btn=1`) — the "sound only, no window" bug is gone.
- **New finding — back-to-back system UIs reject each other**: the device selector, called right after the question box was dismissed, failed with `ERROR_ACCESS_DENIED` (XAM was still tearing the previous screen down — the unhook lines land mid-import). The import silently went to the local fallback while the log claimed `-> fnaf_save:\freddy`, and the "Import complete" box stayed busy for the whole 5 s retry window.
- Fixes: the device selector now retries up to 40 × 250 ms (10 s) on `ACCESS_DENIED` (same pattern as the message box); the message-box retry budget is raised 20 → 40; `StorageOpen` logs which backend actually served the file (`SAVE: storage = XContent|local fallback`); the import log no longer hardcodes the destination.
- Version → v2.42.

---

## v2.41 — the import prompt works; the "hang" was the storage-device picker; v2.40's cap removed

- **ROOT CAUSE of "sound only, no window" — the frame stream must keep flowing while the box is pending**: the system box is asynchronous and XAM composites it over the title's frame chain; a stalled stream (the v2.35+ bare `Sleep` wait) leaves the box nothing to composite over. `SysPrompt` now pumps a black Clear+Present frame on every iteration of the wait. This is also why the box worked in the v2.34 era: the software fallback unintentionally kept frames flowing; removing it in v2.35 stalled the stream. No timeout cap (the v2.40 cap is gone), `ret`/`res` logging and the `IMPORT:` step logs stay.
- **UI-busy retry**: `XShowMessageBoxUI` returning `ERROR_ACCESS_DENIED` (another system screen owns the display, e.g. right after boot) is retried up to 20 × 250 ms instead of failing the prompt instantly.
- **Heartbeat while waiting (v2.41)**: `XMB: waiting N s for the box...` every 5 s — the Sep-18 boots (profile signed in) showed the box AFTER 20 s, which read as "sound only, no window"; the heartbeat proves the wait is alive and measures the actual cold-start time per boot.
- **What actually read as a hang**: after "Yes" the import path calls `XShowDeviceSelectorUI` — a FULL-SCREEN system picker waiting for the player to choose a storage device. Unlogged, it looks like a freeze. Now loud: `XContent: device selector open (pick a storage device)` before the pump, `XContent: device selector res=0x…` after.
- **Import UX (documented)**: boot → "Import save?" box → Yes → storage-device picker (choose the HDD) → import runs → "Import complete" box → OK → exit to dashboard for a fresh restart.
- Version → v2.41.

---

## v2.40 — import hang guard: a message box that never completes can't freeze boot anymore

- **Real-HW repro (with a signed-in profile this time)**: the boot import flow printed two `XMB: ret=0x3E5` (ERROR_IO_PENDING) and froze — a system box that is never presented/completed leaves the old `while(!XHasOverlappedIoCompleted) Sleep(16)` spinning forever. The docs only guarantee completion on a button press; nothing guarantees the box appears at all.
- **`SysPrompt` hang guard (v2.40)**: a 20 s cap on the pending wait (~2× the documented XUI cold-start of ~10 s, so a slow first box is NOT falsely tripped). On expiry: `XMB: TIMEOUT 20s (box never completed)` + the call is treated as CANCELLED → the caller's safe default applies (import question → "No", so boot proceeds without importing; pak-error → dashboard).
- **Import flow now logs every step** (`IMPORT: loose freddy found / YES -> importing / done -> restart / FAILED / NO`): the next real-HW log pinpoints the exact branch instead of a silent stop.
- XContent canon itself is verified on real HW (v2.39 log): `user=0 create=0x0 disp=0x2 (OPENED_EXISTING), flush=0x0, close=0x0` with a signed-in profile, `ACH UI -> 0x0` — the storage round-trip per docs works.
- Version → v2.40.

---

## v2.39 — saves/achievements per the XDK canon

- **Canon XContent flow (docs section in `docs/SAVES_XCONTENT.md`)**: `XShowDeviceSelectorUI` (overlapped) → `XContentCreateEx(signed-in gamer index, "fnaf_save", XCONTENT_DATA{DeviceID, SAVEDGAME, displayName, szFileName}, CREATEALWAYS)` → plain `fopen` inside the mounted root → `XContentFlush` → `XContentClose` (close must succeed for the write to count).
- **User index is bound to the FIRST player** (`PickSignedInUserIndex`): the original ties its save/achievements to player 1's profile (gamer index 0 — the boot import round-trips through that profile), so we use slot 0 only, no scanning. With nobody signed in at slot 0, XContent would fail with the documented `ERROR_ACCESS_DENIED`, so we skip it entirely and use the v2.38 local fallback; the achievements write is gated the same way (local `fnaf_ach.ini` still counts).
- **Loud diagnostics**: `XContent: user=N create=0x… disp=0x…` and `XContent: flush=0x… close=0x…` in the debugger log (0 = OK).
- Version → v2.39.

---

## v2.38 — storage fallback: XContent down ⇒ plain HDD files (fixes the "fresh game after game over" + "ACH save FAILED")

- **Root cause (real-HW logs)**: the system build's XContent storage is unusable on this RGH (no signed-in profile) — `ACH save FAILED` and, worse, the 6 AM save never persisted, so any death/game-over (incl. from the DEV jumpscare) went through `RefreshMenuFromProgress` → `Progress::Load` → read failed → the title showed a fresh game. Exactly the reported bug.
- **`StorageOpen` in the system build now FALLS BACK to a plain HDD file** when XContent can't mount or the container file open fails: writes/reads `game:\save_fallback\<file>` (`freddy` + `fnaf_ach.ini`). `StorageClose` only unmounts when XContent was actually used. The fallback dir deliberately differs from `game:\save\` (Live Safe + import candidate) so boot never offers to import our own save. Live Safe is untouched.
- Effects: the save re-read after game over works everywhere; `ACH save FAILED` is replaced by a real write on RGH; achievements persist locally on such setups (the profile-side XUserWriteAchievements write is separate and still needs the SPA for the Guide list).
- Version → v2.38.

---

## v2.37 — pad rumble (two motors) + Foxy scare safety/diagnostics

- **Rumble service** (`RumbleKick(left, right, seconds)` + `TickRumble`, linear decay, safety cap 5 s, zeros when idle): uses `XInputSetState(0, …)` — both motors, mapped per the recommended card:
  - Jumpscares (all, incl. Golden Freddy): **both motors at max** for the whole scream (~1.4 s);
  - Foxy door bang: strong **left-motor thump** at the power-penalty moment;
  - Air-lock door close (only on close): dull left-motor push;
  - Freddy's nose honk (Y in the office): micro **right-motor** click in sync with the honk;
  - Freddy's steps/laugh: faint low-freq pulse per move/laugh;
  - Power at 0%: dry fading **right-motor** crackle at the cut, then dead silence.
- **Foxy jumpscare safety**: the `JUMPSCARE → GAME_OVER` handover is now held until the RENDER clock (`scareElapsed`) also passed the scare duration — the logic-timer-only exit could cut the scare short on frame drift (the reported "plays ~1 in 3"). Golden Freddy is excluded (its own exit(0) path).
- **Diagnostics** in the debug console: `SCARE <name> begin`, `SCARE end: rend=X/Y` (shows if a scare was ever truncated), `FOXY bang #N door=open|closed` (tells the "1 in 3" apart from the correct no-scare-while-door-closed behaviour).
- Version → v2.37.

---

## v2.36 — FNAF1 at ~99.5%: the last three easter eggs

- **Rare Pirate Cove "IT'S ME" sign** (dump groups 64/65 + 348): at Foxy stage 3 the feed now splits on the shared `random for pic` roll (1..100, re-rolled on every monitor drop) — `> 10` → the gone pose (240), `<= 10` → the rare sign (img 553, `CAMFEED_1C_ITSME`, was defined but never drawn).
- **Post-Game-Over creepy screen 1/10000** (dump frame 8 groups 2/3/5): when the backroom ends the game rolls `rand()%10000 == 0` and, on a hit, shows the f14 "creepy start" face (~2.5 s, SILENT — no force-close, per the dump it just ends on the title) instead of going straight to the menu.
- **Door/light button jam** (dump groups 97/101/107/109): while an animatronic stands in the doorway (Bonnie on the left zone, Chica on the right — `IsAnyAnimatronicAtDoor` now finally has callers), door/light clicks play only the "error" stinger and do nothing. Freddy's right-door arrival is separate in the dump and is not jammed.
- Version → v2.36. FNAF1 is **~99.5%** — the remaining half-percent: **achievements and XContent saves not yet verified on the console** (the ACH writes log via the debug console v2.35, but the RGH profile-side result and the XContent save round-trip are unconfirmed).

---

## v2.35 — message boxes: software fallback removed (the box was fine)

- Real-HW log proved the v2.34 watchdog was wrong: `XMB: ret=0x3E5` (= ERROR_IO_PENDING — the box opens), then a ~10 s delay until `Hooked: 'HUD: XuiSceneCreate'` — that is XAM **cold-starting its XUI on the FIRST call**; the window itself appears fine afterwards.
- Reverted to the plain blocking wait (no watchdog, no `SoftPrompt`): `SysPrompt` keeps the return-code logging (`ret`/`res`/button) and every box routes through it (import question/done, missing pak, fatal). The one black frame presented before the first boot box stays (harmless).

The Xenia caveat is accepted: Xenia does not render the XMB; if emulator testing needs prompts later, a fallback can return behind a build flag.

- **Start+B DEV combo made pad-robust**: the pad reports the two edges a frame or two apart, so a lone Start fell into the pause handler and booted the game back to the title. A ~330 ms window after a B edge now accepts the Start edge (same-frame still works).
- **Clean exits to the dashboard**: `exit(0)` tears the XDK process down abruptly (kernel threads dying with code 0). `ExitToDashboard()` = `XLaunchNewImage(NULL, NULL)` now ends the missing-pak screen and the import self-close; **Golden Freddy intentionally keeps exit(0)** (it mirrors the original's abrupt close).
- **Golden Freddy flow restored (DEV item 4 now runs the REAL pipeline)**: the old DEV test drew the CAM 2B pose over the camera view ("just a camera frame"). It now spawns the actual sequence — giggle, he appears IN THE OFFICE (img 573), **IT'S ME flashes accompany the visit** (~0.2 s every ~1.1 s, the wiki's hallucination phase), the full-screen face (f14) and the intentional close. READMEs document the "crash" as a safe feature.
- **FNAF1 HUD: the in-game night number in the top-right reads as ONE row** — right after the "Night" word (x1217, y74). It was drawn at the raw counter anchor (1237,89), hanging below-right of the word — the "crooked" look. The "12 AM" clock keeps its row (AM at (1198,31), digits right-aligned to it); the menu's "Continue → Night N" position was already inline (unchanged).
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
