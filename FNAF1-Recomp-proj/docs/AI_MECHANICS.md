# AI Mechanics — verified against the original event dump

Source of truth: CTFAK JSON export of the original `FiveNightsatFreddys.exe`
(frame 3 "Frame 1" event program, 435 groups). Event group numbers below refer
to `Events/frame_3_Frame 1_events.txt` from `upload/JSON.zip`.

All frame counts assume the original's **60 FPS** frame rate (application
header `frameRate: 60`).

## 1. Clock

| Fact | Value | Evidence |
|---|---|---|
| Frame rate | 60 FPS | application.json header |
| 1 in-game hour | 90 s (minute counter >= 90 → hour++) | groups 264–265 |
| Hour counter | 0=12AM … 6=6AM, night ends at 6 | groups 303, 265 |
| AI hourly bumps | 2AM, 3AM, 4AM (see §3) | groups 335–337 |

## 2. Movement opportunities (group 188–191)

Each animatronic has its **own** timer (centiseconds, CF Timer condition):

| Animatronic | Interval | Group |
|---|---|---|
| Bonnie  | 4.97 s | 188 (timer 4970) |
| Chica   | 4.98 s | 189 (timer 4980) |
| Freddy  | 3.02 s | 190 (timer 3020) |
| Foxy    | 5.01 s | 191 (timer 5010) |

Roll: **Random(20)+1 <= AI level** (AI held in counter `<name> activity`,
alterable/counter slot 80; Night 7 copies from the customize sliders via
`freddy AI`/`bonie AI`/`chica AI`/`foxy AI` counters, group 311).

Bonnie/Chica share the `move who?` channel (1 = Bonnie, 2 = Chica): if both
succeed on the same tick, the later write wins and the other loses its move
(a real original quirk). Freddy/Foxy do not use this channel.

## 3. Night AI tables (groups 305–311, 335–337)

Starting levels [Freddy, Bonnie, Chica, Foxy]:

| Night | F | B | C | X | Notes |
|---|---|---|---|---|---|
| 1 | 0 | 0 | 0 | 0 | |
| 2 | 0 | 3 | 1 | 1 | |
| 3 | 1 | 0 | 5 | 2 | |
| 4 | 1+Random(2) | 2 | 4 | 6 | Freddy randomized per run (group 308) |
| 5 | 3 | 5 | 7 | 5 | |
| 6 | 4 | 10 | 12 | 6 | NOT 20/20/20/20 |
| 7 | sliders | sliders | sliders | sliders | customize frame; 1/9/8/7 = 1987 easter egg |

Hourly deltas applied once when the hour is reached (groups 335–337):

| Hour | Freddy | Bonnie | Chica | Foxy |
|---|---|---|---|---|
| 2 AM | — | +1 | — | — |
| 3 AM | — | +1 | +1 | +1 |
| 4 AM | — | +1 | +1 | +1 |

Freddy's level never rises during the night.

## 4. Bonnie / Chica room graphs (groups 199–215, 232–244)

Choice value `a0` = Random(2)+1 ∈ {1,2} (re-rolled every 1 s, group 201;
statistically a coin flip taken at move time).

Bonnie (`move who?` == 1):

```
1A ShowStage : 1→Backstage   2→1B Dining
Backstage    : 1→1B Dining   2→2A WestHall
1B Dining    : 1→Backstage   2→2A WestHall
2A WestHall  : 1→Closet      2→2B WHallCorner
2B WHallCor  : 1→Closet      2→DoorL
Closet       : 1→DoorL       2→2A WestHall
DoorL (ready to attack left):
    door open   (state 0) → ENTER office ("got you left") → kill
    door closed (state 2) → retreat to 1B Dining
    door animating (state 1) → nothing this cycle
```

Chica (`move who?` == 2):

```
1A ShowStage : →1B Dining (only exit)
1B Dining    : 1→7 Bathrooms  2→6 Kitchen
6 Kitchen    : 1→7 Bathrooms  2→4A EastHall
7 Bathrooms  : 1→6 Kitchen    2→4A EastHall
4A EastHall  : 1→1B Dining    2→4B EHallCorner
4B EHallCor  : 1→4A EastHall  2→DoorR
DoorR (ready to attack right):
    door open   → ENTER office ("got you right") → kill
    door closed → retreat to 4A EastHall
```

Office entry = death sentence: the door/light buttons play "error" while an
animatronic is at `got you *` (groups 97/107). Kill renders when the monitor
is down (anims 35 Bonnie / 44 Chica, groups 117–118). If the tablet is up at
entry, hallucinations flicker (33%/5s, groups 276–279) and the tablet is force
dropped ≤ 30 s later (groups 277/279). Port behaviour: force-drop immediately.

Camera pose variant: `alterable[3]` = Random(2)+1 is set on each successful
opportunity (groups 192–193) and picks which of the two poses a camera shows.

## 5. Freddy (groups 190, 389–409)

Path: `1A → 1B → 7 Bathrooms → 6 Kitchen → 4A → 4B → door zone → office`
(each move = one path step per completed cycle below).

1. Opportunity (3.02 s): requires **monitor down** (`viewing == 0`) and roll
   pass → `pending = 1` (group 190).
2. Delay: a per-frame counter grows while `pending`; when
   `counter >= 1000 − AI×100` ticks **and** monitor is down → `go = 2`
   (groups 397–398). At AI 4 → 10 s delay; AI 10 → instant; AI ≤ 9 →
   100..1000 ticks.
3. Step 1A→1B additionally requires **neither Bonnie nor Chica on 1A**
   (group 389). Steps run regardless of monitor state.
4. At 4B, the decision runs only while the monitor is **up** (viewing > 0)
   and not while you watch CAM 4B (viewing != 42 — both groups 394/395):
   door open → step to the door zone (group 394); door closed → back to 4A
   (group 395, which additionally requires viewing != 4 / CAM 4A — with
   the door closed, watching 4A parks the retreat too). Freddy waits at
   4B until you raise the tablet — the documented "check cameras to
   advance Freddy" behaviour.
5. At the door zone with monitor down, power on, `fox progress < 5`:
   every 1 s, **Random(4)==1 (25 %)** → kill (anim 65) (group 406).
   With the monitor up he just stands there.
6. Every step also sets the laugh timer `Random(3)+1` (groups 389–395, audioned
   through groups 402–404).

## 6. Foxy (groups 191, 315–324, 40, 61–65)

Stages (counter `fox progress`): 0 curtain, 1 peek, 2 out, 3 lurking
(gone), 4 running, 5 at door.

- Opportunity (5.01 s): requires `viewing != 99` (you are not looking at the
  Cove), **tablet cooldown == 0** (see below), stage < 3, roll pass →
  stage += 1 (group 191).
- **Tablet cooldown** reuses `chica` alterable[12]: while any camera is up it
  is refreshed every 0.1 s to `50 + Random(1000)` ticks (group 329); it
  decrements every frame otherwise (group 313). Foxy moves only when it has
  fully decayed — this is the real "cameras hold Foxy" rule (0.8–17.5 s hold
  after the monitor drops, 60 FPS).
- Looking at the Cove while stage 3 → **instant run** (stage 4, anim 51,
  "run" sample, group 40).
- Stage 3 without being seen: after 1500 ticks (25 s) → straight to stage 5
  (group 320).
- Stage 4: run timer 100 ticks (1.67 s) → stage 5 (groups 315–317).
- Stage 5: tablet force-dropped (groups 321–322); then
  door open → kill (anim 52 + XSCREAM, group 323);
  door closed → **bang**: knock, power drain `(10 + 50×bangCount)` tenths
  = `1 % + 5 %×bangCount`, stage = **Random(2)** (0 or 1) (group 324).
- Cove cameras: stage 0/1/2/3 → anims 26/48/49/50; the rare stage-3 picture
  (anim 73, the "Golden Freddy photo" easter egg) shows when `random for pic`
  <= 10 (groups 61–65).

## 7. Power (groups 175–177, 285–286, 342–345, 324)

- `power left` starts at **999 tenths** and is displayed as tenths (power%2 =
  value/10).
- Usage = 1 + monitor + left door + right door + left light + right light
  (1..5). Drain: **usage tenths per second** (group 177 every 1 s).
- Extra per-night drain: N2 −1/6 s, N3 −1/5 s, N4 −1/4 s, N5+ −1/3 s
  (groups 342–345). Night 1 has none.
- Foxy bang: `(10 + 50×bangs)` tenths → 1 %, 6 %, 11 % …
- `power <= 0` → power-down sequence (§8).

## 8. Power-out sequence (groups 272–302)

Phases of `Active 2 alterable[6]`:

| Phase | What happens | Exit |
|---|---|---|
| 0 dark | office dark (anim 46) | 20 % per 5 s, forced at 20 s → phase 1 |
| 1 jingle | music box plays; face flicker: every 0.5 s Random(4)+1 == 1 → lit (anim 47), else dark (anim 46) | 20 % per 5 s, forced at 20 s → phase 2 |
| 2 buzz | 20-tick flicker (alterable[7] = Random(2)+1: buzz + office shown/hidden) | after 20 ticks → phase 3 |
| 3 black | total darkness | 20 % per 2 s, forced at 20 s → Freddy kill |

6 AM still saves you during any phase (the clock keeps running).

## 9. Jumpscare animations (Active 3, application.json)

| Anim | Use | Frames | Speed → FPS |
|---|---|---|---|
| 34 / 43 | Bonnie/Chica **window pose** (single frame 225/227) | 1 | — |
| 35 | Bonnie **kill** | 301,291,303,293,294,…,300 (11) | 75 → 45 FPS |
| 44 | Chica **kill** | 279,65,281,69,216,228,…,239 (16) | 99 → ~60 FPS |
| 51 | **Foxy run** (west hall sprint) | 33 frames, backTo 31 | 65 → 39 FPS |
| 52 | Foxy kill | 25 frames | 50 → 30 FPS |
| 65 | Freddy kill | 31 frames | 50 → 30 FPS |
| 46 / 47 | power-out dark office / Freddy face lit (304 / 305) | 1 | — |
| 56 | power-out office base (476) | 1 | — |

Historical note: v2.7.11 and earlier used the window poses (34/43) as the
Bonnie/Chica kills and played anim 51 (Foxy's run) as the "Freddy flicker"
during power-out. Both fixed in v2.7.12.

## 9a. Kill timings (full scare map, verified from the events)

Timers are raw Clickteam milliseconds (the dump's "~Ns" annotations are a
50 Hz-scale artifact — 20x off); one "per frame" decrement = one engine tick
(frame); eff FPS = speed x 0.6.

IMPORTANT reading note: jump actions print the wrong frame NAMES in the dump
(misaligned name lookup) — the raw jump VALUE is the storyboard slot and is
correct: 0 office ("Frame 1"), 1 "died", 2 "freddy", 3 "next day",
4 "what day", 5 "title", 6 "wait", 7 "gameover", 14 "creepy end". So office
win → 3, night card → 6 → 0, office kills → 1 (died), power-out kill → 2
(freddy), Golden → 14 (creepy end), gameover → 5 (title), 1/10000 → 14.

| Scare | Trigger (office groups) | Kill presentation | XSCREAM | Exit |
|---|---|---|---|---|
| Bonnie | ready-to-attack-left + left door OPEN (214) -> crv=2 (22) -> camera down | Active 3 anim 35 set at 117 (loops); lights off; office pan centred 800 (225/230) | +9 ticks ≈ 0.150 s (228/229: alt2 10→1), ch9 | +39 ticks ≈ 0.650 s → "died" (262/263) |
| Chica | mirror right side (244, crv=4 via 23, camera down 231) | anim 44 at 118, same | same | 0.650 s → "died" |
| Foxy | fox progress 5 → camera ripped down (321/322) + left door open | anim 52 at 323, doors hidden | SAME FRAME (323), ch9 | anim 52 ends (0.833 s) → "died" (325) |
| Freddy | inside "freddy got in" + camera down + power ok + fox<5; 1-in-4 per 1 s tick (406, mean ~4 s) | anim 65 at 406; lights off; normal-view groups all gate bear.alt6==0 | anim 65 frame 7 ≈ 0.233 s (409), ch9 | anim 65 ends (1.033 s) → "died" (407) |
| Golden | poster roll 1/100000 per s when 2B (425), show (44), appear in office (420) | the "yellow bear" sits in the office, no anim | none in the office | 300 ticks ≈ 5.0 s → "creepy end" (421/422): stop all + XSCREAM2 ch29, +1000 ms → End application. Escape = raise camera (423) |
| Power-out | power left <= 0 → 286 (stop all, "powerdown" ch1, dark loops, HUD hidden) | anims 46/47 face flicker re-rolled every 50 ms (289 + 287/288); music box ch30 starts 5–20 s in (272/273) | none in the office frame — the kill lives in the "freddy" frame | box ends 5–20 s later (291/292) → 20-frame face/buzz flicker (297/298) → pitch black 2–20 s (301/302) → "freddy" frame: XSCREAM ch1@100 + the obj-152 dark-face anim [326,307,348,308..325] (backTo 5) → static + blip flashes → 12 s → "gameover" (10 s) → title |

Post-scare: "died" = stop all + "static" ch1, 10 s → "gameover"; "gameover"
stops all, holds 10 s → "title"; 1-in-10000 per second there diverts to
"creepy end" instead (frame 8 group 3).

The XSCREAM sample is always a one-shot on channel 9 (channel 1 in the
power-out "freddy" frame). There is NO audio fade anywhere in the original:
only instantaneous channel-volume sets and hard stop-alls.

Port status (v2.48/2.49): the office kills use dump-exact anims and the
delays above (Freddy 7/30 s, Bonnie/Chica 9/60 s, Foxy/Golden/dark = instant);
the scare length = the kill-anim duration (0.65 / 0.83 / 1.03 s / dark 0.85).
The power-out staging in the port collapses to jingle → dark kill
(USER DECISION — the dump's buzz/black gap is documented here but not played),
with ONE wiki detail spliced in between (user-picked): after the jingle cuts,
Freddy's footsteps (`deep steps`) are heard for a 1.5 s dark warning window
("before Freddy's jumpscare his footsteps are audible"), then the dark kill.

## 10. Misc

- Camera IDs (`viewing` counter): 0 office; 1=1A, 2=1B, 3=2A, 4=4A, 5=5,
  6=kitchen, 7=7, 22=2B, 33=closet, 42=4B, 99=Pirate Cove (full evidence
  chain in CAMERA_FINDINGS.md — 4/42 are camera ids, NOT flip states).
- Kitchen sound reroll: Chica in kitchen re-rolls kitchen cam alterable every
  1 s (group 245).
- Footstep volume grows with proximity: 10 → 20 → 30 → 40 (Speaker ch0 volume
  per graph edge), muted while at the door (groups 212–213).

## 11. v2.46 audit addendum (three dump passes: frame 3 + customize + title)

Corrections applied in v2.46:
- TICK RATE: the port ran its logic at 30 Hz while every constant is
  denominated in 60 Hz ticks — hours 180 s, power/AI half-speed. Fixed
  (main.cpp tickDelta = 1/60; the application frameRate is 60).
- Foxy sprint (group 40): triggered by viewing CAM 2A (viewing==3) at
  progress 3 — NOT the Cove. On 1C the cove just renders empty (64/65).
- Freddy office kill (group 406): the 1 s Random(4)==1 roll is gated on
  fox progress < 5; entering the zone kills both lights (406/408-412) and
  loops "whispering2" (405); watching his cam resets the delay counter
  (group 401: freddy bear alterable[13] := 0 while you are on his cam).
- Door mid-transition (214/215/243/244): door alterable[0] 0=open/1=closing/
  2=closed/4=opening — enter only at 0, retreat only at 2, hold (retry every
  tick) at 1/4. The port decided instantly on the analog amount.
- Night-start AI tables re-verified (groups 305-311): the counters used by
  the movement events are the "activity" counters 112/121/127/138, written
  from a hardcoded table per night; night 7 copies the Customize globals
  141-144 into them. The AI counters are never written on nights 1-6.
- Custom Night (frame 12, groups 5-17): four global counters 141-144,
  +/- arrows clamp 0..20; 1987 (1/9/8/7, group 13) jumps to the creepy
  screen; START -> night 7.
- 'screen follow 1' (groups 2-12): the feed auto-pan — linear ping-pong
  1 px/tick over 320, 100-tick dwell at each end, starts at the left edge.
  'control room follow' (groups 83-88): the office stick pan, clamp
  X 640..960, 2/5 px per frame; spawns at X=640 (the LEFT edge; 800 = the
  centered kill close-up, groups 230/231).
- Move-under-camera static (groups 194-198, 219-222): a move while the
  room is watched → 300-tick static-out ('you' alterable[0]), 'Active 3'
  hidden, blip flash storm + garble/digital sample.
- Title "blip flash 2" (title groups 4-7): anim [430,435,436,434,438,439,
  437,22] speed 10; visible 1-in-3 six-second windows; alpha coefficient
  := Random(100)+100 every 1.6 s. The title static alpha := 50+Random(100)
  every 1.8 s (group 1) — the port keeps its approved 0.61 constant.
- Extra per-night power drain groups 342-345 (6/5/4/3 s on nights 2/3/4/5+)
  was already implemented (PowerSystem).
- "Lives" (lives left = 5, Ini saves at anim 35/44): a Clickteam template
  leftover, NOT used by FNAF's flow — the scare goes straight to Game Over
  / title. The port intentionally has no lives (user decision, v2.46).
