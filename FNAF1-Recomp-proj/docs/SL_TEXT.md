# SL runtime text: font bank + text-object map (from the SL dump)

Chunk 26215 (font bank) of `SisterLocation.exe`, dumped via
`CTFAK_DUMP_FONTS=1 ctfak-cpp -path SisterLocation.exe` (env-gated dump added
to the ctfak-cpp main.cpp, both exe and raw paths). Text objects reference the
bank by HANDLE (the JSON `"font"` field on a paragraph), not by index.

## Font bank (13 entries)

| handle | face                 | height | weight | charset |
|--------|----------------------|--------|--------|---------|
| 0      | Tahoma               | 13     | 400    | 1       |
| 1      | Tahoma               | 27     | 700    | 0       |
| 2      | Trebuchet MS         | 18     | 400    | 0       |
| 3      | Terminal             | 33     | 700    | 255(OEM)|
| 4      | Terminal             | 25     | 700    | 255(OEM)|
| 5      | Terminal             | 25     | 400    | 255(OEM)|
| 6      | Terminal             | 33     | 400    | 255(OEM)|
| 7      | Tw Cen MT Condensed  | 120    | 400    | 0       |
| 8      | Times New Roman      | 27     | 700    | 0       |
| 9      | OCR A Std            | 18     | 400    | 0       |
| 10     | Terminal             | 12     | 400    | 255(OEM)|
| 11     | Times New Roman      | 19     | 700    | 0       |
| 12     | Fixedsys             | 24     | 400    | 0       |

Families in practice: **Terminal/OEM** (the classic pixel-console look:
counters, "CAM07", custom-night labels, 8-bit help), **OCR A Std 18** (Hand
Unit captions / room labels / instructions), **Tahoma** (tiny legal lines 13px;
bold 27 = the Extras character name), **Times NR bold** (custom-night titles),
**Fixedsys 24** ("OXYGEN" gauge label), **Tw Cen MT Condensed 120** ("FAKE
ENDING" end card text).

## Text objects (objectType 3) — font/color/box/initial value

- obj 7   `String`           Tahoma 13      #7B7BC0 478x17   "Press DELETE now to erase all data."   (frame 0 warning prank line — part of the gag screen)
- obj 30  `String`           Tahoma 13      #0000FF 434x17   "Copyright © 2016 Scott Cawthon"      (title, parked off-screen until events show it)
- obj 32  `String 2`         Tahoma 13      #0000FF  78x23   "v 1.121"                               (title version line)
- obj 214 `String`           Terminal 33 b  #3CB371 135x135  "%"   (Breaker Room readout — the percent IS runtime text, dump-authentic)
- obj 241 `String 2`         OCR A Std 18   #FFFFFF 342x32   "Obsv.2"  (room caption, overridable by act #88)
- obj 259 `String 3`         OCR A Std 18   #E6E6FA 610x63   "Press SPACE to play\r\nmascot response audio." (Breaker/PS instruction)
- obj 415 `String 2`         Tahoma 13      #0000FF  51x17   "v 1.0"
- obj 420 `String 3`         Tahoma 27 bold #FFFFFF 260x85   "Baby"  (THE Extras character-name plate; act #88 rewrites it: Baby/Ballora/Funtime Freddy/Funtime Foxy/Ennard — frame_18 events g96)
- obj 473 `String`           Trebuchet 18   #7F7F7F 424x3376 credits roll body (frame 23)
- obj 480 `String 2`         TwCenMTCond120 #7F7F7F 796x147  "FAKE ENDING"
- obj 553 `String`           Terminal 25    #FFFFFF          8-bit minigame controls help
- obj 559 `String`           Terminal 25 b  #FFFFFF 280x75   "CAM07"
- obj 653 `String 2`         Terminal 33    #FFFFFF 445x68   "Custom Night" (frame 29 header — runtime text confirmed)
- obj 666 `night title`      TimesNR 27 b   #FFFFFF 257x35   "Revenge" (frame 29/31)
- obj 669 `Difficulty name`  TimesNR 27 b   #FFFFFF 257x35   "Easy"
- obj 686 `String`           Terminal 12    #FFFFFF 150x28   "Yenndo" (custom menu name labels)
- obj 687 `How to play`      TimesNR 19 b   #FFFFFF 750x58   "Move the mouse cursor over a character…"
- obj 696 `String`           Fixedsys 24    #FFFFFF 280x44   "OXYGEN" (frame 31 custom level)

## Consequences for the rebind wave

- Runtime text IS dump-authentic on: Extras name plate (Tahoma 27 bold),
  Breaker "%" (Terminal 33 bold green #3CB371) + instruction line (OCR A Std
  18), room captions "Obsv.2"-style (OCR A Std 18), credits (Trebuchet 18 gray
  + TwCen 120 "FAKE ENDING"), the whole Custom-menu stack (Terminal 33 /
  TimesNR / Terminal 12 / Fixedsys 24), title legal+version (Tahoma 13 blue),
  warning prank line (Tahoma 13 #7B7BC0).
- Port's TextRenderer (single embedded font) stands in for ALL of these for
  now — font-size/color/box from this table; exact raster faces (Terminal OEM,
  OCR A Std) would need a font-rasterize pass into the pak (follow-up).
- Everything else on the stub-text kill list stays kill: those frames have no
  text objects at all.
