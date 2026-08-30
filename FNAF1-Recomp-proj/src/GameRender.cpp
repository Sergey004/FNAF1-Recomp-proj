/**
 * Five Nights at Freddy's 1 — Recompilation
 * GameRender.cpp: all screens drawn from fnaf1.pak assets
 *
 * Asset sources (ctfak-cpp analysis of the original exe):
 *  title frame "title" 1280x720  — objects: Active 2 (bg img_431, flickers
 *    440/441/442), static [18,20,12,13,14,15,16,17], new game img_448
 *    @(275,420), continue word img_449 @(275,492), night word img_475
 *    @(174,512), stars img_432 @(200/277/352,338), 6th night img_443,
 *    custom night img_526, demo img_572.
 *  office frame "Frame 1" 1600x720 (drawn at 0.8 scale):
 *    Active 3 img_39 bg, left door img_103 open / img_102 closed,
 *    right door img_119 open / img_118 closed, panels img_130 (open) /
 *    img_131 (closed) / img_135 (light on, open) / img_134 (light on,
 *    closed), mute call img_481 @(87,37), cam map img_164 @(848,313).
 *  monitor frame img_11; camera feeds from Active 3 anim table.
 *  jump scares: Active 3 anim 65 (Freddy), anim 52 (Foxy), anim 34 img_225
 *    (Bonnie), anim 43 img_227 (Chica); IT'S ME flash obj "Active 21"
 *    frames [525,543,520,544]; power-out flicker anim 51.
 */

#include "GameRender.h"
#include "XdkCompat.h"   // Snprintf — XDK CRT predates C99 snprintf
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "PakLoader.h"
#include "MenuSystem.h"
#include "Game.h"
#include <cstdio>
#include <cstring>
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
static const f32 OFFICE_SCALE = 0.8f;   // office frame is 1600x720

// ------------------------------------------------------------
//  Real animation frame tables (image handles from the game data)
// ------------------------------------------------------------

// Title/office static cycle — object "static" anim 0 (8 frames)
static const int STATIC_FRAMES[8] = { 18, 20, 12, 13, 14, 15, 16, 17 };

// Menu button art (title frame objects)
static const int TITLE_BG        = 431;
static const int TITLE_FLICKER[3] = { 440, 441, 442 };
static const int IMG_NEW_GAME    = 448;
static const int IMG_CONTINUE    = 449;
static const int IMG_MENU_ARROW  = 450;
static const int IMG_NIGHT_WORD  = 475;
static const int IMG_SIXTH_NIGHT = 443;
static const int IMG_CUSTOM_NIGHT= 526;
static const int IMG_STAR        = 432;

// Office (frame "Frame 1")
static const int OFFICE_BG       = 39;
static const int DOOR_L_CLOSED   = 102;
static const int DOOR_L_MID      = 88;
static const int DOOR_L_OPEN     = 103;
static const int DOOR_R_CLOSED   = 118;
static const int DOOR_R_MID      = 104;
static const int DOOR_R_OPEN     = 119;
static const int PANEL_DOOR_OPEN = 130;   // green door button + gray light
static const int PANEL_DOOR_SHUT = 131;   // red door button + gray light
static const int PANEL_LIGHT_OPEN= 135;   // light on, door open
static const int PANEL_LIGHT_SHUT= 134;   // light on, door closed
static const int IMG_MUTE_CALL   = 481;
static const int IMG_CAM_MAP     = 164;
static const int MONITOR_FRAME   = 11;

// Camera feeds (Active 3 anim table, empty-room variants, verified visually)
static const int CAM_FEED[11] = {
    19,   // CAM_1A show stage
    222,  // CAM_1B dining area
    66,   // CAM_1C pirate cove (curtain)
    43,   // CAM_2A west hall
    486,  // CAM_2B w. hall corner
    67,   // CAM_3  supply closet
    83,   // CAM_4A east hall
    41,   // CAM_4B e. hall corner
    540,  // CAM_5  backstage
    0,    // CAM_6  kitchen — audio only
    62    // CAM_7  restrooms
};
// Pirate cove stages (object 211/338/240 family, "1C stage B" anims)
static const int COVE_STAGES[4] = { 211, 338, 240, 240 };

// Power-out dark office (Active 3 anim 46) and flicker sequence (anim 51)
static const int POWEROUT_OFFICE = 476;
static const int FREDDY_FLICKER[12] = {
    241, 241, 241, 340, 244, 245, 246, 247, 248, 250, 280, 282
};

// Jump scare sequences (Active 3 anims 65 / 52; single frames for B/C)
static const int SCARE_FREDDY[31] = {
    519, 485, 521, 489, 490, 491, 493, 495, 496, 497, 498, 499, 500, 501,
    502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515,
    516, 517, 518
};
static const int SCARE_FOXY[25] = {
    413, 242, 415, 243, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405,
    406, 407, 408, 409, 410, 411, 412, 412, 412, 412, 412
};
static const int SCARE_BONNIE = 225;   // Active 3 anim 34
static const int SCARE_CHICA  = 227;   // Active 3 anim 43
static const int GOLDEN_FREDDY= 571;   // Active 3 anim 75

// IT'S ME hallucination flash (frame 3 object "Active 21" anim 0)
static const int ITSME_FRAMES[4] = { 525, 543, 520, 544 };

static const char* CAM_NAMES[11] = {
    "CAM 1A - Show Stage",  "CAM 1B - Dining Area", "CAM 1C - Pirate Cove",
    "CAM 2A - West Hall",   "CAM 2B - W. Hall Corner",
    "CAM 3 - Supply Closet","CAM 4A - East Hall",   "CAM 4B - E. Hall Corner",
    "CAM 5 - Backstage",    "CAM 6 - Kitchen",      "CAM 7 - Restrooms"
};

GameRender::GameRender()
    : m_batch(0), m_text(0), m_pak(0)
    , m_time(0.0f), m_staticTime(0.0f), m_staticIndex(0)
    , m_cacheCount(0)
{
}

void GameRender::Init(SpriteBatch* batch, TextRenderer* text, PakLoader* pak) {
    m_batch = batch; m_text = text; m_pak = pak;
    m_cacheCount = 0;
}

void GameRender::Tick(f32 dt) {
    m_time += dt;
    m_staticTime += dt;
    // static cycle ~12 fps like the original's 8-frame animation
    if (m_staticTime >= 1.0f / 12.0f) {
        m_staticTime -= 1.0f / 12.0f;
        m_staticIndex = (m_staticIndex + 1) % 8;
    }
}

PakLoadedTexture* GameRender::Tex(const char* name) {
    if (!m_pak || !name) return 0;
    for (int i = 0; i < m_cacheCount; ++i) {
        if (strcmp(m_cache[i].name, name) == 0) return m_cache[i].tex;
    }
    PakLoadedTexture* t = m_pak->FindTexture(name);
    if (m_cacheCount < 8) {
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
    m_batch->Draw(t->texture, x, y, w, h, color);
}

void GameRender::DrawFrame(int imgHandle, float x, float y, float w, float h, u32 color) {
    if (imgHandle <= 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", imgHandle);
    DrawTex(name, x, y, w, h, color);
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

// ------------------------------------------------------------
//  v2.8.0 CRT EFFECT — the Steam-page look
//  The heavy TV noise itself was never missing: it is the original's
//  8-frame "static" animation from the pak (STATIC_FRAMES above), drawn
//  by DrawStaticOverlay on every screen. What this adds on top is the
//  CRT pass — procedural pixel shader (SpriteBatch::DrawCRT):
//    * scanlines  — one dark line every 3 px
//    * vignette   — corner darkening (strongest on cameras, like the
//                   Steam screenshots)
//    * fine grain — per-pixel shimmer between the 12 fps static frames
//  Called from main.cpp right before FrameEnd() so it also covers the
//  menu text. Tunables live in ApplyCRT() in main.cpp + docs/CRT_EFFECT.md.
// ------------------------------------------------------------
void GameRender::DrawCRTOverlay(f32 scan, f32 vignette, f32 grain) {
    if (!m_batch) return;
    // Wrap time to [0,64): keeps the shader hash precise after hours of play
    f32 t = m_time - 64.0f * (f32)((i32)(m_time * (1.0f / 64.0f)));
    m_batch->DrawCRT(t, scan, vignette, grain);
}

// ------------------------------------------------------------
//  Disclaimer — first screen of the game
//  (title frame String obj 0: "WARNING!\n\nThis game contains flashing
//   lights, loud noises, and lots of jumpscares!" 544x259, centered by
//   the boot events; static overlay + static2 loop run behind it)
// ------------------------------------------------------------

void GameRender::RenderDisclaimer(bool blinkOn) {
    if (!m_batch || !m_text) return;
    static const char* LINE1 = "WARNING!";
    static const char* LINE2 = "This game contains flashing lights,";
    static const char* LINE3 = "loud noises, and lots of jumpscares!";
    const i32 cx = (i32)(SCREEN_W * 0.5f);
    const i32 cy = (i32)(SCREEN_H * 0.42f);
    m_text->DrawTextCenteredXY(cx, cy, LINE1, 0xFFFFFFFF);
    m_text->DrawTextCenteredXY(cx, cy + 60, LINE2, 0xFFDDDDDD);
    m_text->DrawTextCenteredXY(cx, cy + 92, LINE3, 0xFFDDDDDD);
    if (blinkOn) {
        m_text->DrawTextCenteredXY(cx, (i32)(SCREEN_H * 0.8f), "PRESS  START", 0xFF909090);
    }
    DrawStaticOverlay(0.35f);
    DrawCRTOverlay(0.10f, 0.22f, 0.05f);   // v2.8.0 CRT
}

// ------------------------------------------------------------
//  Title menu — original art at original coordinates (1280x720)
// ------------------------------------------------------------

void GameRender::RenderTitle(const MenuSystem& menu, bool hasSave, i32 stars) {
    if (!m_batch) return;

    // Background with rare twitch (objects "Active 2" anims 12/13/14)
    int bg = TITLE_BG;
    const int tw = (int)(m_time * 3.0f) % 23;   // ~every 7-8 s, 1-2 frames
    if (tw == 7)  bg = TITLE_FLICKER[0];
    if (tw == 15) bg = TITLE_FLICKER[1];
    DrawFrame(bg, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);

    // "demo" watermark stays hidden in the retail build (data position kept
    // for reference: img_572 @(171,292), img_588 @(26,682))
    DrawFrame(433, 1044.0f, 686.0f, 200.0f, 26.0f, 0xFFFFFFFF);

    // Menu entries — original art at exact data positions (1280x720 frame,
    // sizes = real image dims from the pak manifest)
    const i32 sel = menu.GetMainSelection();
    DrawFrame(IMG_NEW_GAME, 275.0f, 420.0f, 203.0f, 33.0f, 0xFFFFFFFF);
    if (hasSave) DrawFrame(IMG_CONTINUE, 275.0f, 492.0f, 204.0f, 34.0f, 0xFFFFFFFF);
    if (menu.GetUnlockedNight() >= 6) DrawFrame(IMG_SIXTH_NIGHT, 285.0f, 571.0f, 227.0f, 44.0f, 0xFFFFFFFF);
    if (menu.GetUnlockedNight() >= 7) DrawFrame(IMG_CUSTOM_NIGHT, 324.0f, 639.0f, 306.0f, 44.0f, 0xFFFFFFFF);

    // Stars for completed nights
    for (int i = 0; i < stars && i < 3; ++i) {
        static const f32 sx[3] = { 200.0f, 277.0f, 352.0f };
        DrawFrame(IMG_STAR, sx[i], 338.0f, 57.0f, 55.0f, 0xFFFFFFFF);
    }

    // "night word" + continue night number
    if (hasSave) {
        DrawFrame(IMG_NIGHT_WORD, 174.0f, 512.0f, 63.0f, 22.0f, 0xFFFFFFFF);
        char nb[16];
        Snprintf(nb, sizeof(nb), "%d", menu.GetSelectedNight());
        m_text->DrawText(263, 535, nb, 0xFFFFFFFF);
    }

    // Selection arrow: original "Active 4" img_450 — draw beside active row.
    // Rows: 0 New Game @420, 1 Continue @492, 2 6th night @571, 3 custom @639
    f32 ay = 420.0f;
    if (sel == 1 && hasSave) ay = 492.0f;
    else if (sel == 1) ay = 420.0f;
    else if (sel == 2 && menu.GetUnlockedNight() >= 6) ay = 571.0f;
    else if (sel >= 3 && menu.GetUnlockedNight() >= 7) ay = 639.0f;
    const bool blink = ((int)(m_time * 2.0f)) % 2 == 0;
    if (blink && sel <= 1) DrawFrame(IMG_MENU_ARROW, 132.0f, ay, 43.0f, 26.0f, 0xFFFFFFFF);

    // --------------------------------------------------------------
    // v2.8.0: system rows were INVISIBLE before (players pushed down
    // past Continue and landed in EXTRAS blind, over pure static).
    // They now live in a right-hand column: small dim text, white when
    // selected, arrow pointing at the active row.
    // --------------------------------------------------------------
    if (m_text) {
        struct SysRow { int opt; const char* label; f32 y; };
        static const SysRow rows[4] = {
            { 2, "SELECT NIGHT", 420.0f },
            { 3, "EXTRAS",       470.0f },
            { 4, "OPTIONS",      520.0f },
            { 5, "EXIT",         570.0f }
        };
        for (int r = 0; r < 4; ++r) {
            const bool on = (sel == rows[r].opt);
            m_text->DrawText(1000, (i32)rows[r].y, rows[r].label,
                             on ? 0xFFFFFFFF : 0xFF707070);
            if (on && blink) {
                DrawFrame(IMG_MENU_ARROW, 950.0f, rows[r].y - 6.0f, 43.0f, 26.0f, 0xFFFFFFFF);
            }
        }
        // build tag — on-screen version proof (the log banner lives in main.cpp)
        m_text->DrawText(20, 690, "build v2.8.0 (CRT)", 0xFF606060);
    }

    // Animated static overlay (object "static" on top in the original)
    DrawStaticOverlay(0.22f);

    // v2.8.0 CRT pass: subtle scanlines + vignette; grain stays low here
    // because the pak static already dominates this screen.
    DrawCRTOverlay(0.08f, 0.20f, 0.03f);
}

// ------------------------------------------------------------
//  Night start card: "Night N / 12 AM"
// ------------------------------------------------------------

void GameRender::RenderNightStart(i32 night) {
    if (!m_text) return;
    char b1[32], b2[16];
    Snprintf(b1, sizeof(b1), "%d AM", 12);
    Snprintf(b2, sizeof(b2), "Night %d", night);
    const i32 cx = (i32)(SCREEN_W * 0.5f);
    m_text->DrawTextCenteredXY(cx, (i32)(SCREEN_H * 0.40f), b1, 0xFFFFFFFF);
    m_text->DrawTextCenteredXY(cx, (i32)(SCREEN_H * 0.55f), b2, 0xFFCCCCCC);
}

// ------------------------------------------------------------
//  Office (1600x720 source, drawn at 0.8 scale)
// ------------------------------------------------------------

void GameRender::RenderOffice(const Game& game, bool phonePlaying) {
    if (!m_batch) return;
    const DoorSystem& doors = game.GetDoors();
    const f32 s = OFFICE_SCALE;

    DrawFrame(OFFICE_BG, 0, 0, 1600.0f * s, 720.0f * s, 0xFFFFFFFF);

    // Doors: open edge / closed slab (door toggle animation in the data is
    // a 16-frame roll; the closed/open slab swap reads correctly at 60 Hz)
    const bool lc = doors.IsDoorClosed(DOOR_LEFT);
    const bool rc = doors.IsDoorClosed(DOOR_RIGHT);
    const int dl = lc ? DOOR_L_CLOSED : DOOR_L_OPEN;
    const int dr = rc ? DOOR_R_CLOSED : DOOR_R_OPEN;
    DrawFrame(dl, 72.0f * s, -1.0f * s, 223.0f * s, 720.0f * s, 0xFFFFFFFF);
    DrawFrame(dr, 1270.0f * s, -2.0f * s, 248.0f * s, 720.0f * s, 0xFFFFFFFF);

    // Button panels: img_130 green (open) / img_131 red (closed);
    // light variants img_135/img_134 (data: panel 92x247 per side)
    const bool ll = doors.IsLightOn(DOOR_LEFT);
    const bool rl = doors.IsLightOn(DOOR_RIGHT);
    int lp = lc ? (ll ? PANEL_LIGHT_SHUT : PANEL_DOOR_SHUT)
                : (ll ? PANEL_LIGHT_OPEN : PANEL_DOOR_OPEN);
    int rp = rc ? (rl ? PANEL_LIGHT_SHUT : PANEL_DOOR_SHUT)
                : (rl ? PANEL_LIGHT_OPEN : PANEL_DOOR_OPEN);
    DrawFrame(lp, 40.0f * s, 297.0f * s, 92.0f * s, 247.0f * s, 0xFFFFFFFF);
    DrawFrame(rp, 1533.0f * s, 313.0f * s, 92.0f * s, 247.0f * s, 0xFFFFFFFF);

    // Bottom-right cam map button (object "Active 9" img_164 @(848,313))
    DrawFrame(IMG_CAM_MAP, 848.0f * s, 313.0f * s, 400.0f * s, 400.0f * s, 0xFFFFFFFF);

    // MUTE CALL (object img_481 @(87,37), 121x31) blinks while the phone plays
    if (phonePlaying) {
        const bool on = ((int)(m_time * 2.0f)) % 2 == 0;
        if (on) DrawFrame(IMG_MUTE_CALL, 87.0f * s, 37.0f * s, 121.0f * s, 31.0f * s, 0xFFFFFFFF);
    }

    // HUD — original uses counter digits; text placed at counter positions
    if (m_text) {
        const PowerSystem& pw = game.GetPower();
        char buf[64];
        Snprintf(buf, sizeof(buf), "%s", game.GetTimer().GetHourString());
        m_text->DrawText((i32)(1185.0f * s), (i32)(59.0f * s), buf, 0xFFFFFFFF);
        Snprintf(buf, sizeof(buf), "Power left: %d%%", (i32)(pw.GetPower() + 0.5f));
        m_text->DrawText((i32)(30.0f), (i32)(660.0f * s), buf, 0xFFFFFFFF);
        Snprintf(buf, sizeof(buf), "Usage: %d", pw.GetUsageLevel());
        m_text->DrawText((i32)(30.0f), (i32)(660.0f * s) + 30, buf, 0xFFAAAAAA);
    }

    // v2.8.0 CRT pass: office stays subtle (the original office view is clean)
    DrawCRTOverlay(0.05f, 0.12f, 0.03f);
}

// ------------------------------------------------------------
//  Camera monitor
// ------------------------------------------------------------

void GameRender::RenderCamera(const Game& game) {
    if (!m_batch) return;
    const CameraSystem& cams = game.GetCameras();
    const CameraId cam = cams.GetCurrentCamera();

    // Monitor bezel (object "frame" img_11 @0,-1)
    DrawFrame(MONITOR_FRAME, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);

    // Room feed
    int feed = 0;
    if (cam == CAM_1C) {
        const Animatronic& foxy = game.GetAI().GetAnimatronic(ANIM_FOXY);
        int st = (int)foxy.foxyStage;
        if (st < 0) st = 0;
        if (st > 3) st = 3;
        feed = COVE_STAGES[st];
    } else if (cam >= CAM_1A && cam <= CAM_7) {
        feed = CAM_FEED[(int)cam];
    }
    if (feed > 0) {
        DrawFrame(feed, 60.0f, 40.0f, 1160.0f, 560.0f, 0xFFFFFFFF);
    } else if (m_text) {
        m_text->DrawTextCenteredXY((i32)(SCREEN_W * 0.5f), (i32)(SCREEN_H * 0.45f),
                                 "CAMERA DISABLED", 0xFFEEEEEE);
        m_text->DrawTextCenteredXY((i32)(SCREEN_W * 0.5f), (i32)(SCREEN_H * 0.45f) + 40,
                                 "AUDIO ONLY", 0xFFAAAAAA);
    }

    // Heavy camera static
    DrawStaticOverlay(0.45f);

    // v2.8.0 CRT pass: the Steam-screenshot look — strongest here
    // (chunky scanlines + deep corner vignette over the feed)
    DrawCRTOverlay(0.16f, 0.45f, 0.06f);

    // Bottom-right map (object "Active 9" img_164) + label
    DrawFrame(IMG_CAM_MAP, 820.0f, 380.0f, 420.0f, 300.0f, 0xFFFFFFFF);
    if (m_text && cam >= CAM_1A && cam <= CAM_7) {
        const bool blink = ((int)(m_time * 2.0f)) % 2 == 0;
        if (blink) {
            m_text->DrawText(40, 40, "REC", 0xFFFF4040);
        }
        m_text->DrawText(40, 70, CAM_NAMES[(int)cam], 0xFFEEEEEE);
    }
}

// ------------------------------------------------------------
//  Power out: dark office + flickering Freddy (anim 51 frames)
// ------------------------------------------------------------

void GameRender::RenderPowerOut(const Game& game) {
    if (!m_batch) return;
    DrawFrame(POWEROUT_OFFICE, 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    // flicker: first 3 frames are plain darkness, then the sequence runs
    const int idx = (int)(game.GetPowerOutTimer() * 6.0f) % 12;
    DrawFrame(FREDDY_FLICKER[idx], 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    if (m_text) {
        m_text->DrawText(30, 660, "Power left: 0%", 0xFF808080);
    }
    DrawCRTOverlay(0.06f, 0.18f, 0.04f);   // v2.8.0 CRT
}

// ------------------------------------------------------------
//  Jump scares — real frame sequences from Active 3 anims 65/52/34/43
// ------------------------------------------------------------

void GameRender::RenderJumpscare(AnimatronicId anim, f32 elapsed) {
    if (!m_batch) return;

    // Shake like the original's alternating scare frames
    const int sh = ((int)(elapsed * 30.0f)) % 2;
    const f32 ox = (sh ? 10.0f : -10.0f);
    const f32 oy = (sh ? 6.0f : -6.0f);

    int frame = 0;
    if (anim == ANIM_FREDDY) {
        const int i = (int)(elapsed * 25.0f);
        frame = SCARE_FREDDY[i < 31 ? i : 30];
    } else if (anim == ANIM_FOXY) {
        const int i = (int)(elapsed * 25.0f);
        frame = SCARE_FOXY[i < 25 ? i : 24];
    } else if (anim == ANIM_BONNIE) {
        frame = SCARE_BONNIE;
    } else {
        frame = SCARE_CHICA;
    }
    DrawFrame(frame, ox, oy, SCREEN_W + 20.0f, SCREEN_H + 12.0f, 0xFFFFFFFF);

    // IT'S ME hallucination flash (obj "Active 21")
    const int fl = (int)(elapsed * 10.0f) % 4;
    if (fl == 1 || fl == 3) {
        DrawFrame(ITSME_FRAMES[fl], 0, 0, SCREEN_W, SCREEN_H, 0xFFFFFFFF);
    }
    DrawCRTOverlay(0.10f, 0.25f, 0.08f);   // v2.8.0 CRT
}

// ------------------------------------------------------------
//  6 AM / Game over
// ------------------------------------------------------------

void GameRender::RenderNightComplete(i32 night) {
    if (!m_batch || !m_text) return;
    const i32 cx = (i32)(SCREEN_W * 0.5f);
    m_text->DrawTextCenteredXY(cx, (i32)(SCREEN_H * 0.35f), "6 AM", 0xFFFFFFFF);
    char b[32];
    Snprintf(b, sizeof(b), "Night %d complete", night);
    m_text->DrawTextCenteredXY(cx, (i32)(SCREEN_H * 0.55f), b, 0xFFCCCCCC);
    DrawStaticOverlay(0.12f);
    DrawCRTOverlay(0.08f, 0.20f, 0.04f);   // v2.8.0 CRT
}

void GameRender::RenderGameOver() {
    if (!m_batch || !m_text) return;
    DrawStaticOverlay(0.55f);
    m_text->DrawTextCenteredXY((i32)(SCREEN_W * 0.5f), (i32)(SCREEN_H * 0.45f),
                             "GAME OVER", 0xFFEEEEEE);
    DrawCRTOverlay(0.12f, 0.30f, 0.06f);   // v2.8.0 CRT
}

} // namespace fnaf
