# RESTORE NOTICE — v2.7.4-RESTORED (read this first)

## What this package is

This is the GOOD v2.7.4 state of FNAF1-Recomp, restored exactly from the
user's own archive (the build the user confirmed as "everything was better").

It contains ALL of the working v2.7.x features, nothing newer:

* v2.7.2 — office / camera-feed drift (`GameRender::SetLookDir`, original
  implementation)
* v2.7.3 — sound pipeline (XDK 360 big-endian pass, normalized PCM16) and
  the sprite browser (LB+RB)
* v2.7.4 — correct original grain alpha (object-data derived), first-line
  build banner, byte-order probe
* ORIGINAL TEXTS — sprite-strip text system (`DrawStripText`,
  `MeasureStripText`, `DrawStripCentered`) using the game's real counter
  fonts from the pak (see docs/TEXT_SPRITES.md)

## What is NOT here (deprecated, do NOT re-apply)

* v2.8.0 — CRT shader experiment. BROKE GRAPHICS: the procedural CRT overlay
  (scanlines + vignette + grain) was drawn on top of EVERY screen, washing
  out the picture with fake noise.
* v2.8.1 — "crt-minipatch". BROKE THE CAMERA (SetLookDir / sprite browser
  were replaced with rough reconstructions that behave differently) and
  BROKE THE ORIGINAL TEXTS (the restored GameRender.h in that patch dropped
  DrawStripText / MeasureStripText / DrawStripCentered, so the real pak
  sprite fonts were no longer used).

If you still have v2.8.0 / v2.8.1 files, delete them. Do not merge.

## How to verify you are running THIS build

Run APPLY_PATCH.bat in the project root (next to src\ and include\) —
all lines must say [OK].

Then Rebuild in Visual Studio 2010 and check the FIRST line of the Output
window and the on-screen debug console. It must say:

    FNAF1-Recomp v2.7.4-RESTORED built <date> <time>

(The only difference from the user's archive is this banner label, added so
restored builds can never be confused with the broken v2.8.x builds again —
those also printed "v2.7.4", which caused the mix-up.)

## Rule going forward

This package is THE base. Every future patch must:

1. be diffed against exactly this tree,
2. never replace a file wholesale without checking it against the calls in
   main.cpp,
3. ship with a working rollback (keep the previous zip).
