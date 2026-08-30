/**
 * Five Nights at Freddy's 1 — Recompilation
 * PakAssets.h: every image handle used by the renderer, extracted from the
 * original game data (CTFAK JSON export of the exe: banks.json with
 * width/height/hotspot per handle + application.json with per-frame object
 * instances and the event scripts).
 *
 * Every entry below was verified against the actual decoded pak image
 * (605/605 images dumped to PNG and inspected) and, where placement
 * matters, against the frame event tables:
 *  - Title frame "title"    : menu objects with exact instance positions
 *  - Disclaimer "Frame 17"  : img 605 IS the warning text image
 *  - "what day" frame       : night cards 12:00 AM / Nth Night
 *  - Office "Frame 1"       : doors/panels/HUD + event table
 *    (viewing->anim -> cam feeds and location labels, panel state table)
 *  - "next day"/"the end"   : 6 AM digits and paycheck screens
 *
 * Clickteam placement: an instance position is the position of the image's
 * HOTSPOT, so the top-left draw position = instance - hotspot.
 */

#ifndef FNAF_PAK_ASSETS_H
#define FNAF_PAK_ASSETS_H

#include "Types.h"

namespace fnaf {

// ------------------------------------------------------------
//  Image bank handles (name = original object name in the exe)
// ------------------------------------------------------------

enum PakImg {
    // --- disclaimer ("Frame 17") ---
    IMG_WARNING_TEXT   = 605,  // "WARNING! This game contains flashing lights,
                               //  loud noises, and lots of jumpscares!" 465x124

    // --- title menu ("title") ---
    IMG_MENU_BG        = 431,  // menu background, Freddy dark 1280x720
    IMG_MENU_FLICK1    = 440,  // Freddy lit up (rare twitch) 1280x720
    IMG_MENU_FLICK2    = 441,  // 1280x720
    IMG_MENU_FLICK3    = 442,  // 1280x720
    IMG_MENU_LOGO      = 444,  // "Five Nights at Freddy's" text block 201x212
    IMG_NEW_GAME       = 448,  // "New Game" 203x33   (hotspot = center)
    IMG_CONTINUE       = 449,  // "Continue" 204x34   (hotspot = center)
    IMG_MENU_ARROW     = 450,  // ">>" cursor 43x26   (hotspot = center)
    IMG_STAR           = 432,  // star 57x55          (hotspot = center)
    IMG_NIGHT_WORD     = 475,  // "Night" 63x22 (menu)
    IMG_SIXTH_NIGHT    = 443,  // "6th Night" 227x44  (hotspot = center)
    IMG_CUSTOM_NIGHT   = 526,  // "Custom Night" 306x44 (hotspot = center)
    IMG_COPYRIGHT      = 433,  // "(c)2014 Scott Cawthon" 226x14
    IMG_DEMO           = 572,  // "Demo" watermark 99x32
    IMG_VERSION        = 588,  // "v 1.132" 89x15
    IMG_MENU_STRIPE    = 452,  // white stripe 1328x32 (parked offscreen)

    // --- static / blip overlays (1280x720 each) ---
    // menu static cycle (object "static", 8 frames)
    // blip flash = white-noise flash frames (what day / died / menu flicker)
    IMG_BLIP_FULLWHITE = 23,   // FULLY WHITE 1280x720 -- reusable tint rect

    // --- night start cards ("what day": "12:00 AM / Nth Night") ---
    IMG_NIGHT1_CARD    = 453,
    IMG_NIGHT2_CARD    = 454,
    IMG_NIGHT3_CARD    = 472,
    IMG_NIGHT4_CARD    = 473,
    IMG_NIGHT5_CARD    = 474,
    IMG_NIGHT6_CARD    = 446,  // NOTE: 446 is the SIXTH night, not the first!
    IMG_NIGHT7_CARD    = 538,

    // --- office scene ("Frame 1", 1600x720, viewed through a 1280x720 pan
    //     window; original pan range 0..320) ---
    IMG_OFFICE_BG      = 39,   // office panorama 1600x720
    IMG_OFFICE_FRAME   = 11,   // img_11 is 100% TRANSPARENT (A=0) -- draws nothing;
    IMG_OFFICE_LIGHTS  = 608,  // ceiling string lights 1600x253 @ (0,-78)
    IMG_FAN_0          = 57,   // desk fan, 3 blade frames 138x196 @ (868,400)
    IMG_FAN_1          = 59,
    IMG_FAN_2          = 60,
    // v2.7.8: light-button panorama variants -- obj 44 "Active 3"
    // anims 18/34 (left) and 19/43 (right); event groups 119-129.
    // Full evidence chain: docs/OFFICE_FX.md.
    IMG_LIGHT_L_HALL   = 58,   // left light on, Bonnie not at the door
    IMG_LIGHT_L_BONNIE = 225,  // left light on, Bonnie in the doorway
    IMG_LIGHT_R_HALL   = 127,  // right light on, Chica not at the door
    IMG_LIGHT_R_CHICA  = 227,  // right light on, Chica in the doorway

    IMG_PUMPKIN_0      = 628,  // desk pumpkin (demo build), 7 frames 143x150
    IMG_PUMPKIN_6      = 635,  // handles 628,630..635 (629 does not exist)

    IMG_DOOR_L_OPEN    = 103,  // left doorway (open) 223x720 @ (72,-1)
    IMG_DOOR_L_MID     = 88,   // door roll frame
    IMG_DOOR_L_CLOSED  = 102,  // left door slab (hazard stripes) 223x720
    IMG_DOOR_R_OPEN    = 119,  // right doorway 248x720 @ (1270,-2)
    IMG_DOOR_R_MID     = 104,
    IMG_DOOR_R_CLOSED  = 118,  // right door slab 248x720

    // Button panels — states verified from pixels + events:
    //   img 122/124/125/130 = LEFT panel  @ instance (48,390),  hs (42,127)
    //   img 134/135/131/47  = RIGHT panel @ instance (1546,400), hs (49,127)
    //   122/134 = door CLOSED (red button), light off
    //   124/135 = door open (green button), light off
    //   125/131 = door closed + light on (glowing light button)
    //   130/47  = door open  + light on
    IMG_PANEL_L_CLOSED = 122,
    IMG_PANEL_L_OPEN   = 124,
    IMG_PANEL_L_CL_LT  = 125,
    IMG_PANEL_L_OP_LT  = 130,
    IMG_PANEL_R_CLOSED = 134,
    IMG_PANEL_R_OPEN   = 135,
    IMG_PANEL_R_CL_LT  = 131,
    IMG_PANEL_R_OP_LT  = 47,

    // "open/put down monitor" bar — the ONLY real bar art (600x60, rounded
    // dark slab with chevrons). Frame obj 'flip panel' @ instance (554,668),
    // hotspot (299,30). NOTE: the frame's other 'flip up'/'flip down'
    // objects (img_156 792x82 / img_162 1070x82) are SOLID VIOLET FILL
    // rectangles (123,43,127) — Clickteam zone markers, NOT meant to be
    // drawn opaque; v2.6 drew them and got the purple slab bug.
    IMG_FLIP_BAR       = 420,

    // --- office HUD (screen-fixed, counters are scrolling-independent) ---
    IMG_POWER_LABEL    = 207,  // "Power left:" 137x14 @ (106,638) center
    IMG_USAGE_LABEL    = 189,  // "Usage:" 72x14 @ (74,674) center
    IMG_PERCENT        = 208,  // "%" 11x14 (power digits suffix)
    IMG_AM             = 251,  // "AM" 42x26 @ (1198,31), above the hour digits
    IMG_NIGHT_WORD_OFC = 447,  // "Night" 63x14 @ (754,74) during the call
    IMG_MUTE_CALL      = 481,  // "MUTE CALL" button 121x31 @ (87,37)
    IMG_REC_DOT        = 483,  // red 8x8 dot

    // --- camera monitor (screen space) ---
    IMG_MONITOR_FRAME  = 11,   // same transparent img_11 dummy (kept: no-op)
    IMG_CAM_MAP        = 164,  // map 400x400 @ (848,313)
    IMG_CAM_BTN_ON     = 166,  // map cam button, green blink 60x40
    IMG_CAM_BTN_OFF    = 167,  // map cam button, normal gray 60x40
    IMG_AUDIO_ONLY     = 42,   // "CAMERA DISABLED / AUDIO ONLY" 371x54 @ (384,69)

    // camera location labels @ (832,292) — text verified from pixels:
    IMG_LOC_SHOWSTAGE  = 54,   // "Show Stage"
    IMG_LOC_DINING     = 72,   // "Dining Area"
    IMG_LOC_COVE       = 73,   // "Pirate Cove"
    IMG_LOC_WESTHALL   = 74,   // "West Hall"
    IMG_LOC_WHALL_COR  = 76,   // "W. Hall Corner"
    IMG_LOC_CLOSET     = 50,   // "Supply Closet"
    IMG_LOC_EASTHALL   = 79,   // "East Hall"
    IMG_LOC_EHALL_COR  = 75,   // "E. Hall Corner"
    IMG_LOC_BACKSTAGE  = 71,   // "Backstage"
    IMG_LOC_KITCHEN    = 78,   // "Kitchen"
    IMG_LOC_RESTROOMS  = 77,   // "Restrooms"

    // --- 6 AM sequence ("next day" frame) ---
    IMG_DIGIT_5        = 350,  // "5" 53x72  @ (544,298)
    IMG_DIGIT_6        = 351,  // "6" 53x72  @ (548,408)
    IMG_AM_BIG         = 352,  // "AM" 113x72 @ (640,296)
    IMG_FLIP_COVER     = 357,  // black cover block 158x118 (digit flip shade)

    // --- end screens (1280x720 full frames) ---
    IMG_END_NIGHT5     = 210,  // "Good job, sport! (see you next week)" $120.00
    IMG_END_NIGHT6     = 522,  // "Good job, sport! (You've earned some overtime!)" $120.50
    IMG_END_NIGHT7     = 523,  // "NOTICE OF TERMINATION (you're fired)"
    IMG_GAMEOVER_BG    = 358,  // game over backdrop (backstage room)
    IMG_END_DEMO       = 576,  // "Thanks for playing the demo!" 426x224

    IMG_GOLDEN_FREDDY  = 573,  // Golden Freddy slumped in office 541x521
    IMG_LOADING_SPIN   = 482,  // loading spinner 40x40
    IMG_CUST_ARROW_R   = 541,  // customize ">" arrow 34x52
    IMG_CUST_ARROW_L   = 542   // customize "<" arrow 34x52
};

// ------------------------------------------------------------
//  Hotspot table (instance position = hotspot position).
//  drawX = instanceX - hotspotX, drawY = instanceY - hotspotY
//  v2.6.1: the XDK compiler is MSVC 10.0 (C++03) — it REJECTS the C++11
//  brace-init form (PakHotspot( x, y ) -> C2143/C2275), so both structs
//  carry a real constructor and every call site uses PakHotspot(x, y).
// ------------------------------------------------------------
struct PakHotspot {
    i16 x; i16 y;
    PakHotspot() : x(0), y(0) {}
    PakHotspot(i32 px, i32 py) : x((i16)px), y((i16)py) {}
};

inline PakHotspot PakHotspotOf(int handle) {
    switch (handle) {
        case 605: return PakHotspot( -38,  -7 );
        case 444: return PakHotspot(  -3, -11 );
        case 448: return PakHotspot( 101,  16 );
        case 449: return PakHotspot( 102,  17 );
        case 450: return PakHotspot(  21,  13 );
        case 432: return PakHotspot(  28,  27 );
        case 433: return PakHotspot(   0,  -5 );
        case 443: return PakHotspot( 113,  22 );
        case 475: return PakHotspot(  -1,  -5 );
        case 526: return PakHotspot( 153,  22 );
        case 572: return PakHotspot(  -3, -13 );
        case 588: return PakHotspot(  -1,  -7 );
        case 446: return PakHotspot( 116,  48 );
        case 453: return PakHotspot( 113,  48 );
        case 454: return PakHotspot( 115,  48 );
        case 472: return PakHotspot( 115,  48 );
        case 473: return PakHotspot( 115,  48 );
        case 474: return PakHotspot( 115,  48 );
        case 538: return PakHotspot( 120,  50 );
        case 122: case 124: case 125: case 130: return PakHotspot( 42, 127 );
        case 134: case 135: case 131: case  47: return PakHotspot( 49, 127 );
        case 189: return PakHotspot(  36,   7 );
        case 207: return PakHotspot(  68,   7 );
        case 251: return PakHotspot(  -2,   0 );
        case 447: return PakHotspot(-394,   0 );
        case 481: return PakHotspot(  60,  15 );
        case  42: return PakHotspot( -80,   0 );
        case 166: case 167: return PakHotspot( 29, 19 );
        case 156: return PakHotspot( 367,  38 );
        case 162: return PakHotspot( 496,  38 );
        case 628: case 630: case 631: case 632:
        case 633: case 634: case 635: return PakHotspot( 71, 130 );
        case  57: case  59: case  60: return PakHotspot( 88, 97 );
        case 420: return PakHotspot(299, 30 );
        case 573: return PakHotspot( 270, 260 );
        case 350: case 351: case 352: return PakHotspot( -5, 0 );
        case 482: return PakHotspot(  20,  18 );
        default:  return PakHotspot(  0,   0 );
    }
}

// ------------------------------------------------------------
//  Camera feeds — v2.7.10 CANONICAL, read straight out of the original
//  event scripts (application.json, office frame):
//    * map-button highlight groups pin the viewing id per camera:
//      viewing==1:'1A show stage'  ==2:'1B dining area'  ==3:'cam 2A'
//      ==22:'cam 2B'  ==6:'cam 6 kitchen'  ==7:'cam 7 bathrooms'
//      ==5:'5 backstage'  ==42:'cam 4B'  ==33:'cam 3 closet'
//      ==4:'cam 4A'  ==99:'1C stage B'
//    * each click/highlight family sets exactly one "Active 3" (obj 44)
//      animation; the anim's frame list IS the feed image:
//      1A: anim17=19 full, 27=68 no-Bonnie, 41=223 no-Freddy,
//          42=224 Freddy-only, 60=484 / 54=355 empty
//      1B: anim13=48 empty, 28=90 Bonnie, 55=215 Chica
//      2A: anim14=43 empty, 33=206 figure, 15=44 / 39=221 Bonnie
//      2B: anim20=0 "LET'S PARTY!" empty, 59=479 Bonnie face,
//          58=478 Chica face, 74=540 rare golden poster,
//          75=571 Golden Freddy sitting (!), 69..72=549/550/551/552
//          RULES FOR SAFETY poster variants
//      3 : anim24=62 empty (single bulb closet), 31=190 Bonnie
//      4A: anim25=67 empty east hall, 39=221 Bonnie figure
//      4B: anim64=486 empty corner, 38=220 Chica figure
//      5 : anim22=83 BACKSTAGE (endo on table, heads shelf,
//          EMPLOYEES ONLY), 53=354 Bonnie standing, 32=205 Bonnie
//          dark face, 66=555 Bonnie face very close
//      6 : audio only (Active 3 hidden, obj 98 shown)
//      7 : anim21=41 restrooms, 36=217 Bonnie variant
//      1C: stage0 anim26=66 curtain closed, stage1 anim48=211 Foxy
//          peeking, stage2 anim49=338 Foxy out, stage3 anim50=240
//          gone (anim73=553 "IT'S ME" sign rare variant)
//  v2.7.4..v2.7.9 had 2B/3/4A/4B/5/7 feeds SCRAMBLED (each showed
//  another room's image; CAM 5 showed 540 = the 2B golden-poster
//  room) because the enum was filled from image vibes, not events.
// ------------------------------------------------------------
enum PakCamFeed {
    CAMFEED_NONE       = -1,   // kitchen sentinel: no feed, audio only
    CAMFEED_1A_FULL    = 19,
    CAMFEED_1A_NOBC    = 68,   // Chica + Freddy (Bonnie left)
    CAMFEED_1A_NOFC    = 223,  // Bonnie + Chica (Freddy left)
    CAMFEED_1A_FREDDY  = 224,  // Freddy only
    CAMFEED_1A_EMPTY   = 484,  // stage empty
    CAMFEED_1B_EMPTY   = 48,
    CAMFEED_1B_BONNIE  = 90,
    CAMFEED_1B_CHICA   = 215,
    COVE_CLOSED        = 66,   // stage 0: curtain shut, "Sorry! Out of Order"
    COVE_PEEK          = 211,  // stage 1: Foxy face in the gap
    COVE_OUT           = 338,  // stage 2: Foxy standing out, hook visible
    COVE_EMPTY         = 240,  // stage 3: cove empty, he is running
    CAMFEED_2A_EMPTY   = 43,
    CAMFEED_2A_FIGURE  = 206,
    CAMFEED_2A_BONNIE  = 226,
    CAMFEED_2B_EMPTY   = 0,    // img_0 "LET'S PARTY!" corner — handle 0 IS real
    CAMFEED_2B_BONNIE  = 479,  // Bonnie face staring into the camera
    CAMFEED_2B_CHICA   = 478,  // Chica face staring into the camera
    CAMFEED_3_EMPTY    = 62,
    CAMFEED_3_BONNIE   = 190,
    CAMFEED_4A_EMPTY   = 67,
    CAMFEED_4A_FIGURE  = 221,
    CAMFEED_4B_EMPTY   = 486,
    CAMFEED_4B_CHICA   = 220,
    CAMFEED_5_EMPTY    = 83,   // THE backstage: endo on table + heads shelf
    CAMFEED_5_BONNIE   = 555,
    CAMFEED_7_EMPTY    = 41,
    CAMFEED_7_VARIANT  = 217,
    // ---- v2.7.10: canonical variants kept for the graphics queue ----
    CAMFEED_2B_GOLDEN_POSTER = 540, // anim 74 (was wrongly CAM 5 empty!)
    CAMFEED_2B_GOLDENFREDDY  = 571, // anim 75 (easter egg, later)
    CAMFEED_5_BONNIE_STAND   = 354, // anim 53
    CAMFEED_5_BONNIE_DARK    = 205, // anim 32
    CAMFEED_1C_ITSME         = 553  // anim 73 (stage 3, rare sign variant)
};

// Map button positions on the cam map (instance coords, hotspot-centered):
//   1A(983,353) 1B(963,409) 1C(931,487) 2A(983,603) 2B(983,643)
//   3(899,585)  4A(1089,604) 4B(1089,644) 5(857,436) 6(1186,568) 7(1195,437)
struct PakMapBtn {
    i16 x; i16 y;
    PakMapBtn() : x(0), y(0) {}
    PakMapBtn(i32 px, i32 py) : x((i16)px), y((i16)py) {}
};
inline PakMapBtn PakMapButtonOf(int camIdx) {
    static const i16 xs[11] = { 983, 963, 931, 983, 983, 899, 1089, 1089, 857, 1186, 1195 };
    static const i16 ys[11] = { 353, 409, 487, 603, 643, 585,  604,  644, 436,  568, 437 };
    if (camIdx < 0 || camIdx > 10) return PakMapBtn( 0, 0 );
    return PakMapBtn( xs[camIdx], ys[camIdx] );
}

} // namespace fnaf

#endif // FNAF_PAK_ASSETS_H
