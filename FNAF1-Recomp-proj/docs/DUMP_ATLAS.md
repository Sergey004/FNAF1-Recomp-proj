============================================================================
FNAF1-Recomp -- docs/DUMP_ATLAS.md
The FNAF1 (v1.132) dump, completely mapped (v2.52)
============================================================================

Companion docs: AI_MECHANICS.md (AI/power flow), CAMERA_FINDINGS.md
(viewing-id evidence), TABLET_FLIP.md, OFFICE_FX.md, OVERLAY_MAP.md,
SAVES_XCONTENT.md. This file is the "map of the map": read-dump literacy,
the corrected frame graph, every frame's layout+logic, and the object /
animation / sound tables.

============================================================================
0. HOW TO READ THE DUMP (literacy rules)
============================================================================

Timer values are raw Clickteam milliseconds. The printed "(~Ns)" annotations
are a 50 Hz-scale artifact of the event exporter and read ~20x off;
`timer 1000` = 1 s. A Timer DO "cond #-8" fires EVERY N ms; "cond #-7" is a
ONE-SHOT N ms after frame start.

Jump actions print WRONG frame NAMES (the exporter's name lookup is
misaligned) but correct raw VALUES. The value addresses the storyboard slot;
the storyboard's physical chunk order is

    slot -> frame:  0=office("Frame 1")  1=died  2=freddy  3=next day
                    4=what day  5=title  6=wait  7=gameover 14=creepy end
    (remaining named chunks: 8..13,15,16 follow their printed names when
    reached by name-free "next frame" steps.)

So: "DO Game Jump to frame value N -> (WRONG NAME)" — trust N. Validated
against the known flows (6 AM -> 3 = "next day"; night card -> 6 = wait ->
0 = office; Golden -> 14 = creepy end; gameover -> 5 = title).

Condition cheat sheet (c = "cond", a = "act"):
- System -1  "every tick" (60 fps app),  System -7  "edge: once when this
  becomes true",  System -6  "once per true-streak" (hard latch),
  System -3  random comparison (expr tail = Random(N)+1 vs V),
  Game  -1   once at frame start;
- Timer  -8  every N ms,  Timer -7  one-shot at N ms;
- Keyboard -1 key pressed, -2 key held, -4 mouse hovers object, -7 clicked
  object, -5 mouse clicked anywhere;
- Active: cond -2 "animation == V finished"? (door/kill anims), -3 "current
  animation == V" (continuous), -27 alterable[n] cmp V, -29 "is visible",
  -4 overlapping (other=0x01 -> NOT overlapping);
  acts: 1 set position rel to, 2 set X, 17 set animation, 24 destroy,
  26 hide, 27 show, 31 alterable := V, 32 add, 33 subtract, 65 set
  animation speed (value 0..255... "speed" as ink-style coefficient);
- Speaker acts: 1 stop ALL, 11 play sample on channel, 12 loop sample on
  channel, 17 set channel volume (0..100);
- Counter acts: 80 set, 81 add, 82 subtract.   Counter cond -81 compare;
- Expression tokens: System(type=-1,num=1)=Rand(, type=-1,num=-1) = -(,
  num=-2 = ), num=2 = +, num=4 = (, num=6 = *, num=8 = /.

Animations: eff FPS = speed x 0.6 (60 fps app). "repeat 1" plays to the end
once; "repeat 0" loops; backTo N loops from frame index N.

============================================================================
1. THE CORRECTED TRANSITION GRAPH (all 17 frames)
============================================================================

    Frame 0 "Frame 17"  (boot/disclaimer)
        +2000 ms ---(or Enter/click immediately)---> 1 title
        Esc -> end application

    Frame 1 "title"
        Esc -> end
        Menu confirm latch (Active 6): alt0 = 1 New Game / 2 Continue /
        3 6th night / 4 custom; alt1 counts 20 ticks (~0.35 s) then fires:
          alt0==1 -> value 9  -> frame 10 "ad"   (the help-wanted newspaper)
          alt0==2 -> Next frame -> frame 2 what day (Continue: no ad)
          alt0==3 -> Next frame -> frame 2 what day (6th night)
          alt0==4 -> value 11 -> frame 12 customize
        Esc -> end; Delete held 1 s wipes beats/level; C+D+1/2 unlock;
        and a 1-in-1000 one-shot roll at frame start -> value 13 =
        frame 14 "creepy start" (the famous rare title egg — silent dark
        face ~10 s, then back to the title).

    Frame 2 "what day" (night card)
        shows 12:00AM card per night (groups pick the card image by
        "night number"), blip3; after 130 ticks (~2.2 s) -> 6 next day? NO —
        actual: its own alt0 timer -> value 6 = wait (black hop) -> 0 office.

        (DUMP NAME WARNING: the office frame's "Jump to frame value 2" prints
        as "what day" but actually lands on 2 = "freddy", the kill screen.)

    Frame 3 "Frame 1" (THE OFFICE / gameplay)
        -> 1 title (?)     — none in normal play (death goes to died)
        -> value 1 died    — Bonnie/Chica/Foxy/Freddy office kills
        -> value 2 freddy  — power-out kill (groups 301/302)
        -> value 3 next day — "time of day"==6 (group 303)
        -> value 14 creepy end — Golden Freddy sit-out (group 422)
        Esc -> end; expression jumps 151/152/182 = frame self-restart on
        camera flips (see S4).

    Frame 4 "died" (post-kill noise, the in-office kills' interstitial)
        start: stop all, "static" ch1 @100; 10000 ms -> value 7 = gameover.
        Esc -> end.

    Frame 5 "freddy" (power-out kill screen)
        start: stop all + XSCREAM ch1 @100 (the dark Freddy face, obj 152,
        plays over 21 frames, loops back to frame 5); when the anim settles:
        static ch1 + blip flashes; 12000 ms -> value 7 = gameover. Esc -> end.

    Frame 6 "next day" (the 6 AM sequence)
        start: stop all, "chimes 2" ch1 @100; the 5->6 AM clock roll artwork;
        the crowd cheer when it lands; night number +1 and the INI writes
        (level saved, beat6 on beating night 6, beatgame/beat7 paths).
        ~200 ticks celebration, then exit by the (advanced) night number:
          night <= 5 -> value 4  -> frame 2 "what day" (next night's card)
          night == 6 -> value 10 -> frame 9  "the end"   (paycheck)
          night == 7 -> value 12 -> frame 11 "the end 2" (night 6 paycheck)
          night == 8 -> value 8  -> frame 13 "the end 3" (custom 20/20/20/20)
        (every "the end" frame: stops all, loops the music box ch1, holds
        15 s, then -> value 5 = title.)

    Frame 7 "wait" (100 ms black hop) -> value 0 = office (frame 3).
    Frame 8 "gameover" — stop all; 10000 ms -> value 5 = title; per-second
        1-in-10000 roll -> 14 creepy end. Esc -> end.
    Frame 9  "the end"   — paycheck / music box loop on ch1; 15000 ms -> value 5 = title.
    Frame 10 "ad"        — 5000 ms or click/Enter/Esc -> value 4 = what day.
    Frame 11 "the end 2" — same body as 9 (music box loop) -> value 5 = title.
    Frame 12 "customize" — AI editors (arrows clamp 0..20), START
        ("Active 2", objInfo 169) -> value 4 = what day (Night 7 starts);
        1987 combo (freddy=1, bonnie=9, chica=8, foxy=7) arms "1987" counter
        (13) and any of the four !='s disarm it (14-17); with 1987 armed,
        START -> value 14 = frame 15 "creepy end" (XSCREAM2 + app close).
    Frame 13 "the end 3" — = 9/11 body (music box loop) -> 5 freddy.
    Frame 14 "creepy start" — stop all, silent dark face; 9500 ms shows the
        eyes pair; 10000 ms -> 5 freddy.
    Frame 15 "creepy end" — stop all, XSCREAM2 ch29 @start; 1000 ms ->
        End application.
    Frame 16 "end of demo" — music box one-shot; 15000 ms -> 5 freddy.

    (The port deliberately does NOT chain the 10-15 s gameover->freddy roller;
    our game-over state shows art directly. See README/deviations.)

============================================================================
2. PER-FRAME DIGEST
============================================================================

(The office frame's 435 groups get their own section below — SS3.)

### Frame 0 "Frame 17" — boot disclaimer
Objects: Text 'WARNING! ...' (544x259), Active img605 (the "flashing lights"
rendered art) at hotspot (426,249). Timer 2000 ms -> title, or Enter / click
-> title; Esc quits. Fade in/out 1010 ms.

### Frame 1 "title" — 67 groups
Layout (FrameLayout): bg = Active 2 (431 base), static (18, ink9), blip
flash 2 (430, ink1), logo Active (444 @172,68), menu words (new game 448,
continue 449, 6th 443, custom 526), stars 432 x3, "v 1.132" String, the
option marker Active 4 (450), Ini extension.

Groups:
- 1  every 90 ms: static anim-speed coeff := 50+Random(100)  [the noise
     transparency pulse; port v2.51 narrowed to 80+Random(70) per user]
- 3  start: stop all; static2 ch1, darkness music ch2 loop; ch1/2/3 vol 100.
- 4  every 80 ms: Active 2 alt0 := Random(100); blip flash 2 coeff :=
     Random(100)+100 (0.22..0.61 visible alpha).
- 5  every 300 ms: Active 2 coeff := Random(250); blip flash 2 alt0 :=
     Random(3) (band visibility window 1-in-3 rolls into 6/7).
- 6/7 band shown only while alt0==1.
- 8/9/10/11 Active 2 (the Freddy bg) anim: 99->14(442), 98->13(441),
     97->12(440), else 0(431).
- 12/13 up/down arrows move "option selected" (0..3); clamps 14-19 by
     content lockedness (6th-night alt0, star 2 alt0):
     no-6th: cap 1; 6th but no star2: cap 2...
- 20-23 Active 4 marker positions per option (offsets -150/-165/-192 rel
     each button).
- 24-27 CLICKS: New Game (Ini level=1, voices 1-5 := 0, Alt6=1); Continue
     (Alt6=2); 6th night (Alt6=3, night=6, needs 6th alt0==1); custom night
     (Alt6=4, night=7, needs alt0s). (The Enter-key twins 33-36 do the same.)
- 28 start: Active2/alt of 6th-night obj := Ini beatgame; star 2 := Ini
     beat6; star 3 := Ini beat7 (drives hiders 45-48/66-67).
- 29-32 option selected edges: write "night number" (0->1, 1->Ini level,
     2->6, 3->6) + blip3 ch3; 30 also reads Ini lives into tiny lives.
- 37-41 Active 6 (the play/confirm latch): alt0>0 -> alt1 += 1/tick; >20 -> ;
     alt0==1: jump 9 ("the end"... per the misprint — actually the start path);
     ==2: Next frame (what day); ==3: next frame; ==4: jump 11.
     (Confirm lag is 20+ ticks ≈ 0.35 s.)
- 42/57 alt0==2 (Continue) with lives<=0 -> night := 1.
- 43/44 night==1 -> option := 0 (New Game parked); >1 -> option := 1.
- 49 Delete held, every 1000 ms (once): beatgame/beat6/beat7 := 0,
     level := 1, night := 1, stars hidden, blip3 ch1.
- 50 DEMO flag entry wipe (demo legacy).
- 51/52 option==1: show night number/word + tiny lives/man; else hide.
- 53-56 hover -> jump selection to that row.
- 58 option==1 & number>5 -> clamp 5 (the "level<=5" rule of the office
     reload path).
- 59 start: lives left := 5 (template junk — the scare doesn't use lives).
- 60/61 dev: C+D+1 -> 6th-night unlock; C+D+2 -> +star 2 (demo!=1).
- 62 start: 1/1000 one-shot roll -> jump 13 ("the end 3" — the post-credits
     style screen; the rare title-event easter egg).
- 64 DEMO: night>2 clamps to 2. 65 night==0 heals to 1.
- 66/67 star 3 show/hide.

### Frame 2 "what day" (night card)
Layout: blip flash (23), night number counter, big number Active (453
"12:00 AM" family), Active 3 (480), Active 2 (538). Start: stop all, blip3,
create blip flash; Active anim := per-night card value (0/12..17 by
"night number"); Active2 alt0 += 1/tick; >130 (~2.2 s) -> value 6 = wait.
(noise band: see title.)

### Frame 7 "wait" — 100 ms black hop -> value 0 = the office (frame-restart
### hop used by the night card).
### Frame 4 "died" & 5 "freddy" — static noise + XSCREAM flavor (graph above).

### Frame 6 "next day" — the 6 AM celebration
Layout: "5" (350) Big-clock art (352/351), night number, AI counters,
DEMO?, Ini;  groups: start -> chimes ch1 + vols; alt0-window: Active3
alt1 += 1/tick; >200 -> exits by night; plus INI activity-record writes 12/13
(beat6 on night==7, beatgame/beat7 paths) and night number +1 (act 81).

### Frame 8 "gameover"
Backdrop 358 + "Game Over" string; start stop-all; per-1 s: random roll
(o163) 1/10000 -> creepy end; 10000 ms -> freddy (the noise roll); Esc end.

### Frame 12 "customize"
Arrows per character: right acts +1 clamp <20 (groups 5-8), left acts -1
clamp >0 (9-12); START (the big "Active 2"/img530 button) -> value 4 =
frame 2 "what day" (night 7 kicks off through the night card); the 1/9/8/7
combo arms "1987" (groups 13-17); with 1987 armed, START -> value 14 =
frame 15 "creepy end" (XSCREAM2 + End application) — same deadly exit as
Golden's sit-out. Fade-in 560 ms.

### Creepies 14/15 and the end-chain 9/11/13/16 — table in S1.

============================================================================
3. OFFICE FRAME (frame 3 "Frame 1") — reference tables
============================================================================

The 435-group logic lives in AI_MECHANICS.md sections 1-11 (+ §9a kill table).
Here are the two hard-to-find tables.

### 3a. Alterable register map (who stores what)

counter objects: option selected=9, night number=11, ghosts; alterables are
per instance of these Actives —

Active 2 (oi 43, "the office brain"):
  alt0 = cam-static sound pick 1..4 (194/195 -> 219-222 garbles ch5)
  alt1 = fan tier roll Random(3) every 1 s (14)
  alt2 = scream fuse (10 at kill-arm 225/231; -1/tick 228; ==1 -> XSCREAM ch9 229)
  alt3 = death fuse (40 at kill-arm; -1/tick 262; ==1 -> died 263)
  alt4 = breaths pick 1..4 (276/278 -> 280-283)
  alt5 = power-out face roll Random(4)+1 every 50 ms (289)
  alt6 = power-out phase 0->1->2->3 (272/291/297 exit rolls)
  alt7 = buzz strobe Random(2)+1 per tick in phase 2 (293)
  alt8 = buzz counter (def at 298 ends phase at 20 ticks)
Active 3 (oi 44, scene renderer / scare player):
  alt2 = door-light flicker roll Random(10)+1 every tick (122)
  alt5 = poster variant roll Random(30)+1 every 50 ms nights>=4 (388)
bonnie (oi 111): alt0 move coin; alt1 settle-timer 10; alt3 pose 1/2;
  alt18 windowscare latch.
chica (oi 120): same + alt5 = Foxy run timer (315-317), alt6 = Foxy unseen
  timer (318-320), alt12 = Foxy tablet cooldown (313/329),
  alt14 = "Foxy is inside" (323; blocks LEFT door/light), alt15 = bang count.
freddy bear (oi 139): alt6 = killing latch (406); alt12 = move state
  0/1/2 (190/398 + path steps consume); alt13 = pending delay counter.
freddy got in (oi 140): alt6 = laugh selector.
you (oi 109): alt0 = cam static-out 300-tick fuse (195-198).
doors (oi 59/60): alt0 = 0 open / 1 closing / 2 closed / 4 opening.
control room follow (oi 55): alt0 cams-up, alt1-2 doors, alt3-4 lights
  (feed usage meter, 175).
screen follow 1 (oi 41): alt0 feed pan phase, alt1 step counter.
Active 21 (oi 145, IT'S ME): alt0 armed, alt1 decade gate, alt2 show fuse.
yellow bear (oi 146): alt0 0/1/2 (poster arm/in-office), alt1 sit fuse 300.
mute call (oi 137): alt0 0 idle / 1 playing / 2 muted.

### 3b. Speaker channel map (frame "Frame 1")
ch1  fan (Buzz_Fan) / powerdown one-shot | ch2 ColdPresc B / ambience2 /
     buzz-window blips | ch3 BallastHum (door lights) | ch4 door motor
     (SFXBible) | ch5 cam-static garbles | ch6 MiniDV cam-up | ch7 monitor
     raise/put-down | ch8 deep steps (Bonnie/Chica hops, 10..40 by proximity,
     muted on watched room) | ch9 blip3 / XSCREAM / knock2 / windowscare /
     honk | ch10 kitchen clatter (0/10/20/75) | ch12 error | ch13 pirate
     song2 (5 office / 15 cove) | ch14 breaths (50) | ch15 circus (5) |
     ch16 Freddy/Foxy laughs & run | ch18 EerieAmbience dread (0/30/50/75/100
     by corner occupancy) | ch19 phone call (100 office / 50 cams / 0 muted) |
     ch20 rare pounding | ch21 robotvoice (0, door mumble 1+5r/1+20r,
     ITSME 100) | ch22 music box (kitchen Freddy) | ch24 running fast3
     (Freddy steps, 30..100 by path) | ch25 whispering2 | ch27 Golden
     poster laugh | ch30 music box (power-out jingle).

No fades anywhere — only instant volume sets and stop-all.

============================================================================
4. OBJECT / ANIMATION / SOUND DATABASE (from application.json + banks.json)
============================================================================

### 4a. Frames/metadata
17 frames, 1280x720 each (office is 1600x720, 4 layers); header: 60 fps,
build 284 (Fusion 2.5), win 1280x720; extensions: Perspective (oi 40),
Ini (oi 10), Date & Time (oi 151).

### 4b. The hot objects (anims worth knowing)
- oi 44 Active 3 (office scene/kills) — 65 anims; the kill anims:
  A35 Bonnie 11f spd75  A44 Chica 16f spd99  A51 Foxy-run 33f spd65 bk31
  A52 Foxy 25f spd50  A65 Freddy 31f spd50  + one-frame poses for every
  door/light/cam picture (0, 12..75).
- oi 152 'Active' (power-out kill on frame 5) — A0 spd60, 21 frames
  [326,307,348,308..325], backTo 5.
- oi 145 Active 21 (IT'S ME / Golden overlay) — A0 spd75 [525,543,520,544].
- oi 146 yellow bear (Golden in office) — img 573.
- oi 4 Active 2 (title bg) — A0=431, A12=440, A13=441, A14=442.
- oi 2 static / oi 12 blip flash 2 (title) / oi 35 blip flash (transitions).
- oi 59/60 doors — close/open 12-frame slide anims (A12 fwd, A14 back).
- cam buttons (oi 75..95) — gray 167 / green 166 pairs.
- oi 188 img 593 — customize hint strip; customize plates 537 x4.

### 4c. Sample table (bank handle -> name)
 #15 XSCREAM   #46 XSCREAM2 (Golden)   #30 music box   #9 deep steps
 #11 blip3   #10 run   #27 knock2   #26 powerdown   #20 static
 #34 static2   #35 darkness music   #21 pirate song2   #29 circus
 #38 Laugh_Giggle_Girl_1 (Golden poster)   #56/57/58 = 1d/2d/8d Freddy laughs
 #59 whispering2   #55 running fast3   #31 windowscare   #32 chimes 2
 #33 CROWD crowd cheer   #36 PartyFavor honk   #37 Eerie ambience
 #40 robotvoice   #39 DOOR_POUNDING   #41-45 voiceover1c..5 (phone calls)
 #8 COMPUTER_DIGITAL + #12/13/14 garble1-3 (cam static blips)
 #0 ColdPresc B   #1 BallastHumMedium2   #2 Buzz_Fan_Florescent2
 #3 SFXBible_12478 door motor   #4 error   #5 MiniDV cam-up
 #6 CAMERA_VIDEO_LOA monitor raise   #7 put down
 #22-25 breaths (4)   #16-19 oven clatter (4=5 sounds)   #28 ambience2
(Full byte lengths/rates in pak_manifest.json.)

### 4d. Absent handles are pack art: 388-393, 539, 589, 591, 606-627, 629 —
gaps in the image bank (not "missing", the bank skips them).

============================================================================
5. QUIRKS & JUNK (do NOT cargo-cult)
============================================================================
- "lives" lives-left/tiny-lives/tiny-man are Clickteam-template leftovers —
  the real FNAF1 has no lives. The dump still writes "lives" to the Ini from
  the kill groups (346/347) — harmless.
- "not using tablet" (G312) and "test" (G314) are dead debug writes.
- G290 == G299 (identical power-out group, both fire).
- G4 vs G5: near-identical office show/hide (one kill-guarded, one not).
- DEMO?: full-game build leaves it 0; the demo-watermark/"end of demo" frame
  is legacy.
- Counter initial values aren't exported by the dumper (JSON has none) —
  runtime defaults come from events (e.g., power left := 999 on start).
- Fonts: not exported anywhere in the dump.

============================================================================
6. Where each *current* port behavior deviates from this dump, on purpose
============================================================================
- Golden Freddy kill sound: XSCREAM2 played by OUR scare path at the creepy
  screen (dump: the screen's frame start does it — same net effect).
- Power-out: our build collapses jingle -> steps window (wiki-sourced,
  user-approved) -> immediate dark kill; the dump's buzz/black gaps (20-frame
  flicker then 2-20 s black, per §3a alt6/alt7/alt8) are documented here for
  reference only.
- v2.51: the title bg lamp flicker runs more often than the dump's 1%-roll
  (user-approved); the static alpha pulse dampened.

v2.53 audit alignment wave (the rest IS the dump now): New Game keeps the
unlock flags (only the Delete-hold cheat wipes); the Golden arm is the
dump's silent global 1/100000/s roll; Freddy's 4A->4B step needs the right
light off; door/light inputs are monitor-down-gated with the 10-tick click
cooldown and lights are mutually exclusive; the step-mute, dread ladder,
robotvoice corner law, kitchen view ladder, the 4 s clatter, the 1/1000-per-
second ITSME arming, the Foxy sprint one-shot at 3->4 shown on the 2A feed,
power formula ceiling 6, door billing at settle, and the 535 s night are in.
Saves: Continue keeps night 6/7 by choice (dump caps the saved level at 5 —
a deliberate console-UX deviation).
============================================================================
