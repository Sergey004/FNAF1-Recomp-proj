============================================================================
FNAF1-Recomp -- docs/FNAF2_MECHANICS.md
FNAF2 office mechanics as decoded from the events (v2.58 plan basis)
============================================================================

Source: FNAF2 dump "Five Nights at Freddys 2", Events/frame_3_Frame 1_events.txt
(751 groups / 8720 lines), cross-checked with the other frames'
TRANSITIONS + FrameLayout. Companion: docs/DUMP_ATLAS.md (FNAF1 rules).

READING RULES (same as FNAF1): timers are RAW ms (the "(~Ns)" annotations
are a 50 Hz artifact); negated conditions carry other=0x01; act 65 =
alpha coefficient; per-frame counter/alterable decrements = 60 fps.
FNAF2 jump actions print CORRECT names (no FNAF1 name-print bug).

============================================================================
0. THE BIG PICTURE
============================================================================
- 11 AI characters: old Freddy/Bonnie/Chica/Foxy, toy Freddy/Bonnie/Chica,
  Mangle, Balloon Boy, the Puppet, Golden Freddy (+ paper pals non-lethal).
- No doors at all. Defense = the Freddy mask, the flashlight, and the
  music box. The monitor is both your tool and your weakness (several
  animatronics enter the office ONLY while it is up).
- Death = jumpscare on the office frame itself, then the night restarts
  IN PLACE. There is no game-over screen in the real chain (the "gameover"
  frame with 226/228 art exists but nothing routes into it).
- Night = 6 hours x 70 s = 7 minutes. No power drain.

============================================================================
1. STATE AND COUNTERS (all office-frame verified)
============================================================================
viewing (48) 0=office, 1..12 cams · last viewed (100) saved every 0.2 s
lit? (83) flashlight on · battery life (107): max 7000/6000/5000/4000/3000
for nights 1/2/3/4/5+, -1 PER FRAME while lit (~117/100/83/67/50 s total)
night (123) = global night number · time of the night (166) 12..6
being attacked by (155): 1 oldFreddy 2 oldBonnie 3 oldChica 4 oldFoxy
  5 toyBonnie 6 toyChica 7 toyFreddy 8 Mangle 9 Puppet 12 Golden
who got you (142): 4 old Foxy / 5 Mangle (for post-death frames)
puppet kill (206) · office occupied (173) · decide path (175) coin/1 s
in danger (140) · time allowed (151) 100/80/60/55/50/50/45 frames (n1..7)
time left (152) · got you stage (153) 0/1/2 · check and move (154)
blackout timer (193) scare accumulator -> 30 = static+metalrun (G633)
random image (146) hallucination roulette, re-rolled EVERY LOOP monitor-down
music button.alt0 = the music box gauge 0..2000 (init 2000, cap 2000)
per-character alterables: alt0 0 idle/1 armed/2 walking one node;
  alt1 move cooldown frames (decremented per loop; 40/50 when hall-lit,
  stun time when watched-lit, ~500 on retreats); alt2 post-move static 10
  (Puppet: route coin); Mangle alt6 attack-armed; old Foxy alt3 dark-charge
  seconds + alt9 light-on-him frames; old Freddy alt25 monitor-up seconds.

============================================================================
2. AI ROLLS, SCHEDULE, MOVEMENT
============================================================================
Rolls: every 5 s per character Random(20)+1 <= AI -> armed (alt0=1).
  old Foxy: weighted by dark-charge (Random(21)+Random(5)*alt3 <= AI);
  new Foxy/BB/Golden strict <; paper pals <=.
Schedule (G518-528): N1: h2 toyB2 toyC2; h3 toyB3 toyF2. N2: h1 toyF2 toyB3
  foxy1 BB3 mangle3 toyC3 golden rnd. N3: oldB1 BB1 oldC1 foxy2 golden rnd;
  h1 oldF2 oldC2 foxy3 toyB1 oldB3 BB2 toyC1. N4: mangle5 BB3 foxy7 oldB1
  golden rnd; h2 oldC4 oldF3 oldB4 toyB1. N5: toyF5 oldF2 oldB2 oldC2
  mangle1 BB5 foxy5 golden rnd; h1 oldF5 oldB5 oldC5 foxy7 mangle10 toyF1.
  N6: oldF5 oldB5 oldC5 mangle3 BB5 foxy10 golden rnd10; h2 oldF10 oldB10
  oldC10 toyB5 toyC5 BB9 mangle10 toyF5 golden3 foxy15. Puppet AI
  (G637-643): 1/5/8/9/10/15 (n1..6+). Night 7 = custom counters 181-190.
  Caps: most <=15, old Foxy <=17, Golden <=10 (force-0 on nights <6).
Arming gates: stage pairs (old F waits for old B AND old C to leave cam 8;
  toy F waits toy C off cam 9; toy C waits toy B off cam 9 — skipped on
  night 7); toys/old F won't move while their current cam is watched;
  Mangle arms only while unwatched.
Movement (one node per arming; NOTHING enters a hall while the hall light
  is on): old Freddy 8->7->3->{hall2 | back to 7}; hall2->office (monitor
  UP, office unoccupied); mask+unlit at hall2 -> retreat to 3 (huge cd).
  old Bonnie 8->7->hall1->1->5->office. old Chica 8->4->2->6->office.
  old Foxy 8->hall1->box (light off), who-got-you=4. toy Freddy 9->10->
  hall1->hall2->office. toy Bonnie 9->3->4->2->6->office. toy Chica 9->7->
  hall1->1->5->office. Mangle 12->11->10->7->hall1->2->{6|1}->office
  (masked leave 10%/s; desk arm on flip done). BB 10->7->3->1->5->office
  (mask removes 10%/s; blocks lights while present). Puppet 11->10->7->
  {3|4}->...->office->(10%/s)->box. Paper pals: teleport to office view
  (non-lethal) when armed and cam 4 is NOT watched.
Kill delay: entering office needs the monitor up; in office the danger
  countdown runs (see 3); monitor-up-too-long: old freddy alt25 >=
  20*Random(night)+2 s of continuous monitor -> straight to the box.

============================================================================
3. OFFICE ENCOUNTER PIPELINE
============================================================================
in danger=1 -> time left := time allowed; got you stage=1.
  Mask during the window -> stage 0 (attacker returns home).
  Expire -> stage 2; the darkening overlay (alt0 0..300, flicker at
  20/100/200, hide 250) reaching 300 -> check and move: stage 0 -> home;
  stage 2 -> got you box. At the box: per second 50% KILL vs masked 10%
  escape (the mask is nearly futile at the box). Extra triggers: toy
  Bonnie + monitor up -> box; old-anims flip-down/mask-up animation plays
  -> attacked; Puppet box = immediate; Mangle armed + monitor down = box;
  Golden box + 1 s tick = attack.
Dispatch: attack animation value 12-21 per attacker + Xscream3 ch12;
  Puppet adds puppet kill=1; after >=40 ticks or anim end -> NIGHT
  RESTARTS IN PLACE (positions re-init, hour reset to 12, night re-read).

============================================================================
4. MASK / FLASHLIGHT / MUSIC BOX
============================================================================
Mask: mask.alt0 0 off/1 lowering/2 on/3 raising; FENCING_43/42 ch7;
  breathing ch8 60/0; mask closes the monitor, forces lit?=0, hides
  battery/night HUD/flip buttons. Defenses: old B/C/F + toy F danger
  defused; toy B leaves 50%/s (33% in danger); toy C 10%/s; Mangle 10%/s;
  BB 10%/s; old Freddy box-escape 10%/s. Puppet IMMUNE (no group).
  toxic meter (0..20, +1/0.5 s masked, -1/0.5 s off) is COSMETIC ONLY.
  The dump's alt12 timer is broken by a per-loop reset (G179) — the
  retire rules for BB/Mangle/toy C via alt12>=5 are dead code.
Flashlight: hold (office G35 / cams G36-37); error click ch12; off on
  release/battery 0/danger/mask/BB-desk. Reveals lit hall views; freezes
  hall entries (alt1=40/50); pushes old Foxy back (alt3=0, alt9+=1/frame;
  at alt9 > 100+night he retreats + 500+Random(500) cd); Golden attack if
  lit on him; vent lights = hold over left/right light zones (BB blocks
  with error; auto-off 0.2 s; views 37/53/82 and 38/47/83).
Music box: wind = hold the button on CAM 11: +5/frame, windup2 every 0.5 s,
  wind-lock 10; <300 while holding snaps to 300. Drain per 50 ms not
  winding: N1/N2 -2, N3 -3 (-4 demo), N4 -4, N5 -5, N6/N7 -6. Gauge <= 0:
  every 1 s Random(20)+1 <= Puppet AI -> emerge stage 1..3 (winding stops
  further stages, never re-boxes); stage 3 + roll -> walking. In office:
  10%/s -> box -> attacked=9. danger 1/2 sprites at gauge <=400 (angry
  face <=200) once he left cam 11; melody ch13 vol 40 (cam11) / 15
  (cam10,12) / 5 (cam9) / 0; N1: full gauge and no drain during 12-1 AM.

============================================================================
5. FEEDS / VIEWS (Active 16 values per cam, later groups override)
============================================================================
cam01 27/28/51/59 · cam2 33/65/34/44/66 · cam3 29/71/30/43/72 ·
cam4 31/32/88/67/92/91/90 · cam5 13/14/96/52/60/100 · cam6 15/16/45/68/81
(+ribbons) · cam7 21/48/22/49/57/70 · cam8 23/24/62/63/75/64/94 ·
cam9 46/39/41/89/12/40/42/89 · cam10 17/85/18/87/54/86 ·
cam11 25/26/77/78/79/95 · cam12 19/20/80.
Known quirks: cam 9 with only toy Bonnie gone matches no view (stick);
cam 4 unlit shows toy Bonnie's eyes (G116); rare cam5/8/11 images bump
blackout timer (30 = static+metalrun); random image==9 -> JJ under the
desk; Shadow Bonnie = 1-in-1,000,000 per monitor flip, ends the app in 4 s
(replicate the "crash"); desk plushes from Ini c1..c10; Halloween pumpkin
on Oct 31 (system date); BB laugh every 2 s at the desk; Mangle radio
garble by location + 1 s noise barrage at the desk; popstatic during hall
blur; With_S2 breathing at close-by.

============================================================================
6. CLOCK / NIGHT FLOW
============================================================================
AM counter +1/s; >=70 -> hour++ (12->1 special) — 70 s/hour, 6 h = 7 min.
Phone: one shot at 2000 ms, call 1b..6b ch21 (N1-6), mute button hides
after 29 s or on click. 6 AM (time of the night == 6) -> frame 4 "static"
(stare loop) 5 s -> "next day": night number += 1, level saved; then:
night 2 -> card; nights 3/4/5 -> dream cutscene -> "it's me" error ->
card; night 6 -> the end (paycheck) + beatgame=1; night 7 -> the end 2
+ beat6=1; night 8 -> the end 3 (+beat7 if all-20). Death restarts the
night in place. Cheat: honk + C + D + NumPlus -> 6 AM.

============================================================================
7. DO NOT PORT (demo leftovers / dead code)
============================================================================
Game act #8 "expression jumps" G34/62/63/104/146 (alterable 11 never
written — dead); the broken alt12 mask timer (G179) — reproduce the
10%/s rolls, skip alt12; chicalookatyou (needs mask alt0==99); G740;
puppet-face hallucinations G365-368 (gated on a never-set flag);
blackout in progress / close it all / test 4 counters; the 1-in-10
static->quit branch after 6 AM (demo leftover).

============================================================================
8. NON-OFFICE FRAMES (quick map)
============================================================================
boot disclaimer (2000 ms) -> title (rare1 1/1000 = eyeless Toy Bonnie) ->
title (menu: options New Game/Continue/6th/custom; stars from INI
beatgame/beat6/c1; Continue <=5; New Game -> ad newspaper 9 s -> card) ->
card "12:00 AM Nth Night" 2.6 s (1/1000 rare1 Withered Foxy) -> office.
6 AM chain: static -> next day (chimes+cheer+confetti; night+1, level
saved; c<mode>=1 for custom challenges) -> dream (2500-px 8-bit pizzeria
panorama, night-specific scenes) -> "it's me" error -> card. After night
5/6/7: the end* paychecks -> title. gameover screen: unreachable in the
shipped chain (leftover). Rare: 8bit "SAVE THEM" from gameover 1/1000 ->
load -> the 8-bit minigame cycle (SAVE HIM / HELP THEM / party) driven by
INI turn. Title wipe: Delete held -> full INI reset (incl. c1..c10).
Xbox-port note: FNAF2 jump names print CORRECTLY (no FNAF1 bug).
============================================================================
