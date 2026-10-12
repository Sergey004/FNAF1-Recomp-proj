# FNAF 2/3/4 runtime fonts (dump font banks + text objects)

Same extraction as docs/SL_TEXT.md (`CTFAK_DUMP_FONTS=1 ctfak-cpp -path <exe>`,
then join `"font"` handles against `objects[]` objectType 3 text paragraphs).
Heights are LOGFONT pixels (negative = char height; absolute value shown).

## The pattern (confirms the "mixed faces" suspicion — but structured)

Every game splits runtime text by FUNCTION, not randomly:
- HUD clock / cam labels / 8-bit captions: LCD Solid (all three games)
- the big win-screen "6"/"6:00 AM": Quartz MS (F2), 5Computers In Love 64 (F3)
- night-start card: OCR A Extended 43 bold (F2), 5CompIL 27 green (F3),
  Edwardian Script 64 "Night" (F4)
- storybook narrative (F4): Book Antiqua body + Castellar carved caps
- debug/legal/version microtext: Consolas/Tahoma everywhere

## FNAF2 — bank 14

LCD Solid 16b(h1)/35b(h16) · Lucida Sans Unicode 37b(h3) · OCR A Extended
43b(h5)/48(h7)/64(h8)/27(h12)/35(h9) · Quartz MS 123(h2) · Consolas
19(h6)/32(h4)/48(h10) · Verdana 27(h11) · Times New Roman 48b(h13)

Text objects (20): WARNING (Verdana 27) · v 1.033 (Consolas 19) · "Demo Ext"
(OCRA 48) · title fallback "Five Nights at Freddy's 2" (OCRA 64) · night card
"12:00 AM    7th Night" (OCRA 43b — what our office/nightstart shots should
use) · "CAM  12" + "flashlight" (LCD Solid 16b) · win "6" (Quartz MS 123) ·
"Game Over"/"Loading..." (Lucida SU 37b) · "SET MAX" (Consolas 32) · custom
preset names "Classic Horrors" (Consolas 48) · "Custom Mode" (TimesNR 48b) ·
dream whispers "it's him"/"err"/"you can't" (OCRA 27) · minigame captions
"Take cake to the children."/"Give Life."/"Hurray!" (LCD Solid 35b) · demo
thanks (OCRA 35).

## FNAF3 — bank 15

Most uniform: almost everything = **5Computers In Love** (11/21/24/27/37/37b/
48/48b/64 — handles 1,2,4,8,9,10,12,14,15,18). Exceptions: Consolas 21
warning · Lucida SU 37b "..." · Tahoma 37b "name" · LCD Solid 64b gray #7F7F7F
"f  i  n  d    m  e" (rare-screen). Notables: night card "12:00 AM    3rd
Night" 5CompIL 27 #C2E550 · win "6:00 AM" 64 · "Game Over" 37b · minigame
titles color-coded ("Chica's party" #FFFF57, "HAPPIEST DAY" #FFC0CB,
"stage01" #D2B48C, "Mangle's quest" white) · "exit" #666666 · "hold DEL to
reset all data" 5CompIL 11.

## FNAF4 — bank 11

Edwardian Script ITC 64(h3) · Castellar 24b(h4)/21(h6) · Book Antiqua
27(h0)/48(h1)/24b(h12) · Tahoma 29(h5)/16(h9) · Wasco Sans 27b(h7) · Times NR
35b(h10) · LCD Solid 32b(h2)

Text objects (13): WARNING Castellar 21 #FF0000 · "DEMO"/"Fun with balloon
boy" Castellar 24b #8B0000 · "v1.1" Tahoma 16 #8B0000 · "Night" card
Edwardian 64 #E6E6FA · instructions Book Antiqua 27/24b · "Fun with
Plushtrap" Book Antiqua 48 · "Click or Press ENTER to play." Wasco Sans 27b ·
8-bit "w  asd" LCD Solid 32b · "talk box" Times NR 35b (Fredbear plush
dialogue, set at runtime) · "PRESS ENTER" Castellar 21 red.

## Consequence for the port

The single embedded debug TTF is NOT dump-faithful anywhere these 50 objects
show; but their CONTENT and placement already come from the dump (or are
baked sprites we've rebound). Correct end-state: rasterize the ~12 face/size
combos actually used (F2: 9, F3: ~7, F4: 8, SL: table in SL_TEXT.md) into a
per-game bitmap-font set in the pak. Interim: pick nearest embedded face and
lock size+color from these tables — labeled DEVIATION until the raster pass.
