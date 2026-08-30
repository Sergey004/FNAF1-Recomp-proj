============================================================================
FNAF1-Recomp -- docs/PERSPECTIVE.md
The bent office/camera view, reverse engineered from the original
FiveNightsatFreddys.exe -- and its v2.7.7 clean-room rasterizer.
============================================================================

WHAT THIS IS
------------
The original FNAF 1 office view (and every camera feed) is not a flat
image sliding on screen. The gameplay frame carries a THIRD-PARTY
Clickteam Fusion 2.5 extension object -- Andos' "Perspective" (source:
github.com/Andos/Perspective) -- which re-projects the whole scene layer
through a per-column cylindrical curve every frame. That is the famous
"bent camera" look: pan the office and the walls bow like you are
turning your head.

EVIDENCE CHAIN (how it was found)
---------------------------------
1. ctfak-cpp Frame Layout dump of FiveNightsatFreddys.exe
   (CF2.5 build 284, "Frame 1" 1600x720, 120 instances):
   the instance list contains
       objInfo 40  "Perspective"  Extension  (-22,-22)  layer 1
   -- an extension object named exactly like the .mfx, sitting one layer
   ABOVE the scene (Active 3 / office+feeds are layer 0), one layer
   BELOW the UI (static/REC/bezel are layer 2).
2. Extension List chunk of the same EXE: exactly 3 extensions are packed:
       Perspective.mfx, kcini.mfx, kcclock.mfx  (+cctrans.dll transition)
   kcini/kcclock are the INI and clock helpers; Perspective is the
   interesting one.
3. Events dump: the string "Perspective" appears in ZERO event groups.
   Nobody moves it, shows/hides it, or changes its parameters. It runs
   with its serialized setup for the whole night -- and since it is
   never hidden, it bends the camera feeds as well as the office.
4. Object bank chunk (raw chunk 8745, entry 40/196, type 32 = extension)
   decoded from the EXE. Its serialized EDITDATA (28 bytes, no ext
   header in this storage) is:
       00 00 00 00  2c 05 f2 02  00 00 00 00  2c 01 00 00
       00 00 00 00  04 00 00 00  00 00 00 00
   Decoded against the EDITDATA struct in Andos' Main.h:
       sx=0  sy=0  swidth=1324  sheight=754   (the object covers the
                                               1280x720 window + margin)
       Effect      = 0 = PANORAMA
       Direction   = 0 = HORIZONTAL
       DefaultZoom = 300   <-- the edge squeeze, in pixels
       DefaultOffset = 0, SineWaveWaves = 4 (unused by PANORAMA)
       PerspectiveDir = 0, resample = 0
5. Andos' Runtime.cpp, DisplayRunObject(), HWA path: the object grabs
   the ALREADY DRAWN render target and re-stretches it column by column
   into a temp surface, then blits it back opaque. Nothing else draws
   between: the flat scene stays visible wherever the bent copy does
   not cover it (the top/bottom wedges near the screen edges).

THE MATH (verbatim from Andos' PANORAMA / HORIZONTAL branch)
------------------------------------------------------------
for every 1-px column i in 0..1324:
    step(i) = (i - 1324/2) / (1324/3.1415) + 3.1415/2
    h(i)    = max(1, 754 + sin(step(i)) * 300 - 300)
    draw column i at x = -22 + i,
    y = -22 + 754/2 - h/2, height h   (pivot = screen y 355)

Column height: 754 px in the center column, 454 px at both object edges
(469.7 / 470.4 at the VISIBLE screen edges x=0 / x=1279, i.e. ~62%).
Note Andos uses the literal 3.1415 (truncated pi) -- kept as-is for
faithfulness. A sprite whose top edge sits at frame row fy0 is mapped
per column by:
    dest_y(fy) = 355 - h/2 + (fy + 22) * h / 754
(r = frame row + 22 because the object spans screen rows -22..732).
At the center column h = 754 and dest_y == fy exactly (1:1); the bend
grows toward both screen edges.

HISTORY: v2.7.6 (the strip port) and why it was replaced
--------------------------------------------------------
v2.7.6 emulated the extension MECHANIC: every layer-0 sprite was drawn
as ~1 quad per screen column straight from its pak texture (up to 1280
strips per sprite, flat pass + bent pass, ~2150 quads per office frame).
Faithful to the CF2.5 blitter -- and the worst possible way to do this
on a GPU:

  * each 1-px strip bilinear-samples half a texel into its neighbour on
    BOTH edges -> the seams bleed and the whole bent band reads smeared;
  * strip destinations snapped to integer pixels while the pan offset is
    fractional -> per-column width jitter (hairline gaps / doubles) and
    shimmer while panning;
  * ~1300-2150 DrawPrimitiveUP calls per frame for zero visual benefit.

v2.7.7 CLEAN-ROOM REWRITE (current)
-----------------------------------
Keep the EFFECT, drop the 1995-style mechanic. The curve is sampled at
an ~8-px column grid and handed to the GPU as ONE indexed-free triangle
list per sprite (SpriteBatch::DrawTriangles, one DrawPrimitiveUP):

  pass 1 (unchanged)  flat layer-0 sprites -- the original grab+blit
                      semantics: the flat scene stays visible in the
                      top/bottom wedges near the screen edges;
  pass 2 (new)        per sprite, a (cols-1) x 2 quad grid. Column x
                      vertices:
                          y_top(x) = 355 - h/2 + (fy0     + 22) * h/754
                          y_bot(x) = 355 - h/2 + (fy0+ih  + 22) * h/754
                          u(x)     = (x + panX - fx0) / iw   (linear!)
                      v = 0 at the top edge, v = ih/alignedH at the
                      bottom edge, so the sprite's full row range maps
                      through the same curve as v2.7.6 bit-exactly.

Properties:
  * the profile is linear-interpolated by the GPU between grid columns;
    at an 8-px step the max deviation from the true sine is ~0.006 px
    (curvature bound: 300*(pi/1324)^2 * step^2/8) -- invisible;
  * u(x) is exactly linear in x, so UV interpolation has ZERO error;
  * wide continuous quads => bilinear filtering behaves: sharp texture,
    no seam bleed, no per-pixel snapping, no shimmer;
  * office bg worst case: 162 columns -> 161 quads -> 966 vertices,
    ONE draw call (was 1280 strips / 1280 draw calls). Whole bent office
    scene: ~6 DrawTriangles calls + the small flat pass.
  * vertex scratch: one 2048-SpriteVertex buffer allocated in Init()
    (kBentVertCap, GameRender.cpp); DrawBentInstance never allocates.

Preview without a console: scripts/preview_perspective_clean.py renders
the EXACT pass-1+pass-2 math from the real img_39 (1600x720 office
panorama) at pan = 0 / 160 / 320 -- download/perspective_preview/*.png.
That output is the ground truth the mesh converges to (the script
evaluates the profile per 1-px column, i.e. denser than the 8-px grid).

WHERE THE NUMBERS CAME FROM (reproduction recipe)
-------------------------------------------------
scripts/extract_perspective_edata.py walks the EXE: PE '.reloc' end ->
pack header (0x7777, headerSize 32) -> 8 pack records -> chunk stream
("PAMU", build 284) -> chunk 8745 -> nested chunks (17476 header /
17477 name / 17478 ObjectCommon; nested chunks are flag=3 encrypted
with the standard MakeKey(editorFilename, name, copyright) RC4-variant
-- key strings here:
    name            = "Five Nights at Freddy's"
    editorFilename  = "C:\Users\Scott\Desktop\Five Nights\FiveNights-55.mfa"
    copyright       = "" )
-> ObjectCommon ext block at offset 116 -> EDATA above.

VERIFICATION AFTER BUILD
------------------------
1. APPLY_PATCH.bat must end with:
       [OK]   src\GameRender.cpp has the v2.7.7 clean-room Perspective
       [OK]   include\GameRender.h declares DrawBentInstance
       [OK]   include\SpriteBatch.h declares DrawTriangles
2. Rebuild Solution (VS2010). First Output line must be:
       === FNAF1-Recomp v2.7.7-cleanpersp built ... ===
3. In game: the office edges bend smoothly (doorways squeeze toward the
   screen edges, ~62% height at the extremes, center column untouched)
   with NO smearing, NO vertical hairlines and NO shimmer while panning;
   the camera feeds bulge the same way; map/bezel/labels/HUD stay
   straight.

PERF NOTE
---------
Bent office frame: flat pass (~7 quads) + ~6 DrawTriangles calls with
~1300 vertices total -- two orders of magnitude fewer CPU draw calls
than v2.7.6. If a slow console ever needs more headroom, coarsen the
grid by changing the 0.125f grid-step factor in DrawBentInstance (one
constant; 16 px halves the vertices, sag stays < 0.03 px).
