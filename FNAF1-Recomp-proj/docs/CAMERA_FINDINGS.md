/**
 * docs/CAMERA_FINDINGS.md -- v2.7.10-camfix
 * Where every camera feed image comes from, with the exact evidence chain.
 * (CRLF, ASCII -- kept in the repo docs style.)
 */

# CAMERA FINDINGS (v2.7.10)

## The bug the user saw

CAM 5 "Backstage" showed img_540: a dark corner room with a
Golden-Freddy-looking poster. That image is real FNAF1 data, but it is
CAM **2B**'s rare golden-poster variant -- not the backstage.

Root cause: the PakCamFeed enum (v2.7.4..v2.7.9) was filled by visual
guessing ("verified visually" comment was simply wrong). The images
themselves were all genuine camera feeds -- each one just belonged to a
DIFFERENT camera. Everything from 2B onward was scrambled:

| CAM | v2.7.9 (wrong)   | v2.7.10 (canonical)     |
|-----|------------------|-------------------------|
| 1A  | 19/68/223/224/484| same (was correct)      |
| 1B  | 48/90/215        | same (was correct)      |
| 1C  | 211/338/240      | 66/211/338/240 (was one stage late, no closed curtain) |
| 2A  | 43/206/226       | same (was correct)      |
| 2B  | 486 empty / 205 Bonnie / 354 Chica | 0 empty / 479 Bonnie / 478 Chica |
| 3   | 62/190           | same values, but note 62 IS the closet (was correct by luck) |
| 4A  | 67/221           | same (was correct)      |
| 4B  | 49 empty / 220   | 486 empty / 220 Chica   |
| 5   | 540 empty / 555  | 83 empty / 555 Bonnie   |
| 6   | audio only       | audio only (correct)    |
| 7   | 41/217           | same (was correct)      |

Net effect in v2.7.9: CAM 2B showed the BACKSTAGE (83), CAM 4B showed a
2B poster variant (49), CAM 5 showed the 2B golden-poster room (540).
Three rooms were effectively rotated. 1A/1B/2A/4A/7 and the closet were
already right; the cove stage table was off by one.

## The evidence chain (application.json, office frame events)

1. **Map button highlight groups pin the viewing id per camera.**
   Each named map button gets "set anim 12" (pressed look) exactly when
   the counter 49 ("viewing") equals its value:
   viewing==1 -> obj75 '1A show stage', ==2 -> obj76 '1B dining area',
   ==3 -> obj77 'cam 2A', ==22 -> obj81 'cam 2B', ==6 -> obj91 kitchen,
   ==7 -> obj94 'cam 7 bathrooms', ==5 -> obj89 '5 backstage',
   ==42 -> obj86 'cam 4B', ==33 -> obj82 'cam 3 closet',
   ==4 -> obj85 'cam 4A', ==99 -> obj95 '1C stage B'.
   (This is also why the viewing ids look weird: 22=2B, 33=3, 42=4B,
   99=1C, 5=5, 4=4A.)

2. **Each viewing family sets one "Active 3" (obj 44) animation.**
   Groups that act on obj 44 with "set anim N" carry a viewing==id
   condition; the anim's frame list IS the feed image handle:
   - 1A (viewing==1): anim17=[19] full stage, 27=[68] no-Bonnie,
     41=[223] no-Freddy, 42=[224] Freddy-only, 60=[484]/54=[355] empty
   - 1B (==2):  13=[48] empty, 28=[90] Bonnie, 55=[215] Chica,
     40=[222] alt empty, 61=[492], 29=[120] Bonnie-2
   - 1C (==99): 26=[66] stage0, 48=[211] stage1, 49=[338] stage2,
     50=[240] stage3 (hour>10), 73=[553] stage3 rare "IT'S ME" (hour<=10)
   - 2A (==3):  14=[43] empty, 33=[206] figure, 15=[44], 39=[221] Bonnie
   - 2B (==22): 20=[0] "LET'S PARTY!" empty, 59=[479] Bonnie face,
     58=[478] Chica face, 74=[540] golden poster (rare),
     75=[571] Golden Freddy sitting (easter egg), 69..72=[549..552]
     RULES FOR SAFETY poster variants
   - 3  (==33): 24=[62] empty (single hanging bulb), 31=[190] Bonnie
   - 4A (==4):  25=[67] empty east hall, 39=[221] Bonnie figure
   - 4B (==42): 64=[486] empty corner, 38=[220] Chica figure
   - 5  (==5):  22=[83] BACKSTAGE (endo on table, heads shelf,
     EMPLOYEES ONLY door), 53=[354] Bonnie standing, 32=[205] dark face,
     66=[555] face very close
   - 6  (==6):  Active 3 hidden + obj98 "audio only" shown
   - 7  (==7):  21=[41] restrooms, 36=[217] Bonnie variant

3. **Visual confirmation.** All candidate images dumped to PNG,
   brightened and inspected:
   - img_83: shelf of spare heads + endoskeleton staring from the table
     + EMPLOYEES ONLY door -- THE backstage.
   - img_0 vs img_49 vs img_540: the SAME W.Hall-corner room, three
     poster states ("LET'S PARTY!" / "RULES FOR SAFETY" / golden).
   - img_62: supply closet with one hanging bulb (was misread as
     "restrooms moonlight" before).
   - img_41: restrooms (pizza posters + restroom pictogram).
   - img_66/211/338/240/553: cove closed / peek / out / empty /
     "IT'S ME" sign.

## Code changes (minimal diff)

- include/PakAssets.h: enum remapped to the canonical table,
  CAMFEED_NONE = -1 sentinel added, canonical variant handles preserved
  for the graphics queue (540/571/354/205/553).
- src/GameRender.cpp:
  - DrawFrame / DrawFrameFit / DrawInstance / DrawBentInstance guards
    changed from "<= 0" to "< 0" -- handle 0 is a REAL image (img_0,
    CAM 2B), only -1 means "nothing".
  - CamFeedFor: kitchen returns CAMFEED_NONE; RenderCamera feed init
    -1 and draw branch "feed >= 0" (img_0 must draw).
  - Pirate cove: COVE[4] = {66, 211, 338, 240}, clamp 0..3. The old
    {211, 338, 240} was one stage late -- the closed curtain was never
    shown at all.

## Queue notes discovered along the way (do NOT delete)

- anim 35 = death strobe sequence [301,291,303,293,294..300] (a35 queue).
- anim 44 = Chica west-hall hallucination sequence.
- anim 51 (power-out flicker) is 33 frames in the original; port had it
  right in v2.7.6+ (check % 33) but keep an eye on it.
- anim 46 = [304] / anim 47 = [305]: the actual power-out office pair
  per event groups 286/287/289/294 -- POWEROUT_OFFICE=476 (anim 56!)
  is SUSPECT for the power-out task; re-verify when that queue item
  comes up.
- 2B golden-poster (540) / Golden Freddy (571) and backstage Bonnie
  variants (354/205) are timed/conditional in the original (groups
  40..47, 69..72); wiring them up belongs to the easter-egg pass.
