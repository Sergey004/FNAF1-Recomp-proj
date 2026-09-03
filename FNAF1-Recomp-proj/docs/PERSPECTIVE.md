============================================================================
FNAF1-Recomp -- docs/PERSPECTIVE.md
The bent office/camera view, reverse engineered from the original
FiveNightsatFreddys.exe -- and its v2.8 clean-room shader port.
============================================================================

WHAT THIS IS
------------
The original FNAF 1 office view (and every camera feed) is not a flat
image sliding on screen. The gameplay frame carries a THIRD-PARTY
Clickteam Fusion 2.5 extension object -- Andos' "Perspective" (source:
github.com/Andos/Perspective) -- which re-projects the whole scene layer
through a curve every frame. That is the famous "bent camera" look: pan
the office and the walls bow like you are turning your head.

The extension ships TWO renderers: a Windows/software `.mfx` that does a
per-column sine `Stretch()`, and an HWA pixel shader (`RPanorama.fx` /
`RPanorama.hlsl`) that does a parabola re-projection. The **Windows/Steam
build of FNAF 1 uses the HWA shader**, so that is the authoritative
reference -- v2.6-v2.7.7 ported the *software* sine band by mistake; v2.8
ports the real shader.

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
   decoded from the EXE. Its serialized EDITDATA is:
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
5. The HWA path (Windows/RPanorama.fx) does NOT do per-column columns:
   it `readFrameToTexture`-grabs the already drawn window (plus the 22 px
   margin = 1324x754) and draws it back at the object rect through a
   ps_2_0 pixel shader. The shader re-samples vertically per column with a
   PARABOLA, so the whole window is re-projected 1:1 with no flat "wedges"
   -- the source's top/bottom are simply cropped at the screen edges.

THE MATH (verbatim from RPanorama.fx, HORIZONTAL / pDir == 0)
-------------------------------------------------------------
Let u,v be the pixel's object-normalized coords (0..1 over 1324x754),
pivot = 0.5 (object center), fPixelHeight = 1/754.

    fB    = 1.0 - (zoom / 754)                 // zoom = 300 -> 0.621
    fC(u) = max(0.02, 1.0 + (fB - 1.0) * 4.0 * (u - 0.5)^2)
    src.y = (v - pivot) * fC + pivot
    src.x = u
    color = tex2D(capture, src -> window_space)

For HORIZONTAL the x coordinate passes through untouched; the vertical
position is scaled by fC about the pivot and re-centered. Since the object
only spans -22..1302 x / -22..732 y over a 1280x720 window, the visible
window samples the parabola over u in [22/1324, 1302/1324]: fC = 1 at the
center column, ~0.628 at the visible left/right edges, so edge columns read
only the central ~63% of the source band and stretch it to the full window
height (the office appears to bulge). Center column is 1:1.

HISTORY: v2.6-v2.7.7 (the software path) and why it was replaced
----------------------------------------------------------------
v2.6/v2.7.6/v2.7.7 implemented the SOFTWARE `.mfx` mechanic instead: a
per-column sine band (edges squeezed 754 -> 454 px, holding the flat scene
visible in the top/bottom "wedges" the shader never produces). That model
is a per-sprite triangle grid and does not match the Windows/Steam build.
v2.8 drops it entirely for the shader.

v2.8 CLEAN-ROOM SHADER PORT (current)
-------------------------------------
The scene is captured off-screen and re-projected by a clean-room ps_3_0
pixel shader, mirroring the original HWA flow:

  1. `SpriteBatch::BeginSceneCapture` switches the render target to a
     1280x720 EDRAM surface and clears it;
  2. the flat layer-0 scene (office bg / fan / doors / panels, or the
     camera feed) draws into that target;
  3. `EndSceneCapture` resolves the EDRAM capture into a sampleable texture
     and restores the back buffer;
  4. `DrawPerspective` draws a full-screen quad (whose UVs are the
     object-normalized coords) through the parabola shader above;
  5. layers 2/3 (static, REC, flash, bezel, labels, map, HUD) draw flat
     on top and stay straight.

The shader source lives in `SpriteBatch.cpp` (`kPanoramaPS_HLSL`), compiled
at runtime with `D3DXCompileShader(ps_3_0)` like the sprite shaders, and the
three tuner knobs feed its `gParams` constant register.

The device's auto depth-stencil is DISABLED (main.cpp): the game never
writes depth, and the EDRAM would otherwise overflow -- back buffer +
capture target nearly fill the 10 MB EDRAM already.

WHERE THE NUMBERS CAME FROM (reproduction recipe)
-------------------------------------------------
scripts/extract_perspective_edata.py walks the EXE: PE '.reloc' end ->
pack header (0x7777, headerSize 32) -> 8 pack records -> chunk stream
("PAMU", build 284) -> chunk 8745 -> nested chunks (17476 header /
17477 name / 17478 ObjectCommon; nested chunks are flag=3 encrypted
with the standard MakeKey(editorFilename, name, copyright) RC4-variant).
-> ObjectCommon ext block at offset 116 -> EDATA above.

VERIFICATION AFTER BUILD
------------------------
1. Rebuild the Solution (VS2010). The boot log must include the build
   banner, and SpriteBatch::Init must not report a "Panorama PS" or
   "Perspective capture RT" failure (any failure is surfaced as a
   full-screen message box instead of a black screen).
2. In game: the office edges bend smoothly -- center column 1:1, the
   visible screen edges re-project the central ~63% band, no flat wedges,
   no smearing, no hairlines, no shimmer while panning; the camera feeds
   bulge the same way; map/bezel/labels/HUD stay straight.

PERF NOTE
---------
The panorama is now ONE render-target capture + ONE resolve + ONE
full-screen shader pass per frame, independent of how many sprites the
scene contains (the old path cost ~6 triangle-grid draws + a flat pass).
The capture/resolve is the only fixed cost; the parabola runs per-pixel on
the GPU.