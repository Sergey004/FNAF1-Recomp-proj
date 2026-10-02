# Sister Location — Movement & Game-Flow Mechanics (dump digest)

Source of truth: `ctfak-cpp/build/Dumps/Sister Location/` (Events/, JSON/application.json,
Images/, Sounds/). Research pass 2026-10-02 (two dump sweeps + first-source spot checks).
This is the implementation reference for the port's SL wave (staged LAST per the roadmap).

Reading conventions (established project-wide, see DUMP_ATLAS.md): **timer values are RAW
MILLISECONDS** (the dump's `(~Ns)` labels divide by 50 and read 20x long); condition/action
cheat sheet = DUMP_ATLAS.md's. The SL dump has **no FrameLayout/ directory** — per-frame
instance tables were recovered from `JSON/application.json` (`frames[i].layers[]/instances[]`).
Game window: 1280x720; room frames are LARGER and pan.

---

## 1) Frame list (storyboard slot = 0-based index, verified)

| # | Name | Size | Role | Gameplay? |
|---|------|------|------|-----------|
| 0 | Warning | 1280x720 | legal warning; inter-passage splash (Enter/click/60 s); INI load; Delete = wipe save | no |
| 1 | title screen | 1280x720 | menu (new game / continue / extras / custom) | no |
| 2 | Elevator | 1900x1000 | cabin ride + HandUnit scripts (nights 2/3/5) | ride/hub |
| 3 | Vent crawl | 1700x1000 | vent travel (hold W) | travel |
| 4 | load | 1280x720 | **the travel router**: consumes the global `go to` code | router |
| 5 | Main Hub | 1900x1000 | Circus Control: vents, lights, shock buttons | gameplay |
| 6 | Baby's Room | 1700x900 | Baby's gallery (night 1 speech; night ends) | gameplay |
| 7 | to vent | 1280x720 | 100 ms black hop, always → Elevator | transition |
| 8 | Ballora Gallery | 2200x1100 | walk + the dance mechanic | gameplay |
| 9 | Breaker Room | 1900x1000 | breaker reboot task (Freddy danger) | gameplay |
| 10 | Funtime Auditorium | 1900x1100 | walk + flash beacon + Foxy | gameplay |
| 11 | Parts and Service | 1700x1500 | Bonnie module (script-gated face buttons) | gameplay |
| 12 | win night | 1280x720 | night complete: `night 2 += 1`, INI save, checkmark | no |
| 13 | Under Desk | 1900x1000 | hide-under-desk (Bidybab eye-match) | scene |
| 14 | death | 1280x720 | 8-bit death / "planned death" router | no |
| 15 | game over | 1280x720 | jumpscare-caught; per-night respawn router | no |
| 16 | "Night 4" | 2000x1000 | **mislabeled** — first-launch Baby monologue cutscene (`stage` 0→16, `Part01-A…`) | cutscene |
| 17 | intro | 1280x3000 | night-4 start/resume card (plays `intro`, sets INI `intro=1`) | no |
| 18 | Extras | 1280x720 | extras menu (jumpscare viewer) | no |
| 19 | Parts and Service 2 | 1700x1500 | night 5 (Ennard) | gameplay |
| 20 | Scooping Room | 1900x800 | night 4 end scene | scene |
| 21 | red fade out | 1280x720 | interstitial; sets `end words=1` on entry | no |
| 22 | bathroom | 2000x720 | night-5 mirror scene | scene |
| 23 | credits | 1280x720 | credits (after bathroom) | no |
| 24 | tv show | 1900x900 | post-win interstitial | no |
| 25 | Girl Voice | 1280x720 | between-night voice interludes (`line_1..6` by night) | no |
| 26 | 8-bit Baby Game | 12800x720 | secret platformer (Ballora Gallery: type "1"+"0") | minigame |
| 27 | Final Encounter | 1900x800 | night 5 encounter | gameplay |
| 28 | "Frame" | 1280x720 | keypad screen (keycard); **no frame exit in the dump** | ? |
| 29 | Custom menu | 1280x720 | custom-night difficulty select | menu |
| 30 | custom please wait | 1280x720 | 2 s loader → custom level | no |
| 31 | custom level | 1900x800 | **custom-night OFFICE** (501 groups; FNAF1-style doors/cams/power) | gameplay |
| 32 | custom death | 1280x720 | → Custom menu | no |
| 33 | custom night win | 1280x720 | → menu / cutscenes | no |
| 34 | custom 8-bit cutscene | 3000x720 | cutscene | no |
| 35 | Final Cutscene | 2280x1220 | → Custom menu | no |

In-night gameplay rooms: 2, 3, 5, 6, 8, 9, 10, 11, 13, 19, 20, 27 (+ 31 for the custom night).

---

## 2) Room-to-room travel: the global `go to` counter + the load router

Travel is NOT click-to-exit doorways. It is a two-layer system:

1. Gameplay **arms the global counter `go to`** with a room code (progress thresholds,
   vent clicks with availability flags, key S in P&S) and starts the room's fade-out.
2. **Frame 4 "load" consumes the code**: every group is
   `ON "go to" == N && ON "print" != 1 && ON Timer 200 (200 ms, every) → "go to" = 0; Jump to frame`.
   Verified verbatim. Route table:

| `go to` | target frame |
|---|---|
| 1 | 2 Elevator |
| 3 | 5 Main Hub |
| 5, 8 | 7 "to vent" (→ Elevator) |
| 6, 11 | 9 Breaker Room |
| 7 | 8 Ballora Gallery |
| 10 | 10 Funtime Auditorium |
| 12 | 6 Baby's Room |
| 13 | 20 Scooping Room |
| 30 | 22 bathroom |
| 2, 4, 9 | 4 load (self; hold) |

The SAME mechanism is the death-resume path: frame 15 "game over" jumps to `load` while
`go to` still holds the current route, with the global `Script Event` counter preset to the
night's resume point (night 2 → 81, night 3 → 111, night 4 → 57, night 5 → 512).

**Labeled dump caveat:** every first-person room also has a `fade out > 255 → value 1
("title screen")` group (frames 2/3/5/8/9/10/11/19). The raw parameter really is 1, but the
value contradicts the go-to code the same room just armed and duplicates FNAF1's quit
template; the exporter is known to misalign jump names (DUMP_ATLAS.md). **Port decision:
treat those rows as inert and route all travel by the go-to semantics + TRANSITIONS.txt.**
(TRANSITIONS.txt itself is COMPLETE and self-consistent for SL — unlike FNAF1's — 99 jumps +
4 next-frame actions, all names matching targets.)

---

## 3) In-room movement: W-hold progress (no avatar — "you are the camera")

No walk room has a player sprite. Each room has an invisible cursor proxy (`cursor`,
`target`, `Active 2`) and a **Perspective extension object** that eases/animates the room
backdrop (its C++ source ships in this workspace: `/home/user/FNAF1-Recomp/CTF-Perspective/`
— port the easing from THERE, do not guess the curve). Movement itself is a counter:

Ballora Gallery as the model (frame 8 events, verbatim anchors):

- Arming — W held once after 2 s arms the crawl latch:
  `[G2] Timer 2000 + Key W + jumpscare==0 → "crawl" := 15` ;
  `[G3] crawl > 0 → crawl -= 1` (a 15-tick decay buffer fed while held).
- Per-tick states (Groups 84/85): `crawl > 0` + Shift **not** held → floor/door anim
  speed **30**, quiet patter (ch3 vol 50); Shift held → speed **50**, loud patter
  (ch2 vol 100). Movement: `quick count` accumulates 1 or 2 per tick; at >= 5 →
  `progress += 1` (G41). **Holding W fills `progress` ~1/tick, Shift 2/tick.**
- Exits: `progress >= 850 && backwards == 0 → go to = 7` (Ballora Gallery reached);
  `progress >= 650 && backwards == 1 → go to = 9` (leave back to Baby's Room).

Other rooms use the same skeleton with their own thresholds: Breaker Room (the breaker
task instead of progress), Funtime Auditorium (progress 2000 forward / 1500 backward),
P&S (key S + Script Event gate).

---

## 4) Room mechanics

### Ballora Gallery (8) — "the dance"
- Ballora (`dancer`) is created at `progress >= 400`.
- **`distance`** = how much she has heard you: +1/100 ms while W held; while she is
  actively approaching (`moving toward == 2`, set after 5 s of continuous walking via
  `small trigger` alt10 > 300) +1/+2 per tick; **decays −1 per 30 ms while you stand**.
- Her music **pans by side**: `left pan` ramps −100..+100 (1 per 20 ms) — the audio pan IS
  the "which side is she on" cue.
- **Death: `distance > 600 → jumpscare`.** A "quickly!" hint plays at progress >= 300 &&
  distance < 200.
- Secret: typing keys "1" then "0" accumulates `morum`; > 5000 → the 8-bit game (26).

### Vent crawl (3) — hold, not mash
- Hold W: `crawl := 15`, `location.alt0 += 1` per held tick (+Shift = `loud := 15`).
- States: crawl>0&loud>0 → vent anim speed 75, `metal_duct_fast` ch2 vol 100;
  crawl>0&loud==0 → speed 40, `metal_duct_slow` ch3; both 0 → hidden/silent.
- **100 held ticks = one "notch"** (`location` moves (0,−1), alt1 += 1);
  **10 notches traverse the vent**: `alt1 >= 10 && vent going == 1/2/3 → go to = 2/3/4`
  (2 = hold, 3 = Main Hub, 4 = hold).
- No enemy, no death in the vent frame — just bang sounds (Random 1-4 every 1 s while
  `Script Event == 29`) and dialogue triggers. Bidybab is NOT here ("Bidy" = 1/1000 idle
  easter egg in Baby's Room/Elevator); the real encounter is Under Desk (13).

### Funtime Auditorium (10) — flash beacon vs Foxy
- W = walk (+1/tick), Shift = scurry (+2/tick); A/D strafes exist only to advance
  HandUnit's scripted `directions01-11` monologue.
- **Space = flash**: requires `recharge >= 50` (refills +1 per 40 ms → 2 s full);
  flashing **ADDS +50 to `distance`** (it ATTRACTS him), plays `flash` + `recharge2`.
- Foxy: `approaching = Random(2)` every 3 s; approaching → `distance` +1/tick else −1/tick
  (clamped 0..600); W held adds +1 (+2 Shift). Repositions among `position 1/2/3` every 2 s.
- The three silhouettes by distance bands: <=100 hidden, 100-200 `close Foxy 3`,
  200-300 `close Foxy 2`, >400 `close Foxy`; force-hidden at progress > 1300/1700.
- **Death: `distance > 500` while flashing, or `distance > 600` walking** → `foxy scare`
  + "scream op5-2" → game over (15). Night 5 uses `Ennard_scare`.
- **Backwards trip ends night 3**: `progress >= 1500 && backwards == 1 → got you = 1;
  night 2 = 4` (the always-fatal scare that IS the night 3→4 transition).
- Exits: `progress >= 2000 → go to = 10` (or `= 13` on night 5 → Scooping Room);
  keycard route: `have keycard` + timer → frame 28 (keypad).

### Main Hub (5) — Circus Control
- Vents: click `into vent` (Baby vent, needs `Baby vent available == 1` → `vent going := 2`
  → fade → Vent crawl), `into left vent` (Ballora, `go to := 5`), `into right vent`
  (Foxy, `go to := 6`); unavailable → a "denied" sound.
- Lights = hold-click; shock buttons = sparks FX + advance the `Script Event` voice script.
- Easter egg: `Minireena` @(633,725).

### Elevator (2) — the ride state machine
- `stop elevator` 0→4: dialogue chains (`Script Event` per-night presets: night 2 → 50,
  night 3 → 100, night 5 → 500; night 1 is the default `== 0` chain) + the movement anim
  (`Active 3.alterable[18]` 0→1500 → arrival clank sets 3); 4 = doors open, `map`/`location`
  shown, `elev button` arms after a 40 s timer.
- Click `elev button` → "clankv2" → state 5; click `go forward` → `go to = 1` + fade
  (→ load → Elevator, i.e. the arrived state re-enters through the router).
- "Bidy" idle easter egg: `alt0 == Random(1000)`.

### Parts and Service (11) / P&S 2 (19) — script-gated buttons
- The Bonnie-module face buttons (`right cheek`, `right eye`, `nose`, `left cheek`, `chin`)
  accept a click ONLY when the `Script Event` counter equals the expected beat
  (e.g. `Script Event == 151 && cursor over "right cheek button" → 152 + sound`);
  a wrong button plays "Clarification" and does NOT advance.
- Exit: key S + `Script Event` gate → `go to = 11` (→ Breaker Room via load).

### Under Desk (13) — Bidybab
- `bidybab` + `eye match`: her eyes track the cursor; `peek 1` plays "hello_in_there";
  you must keep the `eye match` cursor overlapping her face spot (Groups 12-19).
- Exits: `jumpscare` → 15; `fade out 2 > 255` → 2 Elevator.

### Baby's Room (6) / Scooping Room (20) / Final Encounter (27)
- Baby's Room: night-1 speech (via the monologue cutscene 16 on first launch); leave via
  vent (`go to = 3`); **the night ends here**: `fade out 2 > 255 → win night (12)`.
- Scooping Room: `fade to red` → Next → red fade out (21) — feeds the night-5 interlude.
- Final Encounter: scare timers → game over; `fade > 255` → Baby's Room.

---

## 5) The Script Event spine (dialogue + pacing)

One global counter `Script Event` indexes the voice script ACROSS frames. Each line is:

```
ON "Script Event" == N
ON Speaker "<line_N>" finished
DO Speaker play "<line_N+1>"
DO "Script Event" := N+1
```

Known block indices: night-1 elevator chain from 0; night 2 → 50; night 3 → 100;
night 4 → 57; night 5 → 500; P&S beats ≈ 151+; death-resume presets 81 (n2 checkpoint 1) /
111 (n3) / 57 (n4) / 512 (n5). The `print` counter (text still "printing") gates travel:
load-router groups require `print != 1`. HandUnit is an Active whose animation value
selects the talking anim (12/13/14).

---

## 6) Nights, save, death resume

- The night number is the global counter **`night 2`** (1..6), initialized from the INI at
  the title (`Ini "sl"; night 2 = IniGet("current")`, clamped 1..5) and set to 1 on new game.
- **A night ends in Baby's Room** → `win night` (12): on frame start play `Jingle_4b`,
  INI load/save "sl", **`night 2 += 1`, INI `current = night 2`** → checkmark → tv show (24)
  → `night2 < 6` → Girl Voice (next interlude) / `>= 6` → red fade out → `end words = 1` →
  Girl Voice `line_6` → **P&S 2 = night 5**.
- Night 4 is special: walking BACKWARD in Funtime Auditorium sets `night 2 = 4` directly;
  its start/resume both go through `intro` (17, sets INI `intro=1`) → Parts and Service.
- INI file **"sl"** keys: `current` (night), `intro`, `beat1` (Extras unlocked),
  `beat3` (Custom unlocked), `keycard` (star 2), `endsceneno`, `104` (star 4).
  Warning-frame Delete wipes: `current`, `beat1`, `intro`, `beat3`, `keycard`, `endsceneno`.
- Death resume (game over 15 → load): night 2 → `Script Event = 81` (needs the
  `night 2 checkpoint` flag set on Ballora Gallery frame start), night 3 → 111,
  night 4 → `Script Event = 57, 4 continue = 1` → intro, night 5 → 512.

---

## 7) Custom level (31) — the custom-night OFFICE

NOT the checklist and not a debug frame: after Custom menu (29, the difficulty select) →
"custom please wait" (30) → frame 31 (1900x800, 137 instances, 501 groups) is an
FNAF1-style office: `left door` / `right door` / `vent door`, `left side`/`middle`/
`right side` with `current pan` easing toward `set pan to` (+1 per 50 ms), cams
`cam 01..07` + `viewing cam`, `controlled shock`, oxygen/power (`power left` countdown),
and per-animatronic difficulty counters (Bidybab/Ballora/Bonnet/Electrobab/Foxy/Freddy/
Funtime Foxy/Lolbit/Minireena×2/Yenndo, plus a `GFVH mode?`). Controls: mouse click AND
keys **A/D/W/S** (A = left door, D = right door, W = vent door, S = cams) with a 20-tick
toggle cooldown.

---

## 8) The 8-bit Baby Game (26, 12800x720)

A side-scrolling platformer: an invisible `hit box` + sprite with four feel-sensors;
A/D walk (14 px per 50 ms), **Shift = jump** (`going up := 200` fuel, −7/−5/−3 px/tick
while held), gravity via `feel bottom`; the room **scrolls** (`screen follow` eases
20 px/tick toward the player; `leave left/right` triggers snap ±1280 = page flips);
collect `powerup` (10 ammo), throw `projectile` with "/" (20-tick cooldown), avoid `die`
objects and a countdown; win at `goal 2` @(12661,609). Death → frame 14 → Extras.

---

## 9) Ambiguities & port decisions (do not guess)

1. **The `fade>255 → value 1` exit rows** in every room: raw param really is 1, but flow
   contradicts it (see §2). Port: route by go-to semantics; label the deviation.
2. **Interlude loop**: as printed, nights 1/2/3/5 interludes end `Girl Voice → Warning →
   Next → title`, and the title immediately jumps back if `started > 300` persisted — the
   chain cannot terminate. Port: treat "interlude end = night start" per the night-4
   pattern; verify against the original binary if it ever matters.
3. **Perspective easing**: the extension's internals are not in the dump; port the curve
   from the CTF-Perspective source in this workspace.
4. Frame 28 ("Frame", keypad) has no frame exit in the dump (only End application) — its
   continuation must come from flow context (post-keycard route).
5. Sprite sizes are not in application.json — take them from Images/*.png.
