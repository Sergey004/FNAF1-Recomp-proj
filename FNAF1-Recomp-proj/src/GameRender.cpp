/**
 * Five Nights at Freddy's 1 — Recompilation
 * GameRender.cpp: all screens drawn from fnaf1.pak assets
 *
 * v2.6 — disclaimer fix: the original warning frame has NO static object
 * (pure black bg + img_605 only), the overlay is gone.
 * (v2.5 — full asset pass. Every handle/position below comes from the
 * CTFAK JSON export of the original exe: banks.json + application.json,
 * verified against the 605 decoded pak PNGs.)
 *
 *  Disclaimer  "Frame 17" : img_605 IS the warning text image @(388,242).
 *  Title       "title"    : logo img_444 @(172,68), New Game img_448
 *                           @(275,420), Continue img_449 @(275,492),
 *                           ">>" img_450 @(132,493), stars img_432,
 *                           6th Night img_443 @(285,571), Custom img_526,
 *                           Night img_475 @(174,512), (c) img_433 @(1044,686),
 *                           v 1.132 img_588 @(26,682), bg img_431 + rare lit
 *                           flickers 440/441/442. (img_572 'demo' @ (171,292)
 *                           exists in the data but is NEVER drawn: the 'DEMO?'
 *                           counter stays 0 in this full-game Steam build --
 *                           see docs/EXE_VERSION.md.)
 *  Night cards "what day" : 12:00 AM / Nth Night cards at (646,318):
 *                           n1=453 n2=454 n3=472 n4=473 n5=474 n6=446 n7=538
 *                           (446 is the SIXTH night card!), white-noise
 *                           blip frames [4,6,8,9,10,21,22,23,25] behind.
 *  Office      "Frame 1"  : 1600x720 scene viewed through a 1280x720 pan
 *                           window (0..320, left stick). bg img_39, string
 *                           lights img_608 @(0,-78), fan 57/59/60 @(868,400),
 *                           pumpkin 628..635 @(734,485), doors 103/102
 *                           @(72,-1) and 119/118 @(1270,-2), button panels
 *                           122/124/125/130 @(48,390) and 134/135/131/47
 *                           @(1546,400) (state table from the event script),
 *                           flip-up bar img_156 @(442,691).
 *  Office HUD (screen-fixed counters): "Power left:" img_207, digits VAR14
 *                           strip + img_208 "%", "Usage:" img_189 + usage
 *                           bars (tinted img_23 solid-white frame),
 *                           "AM" img_251 @(1198,31) above CLOCK-strip hour
 *                           digits @(1185,59), "Night" img_447 + number
 *                           during the phone call, MUTE CALL img_481.
 *  Camera                 : bezel img_11, presence-aware feeds (table in
 *                           PakAssets.h), location label images @(832,292),
 *                           AUDIO ONLY img_42 @(384,69) on kitchen, map
 *                           img_164 + blinking button img_166/167, flip
 *                           down bar img_162 @(589,599).
 * v2.7.2 — camera feed drift: room renders are 1600x720 inside a
 *                           1280x720 window; the feed slowly sine-drifts
 *                           across the full 320 px slack like the original
 *                           (office pans by stick, cams drift on their own).
 * v2.7.6 — PERSPECTIVE PORT. The v2.7.2 note "the fisheye look is baked
 *                           into Scott's renders, not an engine effect" was
 *                           HALF WRONG: the renders are wide-angle, but the
 *                           strong bend is an ENGINE effect. Frame 1 carries
 *                           Andos' Perspective.mfx object (objInfo 40,
 *                           layer 1) with serialized settings PANORAMA /
 *                           HORIZONTAL / Zoom=300 / rect (-22,-22)
 *                           1324x754 -- extracted byte-exact from the
 *                           original EXE, see docs/PERSPECTIVE.md. No event
 *                           ever touches it, so it re-projects EVERYTHING
 *                           on layer 0 (office scene AND camera feeds)
 *                           through a per-column curve: center column full
 *                           height, edges squeezed 754->454 px. Layer 2+ UI
 *                           (static, REC, bezel, labels, map, HUD) is drawn
 *                           AFTER it and stays straight. Ported as the
 *                           v2.7.7 clean-room triangle mesh in
 *                           DrawBentInstance() (one ~8-px grid per sprite).
 * v2.7.7 - cleanpersp: the v2.7.6 strip rasterizer (1-px quads, integer
 *                           snapping, ~2150 DrawPrimitiveUP/frame) smeared
 *                           and tore the view. Same curve, new rasterizer:
 *                           one continuous ~8-px triangle grid per sprite
 *                           through SpriteBatch::DrawTriangles -- the GPU
 *                           interpolates the profile (max sag ~0.006 px),
 *                           no seams, ~1 draw call per bent sprite.
 *                           (v2.7.6 emitted 1-px strips: same math, but
 *                           bilinear seam bleed + pixel snapping smeared
 *                           and tore the view.)
 *  6 AM        "next day" : digit images "5"=350 "6"=351 "AM"=352;
 *                           nights 5/6/7 show paycheck 210 / overtime 522 /
 *                           termination 523 full screens ("the end" frames).
 *  Game over   "gameover" : backdrop img_358.
 *
 * v2.7.11 - persptune: the three PANORAMA constants (Zoom=300 / pivot
 *                           Y=355 / arc=3.1415) are LIVE knobs now. L3+R3
 *                           in game opens the tuner: DPad selects a knob,
 *                           Up/Down adjusts, A = fast step, Y = reset knob;
 *                           exiting prints "PERSP FINAL ..." to the log +
 *                           debug console for baking. Defaults stay the
 *                           exact serialized EDATA values -> bit-identical
 *                           to v2.7.10 until you touch it.
 *
 * Counter-font strips: the pak's dynamic digits are 11 identical strip sets
 * whose glyph order is  0 1 2 3 4 5 6 7 8 9 - + . e  (verified visually).
 */

#include "GameRender.h"
#include "XdkCompat.h"   // Snprintf — XDK CRT predates C99 snprintf
#include "PakAssets.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "PakLoader.h"
#include "MenuSystem.h"
#include "Game.h"
#include <cstdio>
#include <cstring>
#include <math.h>       // cosf — camera feed drift
#include <cstdlib>

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#include <d3d9.h>
typedef D3DDevice D3DDeviceX;
#else
struct IDirect3DDevice9; typedef IDirect3DDevice9 D3DDeviceX;
#endif

namespace fnaf {

static const f32 SCREEN_W = 1280.0f;
static const f32 SCREEN_H = 720.0f;
static const f32 OFFICE_PAN_MAX = 320.0f;   // 1600 - 1280

// ------------------------------------------------------------
// v2.7.6 PERSPECTIVE PORT -- the exact serialized settings of the
// "Perspective" object (objInfo 40, handle 40, type 32 = extension) from
// the original FiveNightsatFreddys.exe. The EDITDATA blob decoded to:
//   sx=0 sy=0 swidth=1324 sheight=754  (instance at (-22,-22):
//                                      1280+44 x 720+34, screen + margin)
//   Effect=0 PANORAMA, Direction=0 HORIZONTAL,
//   DefaultZoom=300, DefaultOffset=0, SineWaveWaves=4 (unused here)
// The Events dump has ZERO references to the object: nobody moves, shows,
// hides or re-parameterizes it -- it bends the whole scene layer forever.
static const f32 PERSP_OBJ_X  = -22.0f;     // object origin on screen
static const f32 PERSP_OBJ_W  = 1324.0f;    // 1280 + 2*22 margin columns
static const f32 PERSP_OBJ_H  = 754.0f;     // 720 + 34 margin rows
static const f32 PERSP_ZOOM   = 300.0f;     // EDATA DefaultZoom (edge squeeze)
static const f32 PERSP_PI     = 3.1415f;    // Andos' literal (truncated pi,
                                            // kept for bit-faithfulness)
static const f32 PERSP_CENTER_Y = 355.0f;   // -22 + 754/2, pivot of the bend

// Triangle-grid vertex budget for ONE bent instance. Worst case is the
// 1600-px office bg clipped to the 1280 window at the ~8-px step:
// 162 columns -> 161 quads -> 966 vertices. 2048 gives ~2x headroom.
static const int kBentVertCap = 2048;

// v2.7.11 PERSPECTIVE TUNER -- live-tunable copies of the three serialized
// PANORAMA constants above. Initialized 1:1 with v2.7.10 (bit-identical
// defaults): the bend only changes when the tuner (L3+R3 in game) adjusts
// them. To bake a tuned look, paste the printed "PERSP FINAL" numbers over
// the three initializers:
//   ZOOM     300.0  -- column-height swing. Center column is ALWAYS full
//                      height (754); edges are 754-ZOOM px. + = bulge
//                      (fish-eye, edges squeeze in), 0 = dead flat pan,
//                      - = pincushion (edges stretch OUT, concave look).
//   CENTER_Y 355.0  -- vertical pivot the columns expand from (object
//                      center: -22 + 754/2). Move up/down to bias where
//                      the bend pinches.
//   ARC      3.1415 -- sine arc span across the 1324-px object. Equals
//                      Andos' truncated pi: edges reach sin=0 exactly.
//                      Bigger (up to ~6.28) = sine dips negative at the
//                      edges -> hard fisheye with collapsed corners;
//                      smaller = gentler, flatter falloff.
static f32 g_perspZoom    = PERSP_ZOOM;      // 300.0
static f32 g_perspCenterY = PERSP_CENTER_Y;  // 355.0
static f32 g_perspArc     = PERSP_PI;        // 3.1415

// ------------------------------------------------------------
//  Real animation frame tables (image handles from the game data)
// ------------------------------------------------------------

// Title/office static cycle — object "static" anim 0 (8 frames)
static const int STATIC_FRAMES[8] = { 18, 20, 12, 13, 14, 15, 16, 17 };

// White-noise blip flash cycle (object "blip flash", what-day/died frames)
static const int BLIP_FRAMES[9] = { 4, 6, 8, 9, 10, 21, 22, 23, 25 };

// Menu flicker overlays (object "blip flash 2" on the title frame)
static const int MENU_BLIP[7] = { 430, 434, 435, 436, 437, 438, 439 };

// Night start cards ("what day" frame, instance (646,318)); index = night
static const int NIGHT_CARDS[8] = { 0, 453, 454, 472, 473, 474, 446, 538 };

// ---- v2.7.8 OFFICE_FX_TABLES -- dug out of the original's Frame Items
// animation table + event script (full evidence: docs/OFFICE_FX.md).
// CF2.5 frame duration = (100 / animSpeed) / 50 seconds:
// speed 50 -> 0.040 s/frame, speed 70 -> 0.0286 s/frame.
// obj 59/60 "left/right door" anims a12 (closing) / a14 (opening), 16
// frames each; the events cut the close at frame 12 -> static a13, the
// tail frames are the same slab, so playing the full table lands on it.
static const int DOOR_L_CLOSE[16] = {103, 88,105, 89, 91, 92, 93, 94, 95, 96, 97, 98, 99,100,101,102};
static const int DOOR_L_OPEN[16]  = {102,101,100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 89,105, 88,103};
static const int DOOR_R_CLOSE[16] = {119,104,121,106,107,108,109,110,111,112,113,114,115,116,117,118};
static const int DOOR_R_OPEN[16]  = {118,117,116,115,114,113,112,111,110,109,108,107,106,121,104,119};
static const f32 DOOR_FRAME_T = 0.040f;
// Foxy sprint frames live in FOXY_RUN[33] below (Active 3 anim 51,
// verified: 33 frames @ speed 65, backTo 31). The old 25-frame table
// here was actually anim 52 (the kill) -- v2.7.12 fix.
// obj 46 "Active 5": white flash while the tablet goes up (blip==1,
// event group 16); one-shot, destroyed on end (group 17).
static const int FLASH_SEQ[9] = {23,4,25,6,8,9,10,21,22};
static const f32 FLASH_FRAME_T = 0.02857f;   // speed 70
// obj 73 "flip down 2": dark uncover wipe when the tablet drops
// (put down==1, event group 322); starts black, dissolves clear.
static const int WIPE_SEQ[11] = {141,140,139,138,137,136,133,132,144,46,142};
static const f32 WIPE_FRAME_T = 0.040f;

// obj 68 "panel": the tablet RAISE animation. Event group 130 creates
// the object the moment the flip bar is clicked (with the
// CAMERA_VIDEO_LOA static burst); its 11-frame anim covers the office
// bottom-up and ONLY when it ends does group 133 destroy the panel and
// commit viewing := last clicked (group 18) -> blip == 1 -> group 16
// fires the white flash OVER the freshly appeared feed. The images are
// exactly the WIPE_SEQ stack played BACKWARDS: 142 = tablet parked on
// the desk (bottom edge + red/green LEDs), 141 = fully risen. All
// frames are native 1280x720 window-space art (straight edges baked
// in) -> drawn FLAT like the wipe, not through the panorama curve.
static const int RAISE_SEQ[11] = {142,46,144,132,133,136,137,138,139,140,141};
static const f32 RAISE_FRAME_T = 0.040f;   // speed 50

// v2.7.4: the ORIGINAL grain alpha comes from the object data, not guesswork.
// Every fullscreen static object in the game carries inkEffect=1 (semi-
// transparency) with inkEffectValue = 100; Fusion stores the transparency
// COEFFICIENT (0 = opaque, 255 = invisible), so the runtime alpha is
// (255-100)/255 = 0.608. This IS the "Steam screenshot effect": dark
// pre-rendered art + one animated gray-noise sprite at ~61% opacity.
// The 'mute call' button uses coeff 50 -> alpha 205/255 = 0.804.
// (img_11 'frame' @ (0,-1) is 100% transparent A=0 -- it draws NOTHING;
// the vignette is baked into the pre-rendered room art itself.)
//
// v2.7.5 OVERLAY VERDICT (frame layout + events + pixel dump, see
// docs/OVERLAY_MAP.md): the Frame 1 static object (obj 42 "Active",
// anim [18,20,12..17] @ speed 100, ink 1/100) has VisibleAtStart=FALSE
// and the event script only ever toggles it with the monitor:
//   group 81: viewing == 0 -> HIDE static + REC + frame/bezel + white flash
//   group 82: viewing >  0 -> SHOW them again
// So the animated static belongs to the MONITOR ONLY. The office has NO
// animated noise: its grain is baked into the pre-rendered room art
// (img_39, mean luma ~10/255). The title's "static" (obj 2) uses ink=9
// (different effect id, value 0) and is always visible there.
static const f32 STATIC_ALPHA    = 155.0f / 255.0f;  // coeff 100
static const f32 MUTECALL_ALPHA  = 205.0f / 255.0f;  // coeff 50

// Desk pumpkin (Halloween easter egg, object "Active 28", 7 frames at
// 143x150) -- in the frame data it is gated by Date&Time/'month'/'day'
// objects, so it only appears on Halloween
static const int PUMPKIN_FRAMES[7] = { 628, 630, 631, 632, 633, 634, 635 };

// Power-out office: anim 56 base (476) + anim 46 dark (304) /
// anim 47 Freddy face lit (305). v2.7.11 used the 33-frame FOXY RUN
// sequence (anim 51) here by mistake — see docs/AI_MECHANICS.md §9.
static const int POWEROUT_OFFICE = 476;
static const int POWEROUT_DARK   = 304;   // anim 46
static const int POWEROUT_LIT    = 305;   // anim 47 (Freddy face)

// Foxy sprint down the West Hall — Active 3 anim 51 (33 frames,
// speed 65 -> 39 FPS, backTo 31). Shown on the office view while
// Foxy.foxyRunning (group 40 / events render note).
static const int FOXY_RUN[33] = {
    241, 241, 241, 340, 244, 245, 246, 247, 248, 250, 280, 282, 283, 284, 285,
    286, 287, 288, 289, 290, 292, 302, 306, 327, 329, 330, 331, 332, 333, 334,
    335, 336, 337
};

// Jump scare sequences — REAL kill animations from Active 3 (handle 44),
// verified from the original (docs/AI_MECHANICS.md §9):
//   anim 35 Bonnie kill  11 frames @ speed 75 (45 FPS)
//   anim 44 Chica kill   16 frames @ speed 99 (~60 FPS)
//   anim 52 Foxy kill    25 frames @ speed 50 (30 FPS)
//   anim 65 Freddy kill  31 frames @ speed 50 (30 FPS)
// v2.7.11 and earlier mistakenly used the door-window poses (34/43,
// single frames 225/227) for Bonnie/Chica and 25 FPS timing everywhere.
static const int SCARE_FREDDY[31] = {
    519, 485, 521, 489, 490, 491, 493, 495, 496, 497, 498, 499, 500, 501,
    502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515,
    516, 517, 518
};
static const int SCARE_FOXY[25] = {
    413, 242, 415, 243, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405,
    406, 407, 408, 409, 410, 411, 412, 412, 412, 412, 412
};
static const int SCARE_BONNIE_KILL[11] = {
    301, 291, 303, 293, 294, 295, 296, 297, 298, 299, 300
};
static const int SCARE_CHICA_KILL[16] = {
    279, 65, 281, 69, 216, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 239
};
// Door-window poses (anims 34/43) — shown by the office view when the
// door light reveals Bonnie/Chica, NOT during the kill.
static const int SCARE_BONNIE_WINDOW = 225;   // Active 3 anim 34
static const int SCARE_CHICA_WINDOW  = 227;   // Active 3 anim 43
static const int GOLDEN_FREDDY       = 571;   // Active 3 anim 75

// IT'S ME hallucination flash (frame 3 object "Active 21" anim 0)
static const int ITSME_FRAMES[4] = { 525, 543, 520, 544 };

// Camera location label images, indexed by CameraId (CAM_1A..CAM_7);
// text verified from the decoded images.
static const int CAM_LABELS[11] = {
    IMG_LOC_SHOWSTAGE, IMG_LOC_DINING, IMG_LOC_COVE,
    IMG_LOC_WESTHALL,  IMG_LOC_WHALL_COR, IMG_LOC_CLOSET,
    IMG_LOC_EASTHALL,  IMG_LOC_EHALL_COR, IMG_LOC_BACKSTAGE,
    IMG_LOC_KITCHEN,   IMG_LOC_RESTROOMS
};

// ------------------------------------------------------------
//  Counter-font sprite strips (the game's real digit fonts)
//  Verified glyph order: 0 1 2 3 4 5 6 7 8 9 - + . e
// ------------------------------------------------------------
struct StripGlyph { int handle; int w; int h; };
struct SpriteStrip {
    const char* tag; const StripGlyph* glyphs; int count; int uniform;
    int baseIdx;   // alphabet index of glyphs[0] (VAR58 starts at '1')
};

static const StripGlyph STRIP_TINY_G[14] = { {265,8,10},{266,8,10},{267,8,10},{268,8,10},{269,8,10},{270,8,10},{271,8,10},{272,8,10},{273,8,10},{274,8,10},{275,8,10},{276,8,10},{277,8,10},{278,8,10} };
static const StripGlyph STRIP_SMALL_G[14] = { {24,12,18},{26,12,18},{27,12,18},{28,12,18},{29,12,18},{30,12,18},{31,12,18},{32,12,18},{33,12,18},{34,12,18},{35,12,18},{36,12,18},{37,12,18},{38,12,18} };
static const StripGlyph STRIP_XSMALL_G[14] = { {187,14,17},{191,14,17},{192,14,17},{193,14,17},{194,14,17},{195,14,17},{196,14,17},{197,14,17},{198,14,17},{199,14,17},{200,14,17},{201,14,17},{202,14,17},{203,14,17} };
static const StripGlyph STRIP_MID_G[14] = { {151,19,23},{152,19,23},{153,19,23},{154,19,23},{158,19,23},{159,19,23},{178,19,23},{179,19,23},{180,19,23},{181,19,23},{182,19,23},{183,19,23},{184,19,23},{185,19,23} };
static const StripGlyph STRIP_CAM_G[14] = { {414,15,29},{416,15,29},{417,15,29},{418,15,29},{419,15,29},{421,15,29},{422,15,29},{423,15,29},{424,15,29},{425,15,29},{426,10,29},{427,20,29},{428,8,29},{429,14,29} };
static const StripGlyph STRIP_CLOCK_G[14] = { {457,30,38},{458,30,38},{459,30,38},{460,30,38},{461,30,38},{462,30,38},{463,30,38},{464,30,38},{465,30,38},{466,30,38},{467,30,38},{468,30,38},{469,30,38},{470,30,38} };
// BIG42: 532='0' 590='1' 592='2' 594..600='3'..'9' 601='-' 602='+' 603='.' 604='e'
// (591 does not exist in the bank; 593 is the customize hint text image)
static const StripGlyph STRIP_BIG42_G[14] = { {532,22,42},{590,22,42},{592,22,42},{594,22,42},{595,22,42},{596,22,42},{597,22,42},{598,22,42},{599,22,42},{600,22,42},{601,15,42},{602,29,42},{603,11,42},{604,21,42} };
static const StripGlyph STRIP_NIGHT_G[14] = { {556,35,75},{557,35,75},{558,35,75},{559,35,75},{560,35,75},{561,35,75},{562,35,75},{563,35,75},{564,35,75},{565,35,75},{566,35,75},{567,35,75},{568,35,75},{569,35,75} };
// VAR58 has no '0' glyph — starts at '1' (baseIdx 1)
static const StripGlyph STRIP_VAR58_G[13] = { {570,31,58},{575,31,58},{577,31,58},{578,31,58},{579,31,58},{580,31,58},{581,31,58},{582,31,58},{583,31,58},{584,21,58},{585,39,58},{586,15,58},{587,29,58} };
static const StripGlyph STRIP_VAR25_G[14] = { {328,13,25},{353,13,25},{360,13,25},{361,13,25},{362,13,25},{363,13,25},{364,13,25},{365,13,25},{366,13,25},{367,13,25},{368,9,25},{369,17,25},{370,7,25},{371,12,25} };
static const StripGlyph STRIP_VAR14_G[14] = { {372,8,14},{373,8,14},{374,8,14},{375,8,14},{376,8,14},{377,8,14},{378,8,14},{379,8,14},{380,8,14},{381,8,14},{382,5,14},{383,10,14},{384,4,14},{385,7,14} };

static const char STRIP_ALPHABET[15] = "0123456789-+.e";

static const SpriteStrip STRIPS[] = {
    { "TINY   265-278  8x10",  STRIP_TINY_G,  14, 1, 0 },
    { "SMALL  24+26-38 12x18", STRIP_SMALL_G, 14, 1, 0 },
    { "XSMALL 187+191-203",    STRIP_XSMALL_G,14, 1, 0 },
    { "MID   151-185    19x23",STRIP_MID_G,   14, 1, 0 },
    { "CAM    414-429  15x29", STRIP_CAM_G,   14, 0, 0 },
    { "CLOCK  457-470  30x38", STRIP_CLOCK_G, 14, 1, 0 },
    { "BIG42  532+590-604",    STRIP_BIG42_G, 14, 0, 0 },
    { "NIGHT  556-569  35x75", STRIP_NIGHT_G, 14, 1, 0 },
    { "VAR58  570-587",        STRIP_VAR58_G, 13, 0, 1 },
    { "VAR25  328-371",        STRIP_VAR25_G, 14, 0, 0 },
    { "VAR14  372-385   8x14", STRIP_VAR14_G, 14, 0, 0 },
};
static const int STRIP_COUNT = (int)(sizeof(STRIPS)/sizeof(STRIPS[0]));

// Strips used by the HUD
static const SpriteStrip& STRIP_CLOCK = STRIPS[5];   // office hour digits
static const SpriteStrip& STRIP_VAR25 = STRIPS[9];   // menu night number
static const SpriteStrip& STRIP_VAR14 = STRIPS[10];  // power %/night digits

// Standalone text-block candidates (sprite browser, pages 1-6)
static const int LABEL_CANDIDATES[52] = { 42,50,54,70,71,72,73,74,75,76,77,78,79,189,207,209,212,213,214,251,352,420,433,445,446,447,448,449,453,454,455,456,471,472,473,474,475,477,480,481,524,526,530,531,533,534,535,537,538,572,588,593 };
// Every other small sprite (misc/letters/icons)
static const int SMALL_REST[82] = { 1,3,5,7,51,52,61,63,64,80,81,82,84,85,86,87,123,128,129,143,146,147,148,150,165,166,167,168,169,170,171,172,173,174,175,176,177,186,204,208,218,238,249,252,253,254,255,256,257,258,259,260,261,262,263,264,339,341,342,343,344,345,346,347,349,350,351,356,357,359,386,387,394,395,432,443,450,482,483,541,542,547 };

// Glyph index in a strip for a character, or -1 for space / unsupported.
static int StripCharIndex(char c) {
    const char* p = STRIP_ALPHABET;
    for (int i = 0; p[i]; ++i) if (p[i] == c) return i;
    return -1;
}

// Draw one sprite-strip string. x,y = pen origin (top of the line box);
// bottom-aligns glyphs inside the tallest cell of the strip.
void GameRender::DrawStripText(const SpriteStrip& strip, float x, float y,
                               const char* text, u32 color, float scale) {
    if (!m_batch || !text) return;
    int maxH = 0;
    for (int i = 0; i < strip.count; ++i) if (strip.glyphs[i].h > maxH) maxH = strip.glyphs[i].h;
    const float tracking = 1.0f * scale;
    float pen = x;
    for (const char* p = text; *p; ++p) {
        if (*p == ' ') { pen += 0.5f * maxH * scale; continue; }
        const int idx = StripCharIndex(*p) - strip.baseIdx;
        if (idx < 0 || idx >= strip.count) { pen += maxH * scale * 0.5f; continue; }
        const StripGlyph& g = strip.glyphs[idx];
        const float gw = g.w * scale, gh = g.h * scale;
        const float gy = y + (maxH * scale - gh);   // bottom align
        DrawFrame(g.handle, pen, gy, gw, gh, color);
        pen += gw + tracking;
    }
}

f32 GameRender::MeasureStripText(const SpriteStrip& strip, const char* text, float scale) const {
    if (!text) return 0;
    int maxH = 0;
    for (int i = 0; i < strip.count; ++i) if (strip.glyphs[i].h > maxH) maxH = strip.glyphs[i].h;
    const float tracking = 1.0f * scale;
    float w = 0;
    for (const char* p = text; *p; ++p) {
        if (*p == ' ') { w += 0.5f * maxH * scale; continue; }
        const int idx = StripCharIndex(*p) - strip.baseIdx;
        if (idx < 0 || idx >= strip.count) { w += maxH * scale * 0.5f; continue; }
        w += strip.glyphs[idx].w * scale + tracking;
    }
    return w;
}

GameRender::GameRender()
    : m_batch(0), m_text(0), m_pak(0)
    , m_time(0.0f), m_staticTime(0.0f), m_staticIndex(0)
    , m_lookDir(0.0f), m_panX(160.0f)
    , m_cacheCount(0), m_bentVerts(0)
{
    // v2.7.8 office FX state
    m_prevMonitor = m_prevDoorL = m_prevDoorR = false;
    m_doorT[0] = m_doorT[1] = -1.0f;
    m_doorClosing[0] = m_doorClosing[1] = false;
    m_flashT = m_wipeT = -1.0f;
    m_raiseT = -1.0f;      // v2.7.9: tablet raise
    m_prevCam = -1;        // v2.7.9: no settled monitor cam yet
    m_lastT = 0.0f;
}

void GameRender::Init(SpriteBatch* batch, TextRenderer* text, PakLoader* pak) {
    m_batch = batch; m_text = text; m_pak = pak;
    m_cacheCount = 0;
    if (!m_bentVerts) m_bentVerts = new SpriteVertex[kBentVertCap];
}

void GameRender::SetLookDir(f32 dir) {
    m_lookDir = dir;
}

void GameRender::Tick(f32 dt) {
    m_time += dt;
    m_staticTime += dt;
    // static cycle ~12 fps like the original's 8-frame animation
    if (m_staticTime >= 1.0f / 12.0f) {
        m_staticTime -= 1.0f / 12.0f;
        m_staticIndex = (m_staticIndex + 1) % 8;
    }
    // office pan window: stick right -> look right (scene moves left)
    m_panX += m_lookDir * 480.0f * dt;
    if (m_panX < 0.0f)          m_panX = 0.0f;
    if (m_panX > OFFICE_PAN_MAX) m_panX = OFFICE_PAN_MAX;
}

PakLoadedTexture* GameRender::Tex(const char* name) {
    if (!m_pak || !name) return 0;
    for (int i = 0; i < m_cacheCount; ++i) {
        if (strcmp(m_cache[i].name, name) == 0) return m_cache[i].tex;
    }
    PakLoadedTexture* t = m_pak->FindTexture(name);
    if (m_cacheCount < 64) {
        strncpy(m_cache[m_cacheCount].name, name, sizeof(m_cache[0].name) - 1);
        m_cache[m_cacheCount].name[sizeof(m_cache[0].name) - 1] = '\0';
        m_cache[m_cacheCount].tex = t;
        ++m_cacheCount;
    }
    return t;
}

void GameRender::DrawTex(const char* name, float x, float y, float w, float h, u32 color) {
    PakLoadedTexture* t = Tex(name);
    if (!t || !t->texture || !m_batch) return;
    // Clamp UVs to the ORIGINAL image size: the pak texture is allocated at
    // the 64-aligned size (e.g. 1600x736 for a 1600x720 image) and the extra
    // texels are padding. Sampling 0..1 would squeeze the padding into the
    // draw rect (a ~2% stretch on every unaligned image).
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x, y, w, h, 0.0f, 0.0f, u1, v1, color);
}

void GameRender::DrawFrame(int imgHandle, float x, float y, float w, float h, u32 color) {
    // v2.7.10: img_0 is a REAL asset (CAM 2B "LET'S PARTY!" corner), so
    // the invalid-handle guard is strictly negative now (CAMFEED_NONE=-1).
    if (imgHandle < 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", imgHandle);
    DrawTex(name, x, y, w, h, color);
}

// Fit a sprite inside a maxW x maxH box centered at (cx, cy), keeping the
// pak image's native aspect ratio (never upscaled beyond 4x).
void GameRender::DrawFrameFit(int imgHandle, float cx, float cy, float maxW, float maxH, u32 color) {
    if (imgHandle < 0) return;   // v2.7.10: img_0 is real, only -1 is invalid
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", imgHandle);
    PakLoadedTexture* t = Tex(name);
    if (!t || !t->texture) return;
    const f32 iw = (f32)(t->origWidth ? t->origWidth : 1);
    const f32 ih = (f32)(t->origHeight ? t->origHeight : 1);
    f32 scale = (iw / maxW > ih / maxH) ? (maxW / iw) : (maxH / ih);
    if (scale > 4.0f) scale = 4.0f;
    const f32 w = iw * scale, h = ih * scale;
    DrawTex(name, cx - w * 0.5f, cy - h * 0.5f, w, h, color);
}

// Hotspot-aware draw at the original instance position. scene=true subtracts
// the office pan offset (the office scene is 1600x720, the window 1280x720).
void GameRender::DrawInstance(int imgHandle, float ix, float iy, u32 color, bool scene) {
    if (imgHandle < 0) return;   // v2.7.10: img_0 is real, only -1 is invalid
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", imgHandle);
    PakLoadedTexture* t = Tex(name);
    if (!t || !t->texture) return;
    const PakHotspot hs = PakHotspotOf(imgHandle);
    const f32 x = ix - hs.x - (scene ? m_panX : 0.0f);
    const f32 y = iy - hs.y;
    // v2.7: clamp UVs to the ORIGINAL size (same fix as DrawTex). Sampling
    // 0..1 pulls the 64-aligned PADDING rows into the draw rect — for the
    // title/HUD text images (33px content in a 64px texture) that squeezed
    // the letters and painted white padding bands under them.
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x, y, (f32)t->origWidth, (f32)t->origHeight, 0.0f, 0.0f, u1, v1, color);
}

// ------------------------------------------------------------
// ------------------------------------------------------------
// v2.7.7 CLEANROOM PERSPECTIVE -- the same PANORAMA effect as the
// original extension, re-implemented the way it would be written for a
// GPU today instead of emulating the CF2.5 1-px Stretch() mechanic.
//
// Why v2.7.6 (faithful 1-px column strips) looked bad:
//   * every 1-px quad bilinear-samples its neighbours across the strip
//     seams -> the whole bent band reads smeared/soft;
//   * integer column snapping against the fractional pan offset ->
//     per-column width jitter (hairline gaps/doubles) while panning;
//   * ~1280 separate DrawPrimitiveUP strip calls per big sprite
//     (office scene ~2150 quads/frame) for zero visual benefit.
//
// The clean-room pass keeps the exact EFFECT math (constants above are
// the serialized EDATA of the original EXE) and changes only HOW it is
// rasterized:
//   1) the flat layer-0 pass underneath is unchanged -- that is the
//      original grab+blit semantics (flat scene stays visible in the
//      top/bottom wedges near the screen edges);
//   2) the bent copy is ONE triangle grid per sprite: an ~8-px column
//      grid whose vertices follow the PANORAMA profile. The GPU
//      interpolates the curve between grid columns (max sag ~0.006 px
//      at 8 px), bilinear filtering runs inside wide continuous quads,
//      and the office bg costs 161 quads / 1 draw call instead of
//      1280 strips / 1280 draw calls.
// Vertex positions are bit-identical to the strip math at every grid
// column:
//   h(x)       = max(1, 754 + sin(step) * 300 - 300)
//   step(x)    = (x + 22 - 662) / (1324 / 3.1415) + 3.1415 / 2
//   dest_y(fy) = 355 - h/2 + (fy + 22) * h / 754
// ------------------------------------------------------------

// PANORAMA column height at a screen-space x (exact original formula;
// v2.7.11 reads the three tuner knobs, defaults bit-identical).
static f32 BentHeightAt(f32 screenX) {
    const f32 ci   = screenX - PERSP_OBJ_X;
    const f32 step = (ci - PERSP_OBJ_W * 0.5f) / (PERSP_OBJ_W / g_perspArc)
                   + g_perspArc * 0.5f;
    f32 h = PERSP_OBJ_H + sinf(step) * g_perspZoom - g_perspZoom;
    if (h < 1.0f) h = 1.0f;
    return h;
}

void GameRender::DrawBentInstance(int imgHandle, float frameX, float frameY,
                                  u32 color, float panX) {
    if (imgHandle < 0 || !m_batch || !m_bentVerts) return;   // v2.7.10: img_0 is real
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", imgHandle);
    PakLoadedTexture* t = Tex(name);
    if (!t || !t->texture) return;

    const PakHotspot hs = PakHotspotOf(imgHandle);
    const f32 fx0 = frameX - hs.x;              // frame-space left of image
    const f32 fy0 = frameY - hs.y;              // frame-space top of image
    const f32 iw = (f32)(t->origWidth  ? t->origWidth  : 1);
    const f32 ih = (f32)(t->origHeight ? t->origHeight : 1);
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;

    // Visible screen span covered by the image (window = 0..1280).
    // Float end to end: nothing snaps to the pixel grid any more.
    const f32 leftPx = fx0 - panX;
    f32 xa = leftPx;              if (xa < 0.0f)     xa = 0.0f;
    f32 xb = leftPx + iw;         if (xb > SCREEN_W) xb = SCREEN_W;
    if (xb - xa < 0.5f) return;

    // ~8 px grid step, both endpoints exact.
    int cols = (int)((xb - xa) * 0.125f) + 2;
    if (cols < 2) cols = 2;
    const f32 stepX = (xb - xa) / (f32)(cols - 1);

    // dest_y(frameY) = 355 - h/2 + (frameY + 22) * h / 754, precomputed
    // for this sprite's top (fy0) and bottom (fy0 + ih) frame rows.
    const f32 topA = fy0 + 22.0f;
    const f32 botA = fy0 + ih + 22.0f;

    SpriteVertex* v = (SpriteVertex*)m_bentVerts;
    int n = 0;
    f32 xp = xa;
    f32 hp = BentHeightAt(xp);
    f32 ytp = g_perspCenterY - hp * 0.5f + topA * hp / PERSP_OBJ_H;
    f32 ybp = g_perspCenterY - hp * 0.5f + botA * hp / PERSP_OBJ_H;
    f32 up = (xp + panX - fx0) / iw * u1;

    for (int k = 1; k < cols; ++k) {
        const f32 xk = (k == cols - 1) ? xb : xa + stepX * (f32)k;
        const f32 hk  = BentHeightAt(xk);
        const f32 ytk = g_perspCenterY - hk * 0.5f + topA * hk / PERSP_OBJ_H;
        const f32 ybk = g_perspCenterY - hk * 0.5f + botA * hk / PERSP_OBJ_H;
        const f32 uk  = (xk + panX - fx0) / iw * u1;

        if (n + 6 > kBentVertCap) break;
        // quad xp..xk as two tris: TL,BL,TR + BL,BR,TR (cull is NONE)
        v[n].x = xp; v[n].y = ytp; v[n].z = 0.0f; v[n].w = 1.0f;
        v[n].u = up; v[n].v = 0.0f; v[n].color = color; ++n;
        v[n].x = xp; v[n].y = ybp; v[n].z = 0.0f; v[n].w = 1.0f;
        v[n].u = up; v[n].v = v1;  v[n].color = color; ++n;
        v[n].x = xk; v[n].y = ytk; v[n].z = 0.0f; v[n].w = 1.0f;
        v[n].u = uk; v[n].v = 0.0f; v[n].color = color; ++n;
        v[n].x = xp; v[n].y = ybp; v[n].z = 0.0f; v[n].w = 1.0f;
        v[n].u = up; v[n].v = v1;  v[n].color = color; ++n;
        v[n].x = xk; v[n].y = ybk; v[n].z = 0.0f; v[n].w = 1.0f;
        v[n].u = uk; v[n].v = v1;  v[n].color = color; ++n;
        v[n].x = xk; v[n].y = ytk; v[n].z = 0.0f; v[n].w = 1.0f;
        v[n].u = uk; v[n].v = 0.0f; v[n].color = color; ++n;

        xp = xk; hp = hk; ytp = ytk; ybp = ybk; up = uk;
    }

    if (n >= 6) m_batch->DrawTriangles(t->texture, v, n);
}

// Tinted solid rectangle: img 23 is a fully white 1280x720 pak frame.
void GameRender::DrawSolidRect(float x, float y, float w, float h, u32 color) {
    DrawFrame(IMG_BLIP_FULLWHITE, x, y, w, h, color);
}

void GameRender::StaticFrame(char out[32]) {
    Snprintf(out, 32, "img_%d", STATIC_FRAMES[m_staticIndex]);
}

void GameRender::DrawStaticOverlay(float alpha) {
    if (!m_batch) return;
    u32 a = (u32)(alpha * 255.0f);
    u32 color = (a << 24) | 0xFFFFFF;
    char name[32];
    StaticFrame(name);
    DrawTex(name, 0, 0, SCREEN_W, SCREEN_H, color);
}

void GameRender::DrawStripCentered(const SpriteStrip& strip, float cx, float y,
                                   const char* text, u32 color, float scale) {
    const f32 w = MeasureStripText(strip, text, scale);
    DrawStripText(strip, cx - w * 0.5f, y, text, color, scale);
}

// ------------------------------------------------------------
//  Disclaimer — first screen of the game.
//  "Frame 17": the visible warning is img_605 (the text pre-rendered as an
//  image by the original), instance (388,242), frame 1280x720 -> 1:1.
//  v2.6: NO static overlay — the original frame is a pure black background
//  (bg=[0,0,0,0]) with exactly 2 objects (an offscreen-parked Text and
//  img_605); the animated static only exists on the title frame.
// ------------------------------------------------------------

void GameRender::RenderDisclaimer(bool blinkOn) {
    if (!m_batch) return;
    DrawInstance(IMG_WARNING_TEXT, 388.0f, 242.0f, 0xFFFFFFFF, false);
    if (blinkOn && m_text) {
        m_text->DrawTextCenteredXY((i32)(SCREEN_W * 0.5f), (i32)(SCREEN_H * 0.82f),
                                   "PRESS  START", 0xFF6E6E6E);
    }
}

// ------------------------------------------------------------
//  Title menu — original art at original instance positions (1280x720).
// ------------------------------------------------------------

void GameRender::RenderTitle(const MenuSystem& menu, bool hasSave, i32 stars) {
    if (!m_batch) return;

    // Background with rare lit-Freddy twitch ("Active 2" anims)
    int bg = IMG_MENU_BG;
    const int tw = (int)(m_time * 3.0f) % 23;   // ~every 7-8 s, 1-2 frames
    if (tw == 7)  bg = IMG_MENU_FLICK1;
    if (tw == 15) bg = IMG_MENU_FLICK2;
    DrawFrame(bg, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);

    // Logo + static labels (hotspot-corrected instance positions)
    // v2.7.4 VERDICT CORRECTION: the source exe is the FULL Steam game
    // (ContentBuilder targetFilename, DEMO? counter never set -> stays 0),
    // NOT the demo. The 'demo' watermark obj 572 / "end of demo" frame are
    // leftovers of the shared MFA project and are event-gated off; we
    // simply never draw them (same on-screen result as the real game).
    DrawInstance(IMG_MENU_LOGO,   172.0f,  68.0f, 0xFFFFFFFF, false);
    DrawInstance(IMG_COPYRIGHT,  1044.0f, 686.0f, 0xFFFFFFFF, false);
    DrawInstance(IMG_VERSION,      26.0f, 682.0f, 0xFFFFFFFF, false);

    // Menu entries (hotspots are centered on the buttons)
    const i32 sel = menu.GetMainSelection();
    DrawInstance(IMG_NEW_GAME,     275.0f, 420.0f, 0xFFFFFFFF, false);
    if (hasSave) DrawInstance(IMG_CONTINUE, 275.0f, 492.0f, 0xFFFFFFFF, false);
    if (menu.GetUnlockedNight() >= 6) DrawInstance(IMG_SIXTH_NIGHT, 285.0f, 571.0f, 0xFFFFFFFF, false);
    if (menu.GetUnlockedNight() >= 7) DrawInstance(IMG_CUSTOM_NIGHT, 324.0f, 639.0f, 0xFFFFFFFF, false);

    // Stars for completed nights
    for (int i = 0; i < stars && i < 3; ++i) {
        static const f32 sx[3] = { 200.0f, 277.0f, 352.0f };
        DrawInstance(IMG_STAR, sx[i], 338.0f, 0xFFFFFFFF, false);
    }

    // "Night" word + selected night number (VAR25 white digit strip)
    if (hasSave) {
        DrawInstance(IMG_NIGHT_WORD, 174.0f, 512.0f, 0xFFFFFFFF, false);
        char nb[16];
        Snprintf(nb, sizeof(nb), "%d", menu.GetSelectedNight());
        DrawStripText(STRIP_VAR25, 246.0f, 515.0f, nb, 0xFFFFFFFF, 1.0f);
    }

    // ">>" cursor beside the active row
    f32 ay = 420.0f;
    if (sel == 1 && hasSave) ay = 492.0f;
    else if (sel == 1) ay = 420.0f;
    else if (sel == 2 && menu.GetUnlockedNight() >= 6) ay = 571.0f;
    else if (sel >= 3 && menu.GetUnlockedNight() >= 7) ay = 639.0f;
    const bool blink = ((int)(m_time * 2.0f)) % 2 == 0;
    if (blink) DrawInstance(IMG_MENU_ARROW, 132.0f, ay, 0xFFFFFFFF, false);

    // Rare white-noise blip (object "blip flash 2")
    if (((int)(m_time * 3.0f) % 29) == 5) {
        const int f = MENU_BLIP[((int)(m_time * 15.0f)) % 7];
        DrawFrame(f, 0, 0, SCREEN_W, SCREEN_H, 0x5AFFFFFF);
    }

    // Animated static overlay (object "static" on top in the original,
    // semi-transparency coeff 100 -> alpha 155/255, see STATIC_ALPHA above)
    DrawStaticOverlay(STATIC_ALPHA);
}

// ------------------------------------------------------------
//  Night start: the original "12:00 AM / Nth Night" card images over the
//  white-noise blip cycle ("what day" frame objects).
// ------------------------------------------------------------

void GameRender::RenderNightStart(i32 night) {
    if (!m_batch) return;
    if (night < 1) night = 1;
    if (night > 7) night = 7;

    // white-noise flash background (blip flash anim, ~12 fps)
    const int bf = BLIP_FRAMES[((int)(m_time * 12.0f)) % 9];
    DrawFrame(bf, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);

    // the card itself (hotspot-centered at instance (646,318))
    DrawInstance(NIGHT_CARDS[night], 646.0f, 318.0f, 0xFFFFFFFF, false);
}

// ------------------------------------------------------------
//  v2.7.9 shared layer-3 HUD: mute call, clock, night, power, usage.
//  In the frame data these objects (137/122/133/101/102/'AM'/usage)
//  sit on layer 3 and the monitor toggle groups 81/82 hide ONLY the
//  layer-2 overlays (static/REC/bezel/flash) -- the HUD is never
//  hidden, so it draws on BOTH the office and the monitor. This is
//  why the real camera view shows "1 AM"/"Night N"/"Power left:
//  NN%"/usage bars (reference screenshots).
// ------------------------------------------------------------
void GameRender::DrawSharedHud(const Game& game, bool phonePlaying) {
    // MUTE CALL blinks while the phone plays (instance (87,37))
    if (phonePlaying) {
        const bool on = ((int)(m_time * 2.0f)) % 2 == 0;
        if (on) DrawInstance(IMG_MUTE_CALL, 87.0f, 37.0f, 0xCDFFFFFF, false);  // v2.7.4: original semi-transparency coeff 50 -> alpha 205/255
    }

    // clock — real-game layout "H AM" on ONE row (reference screenshot):
    // hour digits right-aligned against the AM image (img_251 @ (1198,31)).
    // (The 'time of day' counter anchor (1185,59) seen in the raw frame data
    // is demo-leftover layout; the real game draws the digits beside "AM".)
    DrawInstance(IMG_AM, 1198.0f, 31.0f, 0xFFFFFFFF, false);
    {
        const char* hrs = game.GetTimer().GetHourString();
        const f32 hw = MeasureStripText(STRIP_CLOCK, hrs, 1.0f);
        DrawStripText(STRIP_CLOCK, 1198.0f - hw - 6.0f, 31.0f, hrs, 0xFFFFFFFF, 1.0f);
    }

    // "Night N" — small row under the clock (word img_447 @(1148,74) via
    // its parking hotspot, digits right after the word on the same line —
    // exactly the real-game HUD; no collision now that the hour digits
    // share the AM row above).
    DrawInstance(IMG_NIGHT_WORD_OFC, 754.0f, 74.0f, 0xFFFFFFFF, false);
    {
        char nb[16];
        Snprintf(nb, sizeof(nb), "%d", game.GetCurrentNight());
        DrawStripText(STRIP_VAR14, 1148.0f + 63.0f + 6.0f, 74.0f, nb, 0xFFFFFFFF, 1.0f);
    }

    // power: "Power left:" img_207 @(106,638) + VAR14 digits + img_208 "%"
    DrawInstance(IMG_POWER_LABEL, 106.0f, 638.0f, 0xFFFFFFFF, false);
    {
        const PowerSystem& pw = game.GetPower();
        char num[16];
        // v2.7.12: original truncates (999 tenths -> "99" at night start)
        Snprintf(num, sizeof(num), "%d", (i32)(pw.GetPowerTenths() / 10));
        DrawStripText(STRIP_VAR14, 182.0f, 632.0f, num, 0xFFFFFFFF, 1.0f);
        const f32 dw = MeasureStripText(STRIP_VAR14, num, 1.0f);
        // img_208 "%" drawn directly: its data hotspot (-420,0) is a
        // parking artifact, the glyph itself is 11x14. Real-game layout:
        // "Power left: NN%" all on one line right after the label.
        DrawFrame(IMG_PERCENT, 182.0f + dw + 2.0f, 632.0f, 11.0f, 14.0f, 0xFFFFFFFF);

        // usage: "Usage:" img_189 @(74,674) + 5 tinted bar cells
        // ('usage meter' counter sits at (120,657) in the frame data)
        DrawInstance(IMG_USAGE_LABEL, 74.0f, 674.0f, 0xFFFFFFFF, false);
        const int lvl = pw.GetUsageLevel();
        for (int i = 0; i < 5; ++i) {
            u32 c;
            if (i >= lvl)      c = D3DCOLOR_XRGB(38, 38, 42);   // empty slot
            else if (lvl <= 2) c = D3DCOLOR_XRGB(70, 225, 70);  // green
            else if (lvl == 3) c = D3DCOLOR_XRGB(235, 210, 40); // yellow
            else               c = D3DCOLOR_XRGB(225, 50, 40);  // red
            DrawSolidRect(120.0f + i * 13.0f, 664.0f, 11.0f, 14.0f, c);
        }
    }}

// ------------------------------------------------------------
//  Office — TRUE original composition (frame 'Frame 1', 1600x720):
//  layer 0 = scene (bg 39, doors, panels, fan, pumpkin,GoldenFreddy),
//  layer 2 = monitor-gated overlays (static obj 42, REC obj 43, bezel
//            obj 50) -- event group 81 HARD-HIDES all of them while
//            viewing==0, i.e. they NEVER draw in the office (v2.7.5),
//  layer 3 = HUD (mute call, clock, night, power/usage, flip bar).
//  A 1280x720 window pans across the 1600x720 scene (0..320).
// ------------------------------------------------------------

void GameRender::RenderOffice(const Game& game, bool phonePlaying) {
    if (!m_batch) return;
    const DoorSystem& doors = game.GetDoors();

    // ---------- scene layer (pans + v2.7.6 Perspective bend) ----------
    // The original's Perspective.mfx grabs the ALREADY DRAWN flat layer 0
    // and blits the re-projected copy over it; outside the bent band the
    // flat scene stays visible (top/bottom wedges near the screen edges).
    // So we draw layer 0 TWICE: flat pass, then the bent re-projection.
    // Everything below sits on layer 0 and bends; the HUD below stays flat.
    const f32 pan = m_panX;

    // desk fan (3 blade frames, ~8 fps). v2.7.1: the desk pumpkin
    // (img_628..635) is NOT drawn — in the frame data it is gated by the
    // 'Date & Time'/'month'/'day' objects (Halloween easter egg), and the
    // real full game has no pumpkin on the desk on regular days (confirmed
    // against a real-game reference screenshot). Keep the frames here for a
    // future date check.
    const int fan = (m_time < 9999.0f) ? (int)(m_time * 8.0f) % 3 : 0;
    static const int FAN[3] = { IMG_FAN_0, IMG_FAN_1, IMG_FAN_2 };

    // doors: open doorway vs closed slab
    const bool lc = doors.IsDoorClosed(DOOR_LEFT);
    const bool rc = doors.IsDoorClosed(DOOR_RIGHT);

    // Button panels — state table from the event script + pixel-verified:
    //   LEFT  @ (48,390):  closed=122  open=124  closed+light=125  open+light=130
    //   RIGHT @ (1546,400): closed=134  open=135  closed+light=131  open+light=47
    const bool ll = doors.IsLightOn(DOOR_LEFT);
    const bool rl = doors.IsLightOn(DOOR_RIGHT);
    const int lp = lc ? (ll ? IMG_PANEL_L_CL_LT : IMG_PANEL_L_CLOSED)
                      : (ll ? IMG_PANEL_L_OP_LT : IMG_PANEL_L_OPEN);
    const int rp = rc ? (rl ? IMG_PANEL_R_CL_LT : IMG_PANEL_R_CLOSED)
                      : (rl ? IMG_PANEL_R_OP_LT : IMG_PANEL_R_OPEN);

    // ---- v2.7.8 office FX state machine (docs/OFFICE_FX.md) ----
    const f32 dt = m_time - m_lastT;
    m_lastT = m_time;
    const bool monUp = game.GetCameras().IsMonitorUp();
    if (m_prevMonitor && !monUp) {                 // tablet just dropped
        m_wipeT  = 0.0f;                           // dark uncover wipe (group 322)
        m_raiseT = -1.0f;                          // v2.7.9: cancel a half-finished raise
        m_flashT = -1.0f;
        m_prevCam = -1;                            // next arrival flashes (group 133)
    }
    m_prevMonitor = monUp;
    if (lc != m_prevDoorL) { m_doorT[0] = 0.0f; m_doorClosing[0] = lc; m_prevDoorL = lc; }
    if (rc != m_prevDoorR) { m_doorT[1] = 0.0f; m_doorClosing[1] = rc; m_prevDoorR = rc; }
    if (m_doorT[0] >= 0.0f && (m_doorT[0] += dt) >= 16.0f * DOOR_FRAME_T) m_doorT[0] = -1.0f;
    if (m_doorT[1] >= 0.0f && (m_doorT[1] += dt) >= 16.0f * DOOR_FRAME_T) m_doorT[1] = -1.0f;
    if (m_wipeT  >= 0.0f && (m_wipeT  += dt) >= 11.0f * WIPE_FRAME_T)  m_wipeT  = -1.0f;
    if (m_flashT >= 0.0f && (m_flashT += dt) >= 9.0f * FLASH_FRAME_T) m_flashT = -1.0f;

    // panorama variant: obj 44 "Active 3" anim table via event groups
    // 114-129 (light buttons + strobe) and group 323 (foxy sprint).
    int bg = IMG_OFFICE_BG;
    {
        const AnimatronicAI& ai = game.GetAI();
        const Animatronic& foxy = ai.GetAnimatronic(ANIM_FOXY);
        if (foxy.foxyRunning && !foxy.foxyAtDoor) {
            int idx = foxy.foxyRunTimer * 33 / 100;  // 60 Hz -> 33 frames / 1.67 s
            if (idx > 32) idx = 32;
            bg = FOXY_RUN[idx];
        } else {
            const bool ll = doors.IsLightOn(DOOR_LEFT);
            const bool rl = doors.IsLightOn(DOOR_RIGHT);
            if (ll || rl) {
                // group 122 re-rolls the flicker counter every frame;
                // groups 119/127: <= 1 (of 0..9) -> dark frame 0 (img_39),
                // the ~20% off-duty of the fluorescent strobe.
                const bool dark = (rand() % 10) <= 1;
                if (ll) {
                    const Animatronic& b = ai.GetAnimatronic(ANIM_BONNIE);
                    const bool atDoor = (b.currentRoom == ROOM_WEST_HALL_CORNER ||
                                         b.currentRoom == ROOM_LEFT_DOOR);
                    bg = dark ? IMG_OFFICE_BG
                              : (atDoor ? IMG_LIGHT_L_BONNIE : IMG_LIGHT_L_HALL);
                }
                if (rl) {
                    const Animatronic& c = ai.GetAnimatronic(ANIM_CHICA);
                    const bool atDoor = (c.currentRoom == ROOM_EAST_HALL_CORNER ||
                                         c.currentRoom == ROOM_RIGHT_DOOR);
                    bg = dark ? IMG_OFFICE_BG
                              : (atDoor ? IMG_LIGHT_R_CHICA : IMG_LIGHT_R_HALL);
                }
            }
        }
    }

    // door slide anims: obj 59/60 anims a12 (close) / a14 (open)
    int lImg = lc ? IMG_DOOR_L_CLOSED : IMG_DOOR_L_OPEN;
    int rImg = rc ? IMG_DOOR_R_CLOSED : IMG_DOOR_R_OPEN;
    if (m_doorT[0] >= 0.0f) {
        int idx = (int)(m_doorT[0] / DOOR_FRAME_T);
        if (idx > 15) idx = 15;
        lImg = m_doorClosing[0] ? DOOR_L_CLOSE[idx] : DOOR_L_OPEN[idx];
    }
    if (m_doorT[1] >= 0.0f) {
        int idx = (int)(m_doorT[1] / DOOR_FRAME_T);
        if (idx > 15) idx = 15;
        rImg = m_doorClosing[1] ? DOOR_R_CLOSE[idx] : DOOR_R_OPEN[idx];
    }

    // flat layer 0 (what the Perspective object grabs underneath)
    DrawInstance(bg, 0.0f,   0.0f, 0xFFFFFFFF, true);
    DrawInstance(FAN[fan], 868.0f, 400.0f, 0xFFFFFFFF, true);
    DrawInstance(lImg, 72.0f,  -1.0f, 0xFFFFFFFF, true);
    DrawInstance(rImg, 1270.0f, -2.0f, 0xFFFFFFFF, true);
    DrawInstance(lp,  48.0f, 390.0f, 0xFFFFFFFF, true);
    DrawInstance(rp, 1546.0f, 400.0f, 0xFFFFFFFF, true);

    // bent re-projection on top (v2.7.6: same order, one shared curve)
    DrawBentInstance(bg, 0.0f,   0.0f, 0xFFFFFFFF, pan);
    DrawBentInstance(FAN[fan], 868.0f, 400.0f, 0xFFFFFFFF, pan);
    DrawBentInstance(lImg, 72.0f,  -1.0f, 0xFFFFFFFF, pan);
    DrawBentInstance(rImg, 1270.0f, -2.0f, 0xFFFFFFFF, pan);
    DrawBentInstance(lp,  48.0f, 390.0f, 0xFFFFFFFF, pan);
    DrawBentInstance(rp, 1546.0f, 400.0f, 0xFFFFFFFF, pan);

    // ---- v2.7.5 verdict kept: NO animated static in the office. Obj 42
    // (static, ink 1/100) starts invisible and group 81 hides it whenever
    // viewing==0; the office grain is baked into img_39's pixels. The
    // layer-2 overlays (static, REC, bezel) draw ONLY in RenderCamera.
    // Full map: docs/OVERLAY_MAP.md. ----

    // ---------- screen-fixed HUD (v2.7.9: shared with the
    // monitor, see DrawSharedHud below) ----------
    DrawSharedHud(game, phonePlaying);


    // "open monitor" flip bar — frame obj 'flip panel': img_420 600x60 at
    // instance (554,668), hotspot (299,30) -> draw (255,638).
    // v2.6 and earlier wrongly drew img_156 here (a solid violet fill) —
    // that was the big purple rectangle over the desk.
    // v2.7.9: the bar HIDES while the tablet moves (group 331 hides
    // it the moment the raise starts; group 322 keeps it hidden
    // during the drop wipe until the player can click again).
    if (m_raiseT < 0.0f && m_wipeT < 0.0f)
        DrawInstance(IMG_FLIP_BAR, 554.0f, 668.0f, 0xFFFFFFFF, false);

    // v2.7.8: tablet-close dark wipe (obj 73 "flip down 2", event group
    // 322 creates it on put down==1) -- starts black, dissolves clear.
    if (m_wipeT >= 0.0f) {
        int idx = (int)(m_wipeT / WIPE_FRAME_T);
        if (idx > 10) idx = 10;
        DrawFrame(WIPE_SEQ[idx], 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
    }
}

// ------------------------------------------------------------
//  Camera monitor — screen space: feed drifting inside the 1600-wide
//  room image (sine ping-pong over the 320 px slack), bezel, label
//  image, map with blinking selected-cam button.
// ------------------------------------------------------------
static const f32 CAM_PAN_RANGE  = 320.0f;   // 1600 - 1280
static const f32 CAM_PAN_PERIOD = 18.0f;    // seconds per full sweep

static int CamFeedFor(const Game& game, CameraId cam) {
    // presence-aware feed selection (defaults verified from the pak)
    const AnimatronicAI& ai = game.GetAI();
    const Animatronic& bonnie = ai.GetAnimatronic(ANIM_BONNIE);
    const Animatronic& chica  = ai.GetAnimatronic(ANIM_CHICA);
    const Animatronic& freddy = ai.GetAnimatronic(ANIM_FREDDY);
    const bool b = (bonnie.currentRoom == (RoomId)cam);
    const bool c = (chica.currentRoom  == (RoomId)cam);
    const bool f = (freddy.currentRoom == (RoomId)cam);
    (void)f;

    switch (cam) {
        case CAM_1A: {
            // stage combos: full 19, no-Bonnie 68, no-Freddy 223,
            // Freddy-only 224, empty 484
            const bool bo = (bonnie.currentRoom == ROOM_SHOW_STAGE);
            const bool ch = (chica.currentRoom  == ROOM_SHOW_STAGE);
            const bool fr = (freddy.currentRoom == ROOM_SHOW_STAGE);
            if (!bo && !ch && !fr) return CAMFEED_1A_EMPTY;
            if (!bo &&  ch &&  fr) return CAMFEED_1A_NOBC;
            if ( bo &&  ch && !fr) return CAMFEED_1A_NOFC;
            if ( fr && !bo && !ch) return CAMFEED_1A_FREDDY;
            return CAMFEED_1A_FULL;
        }
        case CAM_1B:
            if (b) return CAMFEED_1B_BONNIE;
            if (c) return CAMFEED_1B_CHICA;
            return CAMFEED_1B_EMPTY;
        case CAM_2A:
            if (b) return CAMFEED_2A_BONNIE;
            if (c) return CAMFEED_2A_FIGURE;
            return CAMFEED_2A_EMPTY;
        case CAM_2B:
            if (b) return CAMFEED_2B_BONNIE;
            if (c) return CAMFEED_2B_CHICA;
            return CAMFEED_2B_EMPTY;
        case CAM_3:
            if (b) return CAMFEED_3_BONNIE;
            return CAMFEED_3_EMPTY;
        case CAM_4A:
            if (b) return CAMFEED_4A_FIGURE;
            return CAMFEED_4A_EMPTY;
        case CAM_4B:
            if (c) return CAMFEED_4B_CHICA;
            return CAMFEED_4B_EMPTY;
        case CAM_5:
            if (b) return CAMFEED_5_BONNIE;
            return CAMFEED_5_EMPTY;
        case CAM_7:
            if (c) return CAMFEED_7_VARIANT;
            return CAMFEED_7_EMPTY;
        default:
            return CAMFEED_NONE;   // v2.7.10: kitchen audio-only; -1 because
                                   // handle 0 now belongs to CAM 2B (img_0)
    }
}

void GameRender::RenderCamera(const Game& game, bool phonePlaying) {
    if (!m_batch) return;
    const CameraSystem& cams = game.GetCameras();
    const CameraId cam = cams.GetCurrentCamera();
    // v2.7.9: the tablet RAISE (obj 68 "panel", event group 130) plays
    // BEFORE the feed appears: 11 full-screen frames cover the office
    // bottom-up; when the anim ends, group 133 commits viewing (->
    // group 18 -> blip==1 -> group 16 white flash) and the monitor
    // takes over. No instant pop-in like v2.7.8 and earlier.
    const f32 dt = m_time - m_lastT;
    if (!m_prevMonitor) m_raiseT = 0.0f;   // monitor just went up
    m_prevMonitor = true;
    if (m_raiseT >= 0.0f) {
        m_raiseT += dt;
        if (m_raiseT >= 11.0f * RAISE_FRAME_T) {
            m_raiseT = -1.0f;
            m_lastT = m_time;
        } else {
            // office stays visible under the rising tablet; RenderOffice
            // also keeps m_lastT and the office FX timers ticking
            RenderOffice(game, phonePlaying);
            int ridx = (int)(m_raiseT / RAISE_FRAME_T);
            if (ridx > 10) ridx = 10;
            DrawFrame(RAISE_SEQ[ridx], 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
            return;
        }
    } else {
        m_lastT = m_time;
    }
    // v2.7.9: EVERY committed cam change flashes. Group 18: 'set viewing
    // to' > 0 -> blip := 1 -> group 16 white flash + blip3. The raise
    // commit (group 133: viewing := last clicked) rides the same counter,
    // so the freshly appeared feed ALWAYS flashes once -- and map clicks
    // flash on every switch (the reference screenshot is exactly that).
    if ((int)cam != m_prevCam) m_flashT = 0.0f;
    m_prevCam = (int)cam;
    if (m_flashT >= 0.0f && (m_flashT += dt) >= 9.0f * FLASH_FRAME_T) m_flashT = -1.0f;

    // Room feed: 1600x720 room image centered behind the 1280x720 bezel.
    // Pirate cove follows Foxy's stage machine; kitchen has no feed.
    int feed = CAMFEED_NONE;   // v2.7.10: -1 = "no feed", 0 = img_0 (CAM 2B)
    if (cam == CAM_1C) {
        const Animatronic& foxy = game.GetAI().GetAnimatronic(ANIM_FOXY);
        int st = (int)foxy.foxyStage;
        if (st < 0) st = 0;
        if (st > 3) st = 3;   // v2.7.10: four canonical cove states
        // v2.7.10: was {211,338,240} = one stage late (closed curtain
        // img_66 never shown!). Canonical: stage0=66 shut, 1=211 peek,
        // 2=338 out, 3=240 gone/running (Active 3 anims 26/48/49/50).
        static const int COVE[4] = { COVE_CLOSED, COVE_PEEK, COVE_OUT, COVE_EMPTY };
        feed = COVE[st];
    } else if (cam >= CAM_1A && cam <= CAM_7) {
        feed = CamFeedFor(game, cam);
    }
    // Authentic drift: in the original the office view pans with the
    // stick while camera feeds slowly wander left<->right on their own
    // (the 1600x720 renders carry 320 px of slack for exactly that).
    // Cosine easing = smooth turnaround at both edges, no dead pause.
    const f32 camPan = 0.5f * CAM_PAN_RANGE
                     * (1.0f - cosf(m_time * 6.2831853f / CAM_PAN_PERIOD));
    if (feed >= 0) {   // v2.7.10: >=0 -- img_0 (CAM 2B) must draw too
        // v2.7.6: the feed is on layer 0 too -- the Perspective object bends
        // it with the SAME curve as the office (this is why FNAF camera
        // feeds bulge). Flat pass first, then the re-projection; drift pan
        // = the 320 px slack of the 1600x720 image.
        DrawFrame(feed, -camPan, 0.0f, 1600.0f, 720.0f, 0xFFFFFFFF);
        DrawBentInstance(feed, 0.0f, 0.0f, 0xFFFFFFFF, camPan);
    } else if (cam == CAM_6) {
        DrawInstance(IMG_AUDIO_ONLY, 384.0f, 69.0f, 0xFFFFFFFF, false);
    }

    // v2.7.5: the one and only gameplay home of the animated static --
    // obj 42 "Active" (img 18/20/12..17, ink 1/100 -> alpha 155/255),
    // which group 82 SHOWs exactly while viewing>0 (monitor up).
    DrawStaticOverlay(STATIC_ALPHA);

    // v2.7.5: the monitor's blinking red REC light -- obj "Active 2",
    // anim [img_7 red 50x50, img_5 fully transparent] at speed 2
    // (~0.8 s per phase), instance (92,76) hotspot (24,24) -> draw
    // (68,52). Layer 2 over the static, under bezel/labels/HUD --
    // the original instance order is 42 < 43 < 50.
    if (((int)(m_time * 1.25f)) % 2 == 0)
        DrawFrame(7, 68.0f, 52.0f, 50.0f, 50.0f, 0xFFFFFFFF);

    // v2.7.8: white flash, layer 2 -- above static/REC (obj 42/43),
    // below the bezel (obj 50); one-shot 9 frames, then gone.
    if (m_flashT >= 0.0f) {
        int idx = (int)(m_flashT / FLASH_FRAME_T);
        if (idx > 8) idx = 8;
        DrawFrame(FLASH_SEQ[idx], 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
    }

    // Monitor bezel (object "frame" img_11)
    DrawInstance(IMG_MONITOR_FRAME, 0.0f, -1.0f, 0xFFFFFFFF, false);

    // Location label image ("location" object, instance (832,292))
    if (cam >= CAM_1A && cam <= CAM_7) {
        DrawInstance(CAM_LABELS[(int)cam], 832.0f, 292.0f, 0xFFFFFFFF, false);
    }

    // Cam map (img_164 @ (848,313)) + blinking selected-cam button
    DrawFrame(IMG_CAM_MAP, 848.0f, 313.0f, 400.0f, 400.0f, 0xFFFFFFFF);
    if (cam >= CAM_1A && cam <= CAM_7) {
        const PakMapBtn mb = PakMapButtonOf((int)cam);
        const bool on = ((int)(m_time * 2.0f)) % 2 == 0;
        DrawInstance(on ? IMG_CAM_BTN_ON : IMG_CAM_BTN_OFF,
                     (f32)mb.x, (f32)mb.y, 0xFFFFFFFF, false);
    }

    // v2.7.9: shared layer-3 HUD (clock/night/power/usage/mute call).
    // Same objects and positions as the office view -- the frame data
    // never hides them with the monitor (groups 81/82 touch layer 2).
    DrawSharedHud(game, phonePlaying);

    // "put down" bar — the real bar is img_420 ('flip panel', same as the
    // office bump). The frame's 'flip down' object (img_162) is a SOLID
    // VIOLET fill (a Clickteam zone marker) — drawing it opaque painted a
    // purple slab over the camera view in v2.6 and earlier.
    DrawInstance(IMG_FLIP_BAR, 554.0f, 668.0f, 0xFFFFFFFF, false);
}

// ------------------------------------------------------------
//  Power out: the real 4-phase sequence (groups 272-302):
//   phase 0 dark office / phase 1 music box + face flicker
//   (anim 47 lit @ 25%/0.5s else anim 46 dark) / phase 2 buzz
//   blink / phase 3 total black.
// ------------------------------------------------------------

void GameRender::RenderPowerOut(const Game& game) {
    if (!m_batch) return;
    // v2.7.6: power-out office + flicker frames are layer 0 -> drawn flat,
    // then re-projected through the Perspective curve
    const i32 poPhase = game.GetPowerOutPhase();

    int flicker = POWEROUT_DARK;                 // anim 46 (dark office)
    if (poPhase == 0) {
        flicker = POWEROUT_DARK;
    } else if (poPhase == 1) {
        // face flicker: Game re-rolls 25% lit every 0.5 s (group 289)
        flicker = game.IsFreddyFaceLit() ? POWEROUT_LIT : POWEROUT_DARK;
    } else if (poPhase == 2) {
        // buzz blink: office toggles on/off (alterable[7] Random(2)+1)
        flicker = POWEROUT_DARK;
    } else {
        flicker = POWEROUT_DARK; // phase 3 handled below (no draw)
    }

    if (poPhase < 3 && (poPhase != 2 || game.IsPowerOutBlinkOn())) {
        DrawInstance(POWEROUT_OFFICE, 0.0f, 0.0f, 0xFFFFFFFF, true);
        DrawInstance(flicker, 0.0f, 0.0f, 0xFFFFFFFF, true);
        DrawBentInstance(POWEROUT_OFFICE, 0.0f, 0.0f, 0xFFFFFFFF, m_panX);
        DrawBentInstance(flicker, 0.0f, 0.0f, 0xFFFFFFFF, m_panX);
    }

    // the HUD stays, power reads 0
    DrawInstance(IMG_POWER_LABEL, 106.0f, 638.0f, 0xFF9A9A9A, false);
    DrawStripText(STRIP_VAR14, 221.0f, 644.0f, "0", 0xFF9A9A9A, 1.0f);
    DrawFrame(IMG_PERCENT, 221.0f + 9.0f + 2.0f, 644.0f, 11.0f, 14.0f, 0xFF9A9A9A);
}

// ------------------------------------------------------------
//  Jump scares — real kill animations from Active 3 (docs/AI_MECHANICS.md §9):
//   Freddy anim 65 @30FPS, Foxy anim 52 @30FPS,
//   Bonnie anim 35 @45FPS, Chica anim 44 @60FPS.
//  The scare holds its last frame for the remainder of the state.
// ------------------------------------------------------------

void GameRender::RenderJumpscare(AnimatronicId anim, f32 elapsed) {
    if (!m_batch) return;

    // Shake like the original's alternating scare frames
    const int sh = ((int)(elapsed * 30.0f)) % 2;
    const f32 ox = (sh ? 10.0f : -10.0f);
    const f32 oy = (sh ? 6.0f : -6.0f);

    int frame = 0;
    if (anim == ANIM_FREDDY) {
        // anim 65: 31 frames @ 30 FPS (speed 50), repeat 1
        const int i = (int)(elapsed * 30.0f) % 31;
        frame = SCARE_FREDDY[i];
    } else if (anim == ANIM_FOXY) {
        // anim 52: 25 frames @ 30 FPS (speed 50), repeat 1
        const int i = (int)(elapsed * 30.0f) % 25;
        frame = SCARE_FOXY[i];
    } else if (anim == ANIM_BONNIE) {
        // anim 35: 11 frames @ 45 FPS (speed 75), play once, hold last
        int i = (int)(elapsed * 45.0f);
        if (i > 10) i = 10;
        frame = SCARE_BONNIE_KILL[i];
    } else {
        // anim 44: 16 frames @ 60 FPS (speed 99), play once, hold last
        int i = (int)(elapsed * 60.0f);
        if (i > 15) i = 15;
        frame = SCARE_CHICA_KILL[i];
    }
    // scare frames are 1600x720 room images on layer 0 -- v2.7.6: flat pass
    // + bent re-projection; the shake offsets shift the source window like
    // the original's Set position on the feed object (+ox on screen = -ox
    // on the pan).
    DrawFrame(frame, -160.0f + ox, oy, 1600.0f, 720.0f, 0xFFFFFFFF);
    DrawBentInstance(frame, 0.0f, oy, 0xFFFFFFFF, 160.0f - ox);

    // IT'S ME hallucination flash (obj "Active 21")
    const int fl = (int)(elapsed * 10.0f) % 4;
    if (fl == 1 || fl == 3) {
        DrawFrame(ITSME_FRAMES[fl], 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    }
}

// ------------------------------------------------------------
//  6 AM / Game over
//  "next day" frame: "5"=350 @(544,298), "AM"=352 @(640,296),
//  "6"=351 @(548,408). Nights 5/6/7 end with the full-screen
//  paycheck / overtime / termination images ("the end" frames).
// ------------------------------------------------------------

void GameRender::RenderNightComplete(i32 night) {
    if (!m_batch) return;
    if (night >= 7) {
        DrawFrame(IMG_END_NIGHT7, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    } else if (night == 6) {
        DrawFrame(IMG_END_NIGHT6, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    } else if (night == 5) {
        DrawFrame(IMG_END_NIGHT5, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    } else {
        // "6 AM" from the real digit images (lower row of the flip)
        DrawInstance(IMG_DIGIT_6, 548.0f, 408.0f, 0xFFFFFFFF, false);
        DrawInstance(IMG_AM_BIG, 640.0f, 406.0f, 0xFFFFFFFF, false);
    }
    // v2.7.5: no static. The original "next day" frame holds ONLY the
    // 5/AM/6 digit images + AI counters -- zero noise objects (dump verdict).
}

void GameRender::RenderGameOver() {
    if (!m_batch) return;
    DrawFrame(IMG_GAMEOVER_BG, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    // v2.7.5: no static. The original "gameover" frame = backdrop img_358 +
    // img_471 + parked Text/counter -- no noise objects at all. (The LOUD
    // noise burst is the separate "died" frame -- static + opaque blip
    // flash over black -- shown between the jumpscare and game over; we do
    // not render that screen yet, see docs/OVERLAY_MAP.md.)
}

// ------------------------------------------------------------
//  Sprite browser (debug): page 0 shows every counter-font strip
//  drawn as its full alphabet; pages 1.. show label candidates and
//  all remaining small sprites in grids.
//  Pages: 0 strips | 1-6 labels (9 per page) | 7-13 small (12 per page)
// ------------------------------------------------------------

i32 GameRender::SpriteBrowserPageCount() const {
    const int labelPages = (52 + 8) / 9;              // 3x3 grid
    const int smallPages = (82 + 11) / 12;            // 3x4 grid
    return 1 + labelPages + smallPages;
}

void GameRender::RenderSpriteBrowser(i32 page) {
    if (!m_batch || !m_text) return;
    const i32 total = SpriteBrowserPageCount();
    if (page < 0) page = 0;
    if (page >= total) page = total - 1;
    char hdr[96];

    if (page == 0) {
        // ---- page 0: the counter strips, one row each -------------------
        Snprintf(hdr, sizeof(hdr), "SPRITE BROWSER 0/%d  STRIPS (alphabet 0-9 - + . e)",
                 total - 1);
        m_text->DrawText(16, 10, hdr, 0xFFFFFF00);
        float y = 34;
        for (int s = 0; s < STRIP_COUNT; ++s) {
            const SpriteStrip& st = STRIPS[s];
            int maxH = 0, maxW = 0;
            for (int i = 0; i < st.count; ++i) {
                if (st.glyphs[i].h > maxH) maxH = st.glyphs[i].h;
                if (st.glyphs[i].w > maxW) maxW = st.glyphs[i].w;
            }
            char cap[80];
            Snprintf(cap, sizeof(cap), "%s  (%d)", st.tag, (int)st.count);
            m_text->DrawText(16, (i32)y + (maxH - 14) / 2, cap, 0xFF88FF88);
            float pen = 300.0f;
            for (int i = 0; i < st.count; ++i) {
                const StripGlyph& g = st.glyphs[i];
                const float gy = y + (maxH - g.h);
                DrawFrame(g.handle, pen, gy, (f32)g.w, (f32)g.h, 0xFFFFFFFF);
                pen += g.w + 2.0f;
            }
            Snprintf(cap, sizeof(cap), "w<=%d", maxW);
            m_text->DrawText(1150, (i32)y + (maxH - 14) / 2, cap, 0xFF6688FF);
            y += maxH + 12.0f;
        }
        m_text->DrawText(16, 700, "DPAD left/right = page   A/B = exit", 0xFF808080);
        return;
    }

    const int labelPages = (52 + 8) / 9;
    if (page <= labelPages) {
        // ---- label candidate pages: 3x3 grid ----------------------------
        Snprintf(hdr, sizeof(hdr), "SPRITE BROWSER %d/%d  LABEL CANDIDATES",
                 page, total - 1);
        m_text->DrawText(16, 10, hdr, 0xFFFFFF00);
        const f32 cellW = 410.0f, cellH = 215.0f;
        for (int i = 0; i < 9; ++i) {
            const int idx = (page - 1) * 9 + i;
            if (idx >= 52) break;
            const int handle = LABEL_CANDIDATES[idx];
            const i32 col = i % 3, row = i / 3;
            const f32 cx = 212.0f + col * cellW;
            const f32 cy = 122.0f + row * cellH;
            DrawFrameFit(handle, cx, cy, cellW - 24.0f, cellH - 40.0f, 0xFFFFFFFF);
            char cap[40];
            Snprintf(cap, sizeof(cap), "img_%d", handle);
            m_text->DrawText((i32)(cx - 30), (i32)(cy + cellH / 2 - 26), cap, 0xFF88FF88);
        }
    } else {
        // ---- remaining small sprites: 3x4 grid --------------------------
        const int smallPage = page - 1 - labelPages;
        Snprintf(hdr, sizeof(hdr), "SPRITE BROWSER %d/%d  SMALL SPRITES",
                 page, total - 1);
        m_text->DrawText(16, 10, hdr, 0xFFFFFF00);
        const f32 cellW = 410.0f, cellH = 163.0f;
        for (int i = 0; i < 12; ++i) {
            const int idx = smallPage * 12 + i;
            if (idx >= 82) break;
            const int handle = SMALL_REST[idx];
            const i32 col = i % 3, row = i / 3;
            const f32 cx = 212.0f + col * cellW;
            const f32 cy = 92.0f + row * cellH;
            DrawFrameFit(handle, cx, cy, cellW - 24.0f, cellH - 36.0f, 0xFFFFFFFF);
            char cap[40];
            Snprintf(cap, sizeof(cap), "img_%d", handle);
            m_text->DrawText((i32)(cx - 30), (i32)(cy + cellH / 2 - 24), cap, 0xFF88FF88);
        }
    }
    m_text->DrawText(16, 700, "DPAD left/right = page   A/B = exit", 0xFF808080);
}

// ------------------------------------------------------------
//  v2.7.11 PERSPECTIVE TUNER -- live bulge/concavity knobs
//  Knob 0 ZOOM:     edges are (754 - ZOOM) px tall, center always 754.
//                   + = fish-eye bulge, - = pincushion (concave), 0 = flat.
//  Knob 1 CENTER_Y: vertical pivot the columns expand from (default 355 =
//                   object center). Shifts where the bend pinches.
//  Knob 2 ARC:      sine span across the 1324-px object; 3.1415 = original
//                   (edges reach sin=0). Bigger = harder edge squeeze.
//  All steps are per DPad TAP (edges only in UpdateInput); A held = fast.
// ------------------------------------------------------------

static const f32 PERSP_TUNE_MIN[3]  = { -500.0f, 100.0f, 1.00f  };
static const f32 PERSP_TUNE_MAX[3]  = {  500.0f, 640.0f, 6.00f  };
static const f32 PERSP_TUNE_STEP[3] = {   10.0f,   2.0f, 0.05f  };
static const f32 PERSP_TUNE_FAST[3] = {   50.0f,  10.0f, 0.25f  };

static f32 PerspTunerGet(i32 knob) {
    if (knob == 0) return g_perspZoom;
    if (knob == 1) return g_perspCenterY;
    return g_perspArc;
}

static void PerspTunerSet(i32 knob, f32 v) {
    if (knob == 0) g_perspZoom = v;
    else if (knob == 1) g_perspCenterY = v;
    else g_perspArc = v;
}

void GameRender::PerspTunerAdjust(i32 knob, i32 dir, bool fast) {
    if (knob < 0 || knob >= PERSP_TUNER_KNOBS || dir == 0) return;
    f32 v = PerspTunerGet(knob)
          + (fast ? PERSP_TUNE_FAST[knob] : PERSP_TUNE_STEP[knob])
            * (f32)((dir > 0) ? 1 : -1);
    if (v < PERSP_TUNE_MIN[knob]) v = PERSP_TUNE_MIN[knob];
    if (v > PERSP_TUNE_MAX[knob]) v = PERSP_TUNE_MAX[knob];
    PerspTunerSet(knob, v);
}

void GameRender::PerspTunerReset(i32 knob) {
    if (knob < 0) {
        g_perspZoom    = PERSP_ZOOM;
        g_perspCenterY = PERSP_CENTER_Y;
        g_perspArc     = PERSP_PI;
        return;
    }
    PerspTunerSet(knob, PerspTunerDefault(knob));
}

f32 GameRender::PerspTunerValue(i32 knob) const {
    if (knob == 0) return g_perspZoom;
    if (knob == 1) return g_perspCenterY;
    return g_perspArc;
}

f32 GameRender::PerspTunerDefault(i32 knob) const {
    if (knob == 0) return PERSP_ZOOM;
    if (knob == 1) return PERSP_CENTER_Y;
    return PERSP_PI;
}

void GameRender::RenderPerspTuner(i32 sel) {
    if (!m_text) return;
    static const char* NAMES[3] = { "ZOOM", "CENTER_Y", "ARC" };
    static const char* HINTS[3] = {
        "+bulge  -pincushion  0 flat",
        "vertical pivot of the bend",
        "3.1415=orig  > harder edges"
    };
    m_text->DrawText(16, 10, "== PERSP TUNER (L3+R3 = exit) ==", 0xFFFFFF00);
    for (int k = 0; k < PERSP_TUNER_KNOBS; ++k) {
        char row[96];
        if (k == 2) {
            Snprintf(row, sizeof(row), "%s %s = %.4f  def %.4f  %s",
                     (k == sel) ? "> " : "  ", NAMES[k],
                     PerspTunerValue(k), PerspTunerDefault(k), HINTS[k]);
        } else {
            Snprintf(row, sizeof(row), "%s %s = %.1f  def %.1f  %s",
                     (k == sel) ? "> " : "  ", NAMES[k],
                     PerspTunerValue(k), PerspTunerDefault(k), HINTS[k]);
        }
        m_text->DrawText(16, 40 + k * 26, row,
                         (k == sel) ? 0xFFFFFFFF : 0xFF9A9A9A);
    }
    m_text->DrawText(16, 126,
        "DPad L/R = knob  Up/Dn = adjust  A = fast  Y = reset",
        0xFF808080);
}

} // namespace fnaf
