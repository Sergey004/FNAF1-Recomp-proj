/**
 * FNaF3Render.cpp: v2.61 — the FNAF3 renderer (title + the night loop).
 * Dump data: frame 1 "title" + frame 3 "Frame 1" placement tables and the
 * office-frame object set. Feed/monitor composition note: the dump's cam
 * feed values are prop overlays (the room feeds were not dumped as whole
 * frames), so the feed draws the base "camera screen" (img 104) with
 * img 206 standing in for "Springtrap is here" — labeled DEVIATION until
 * the per-cam pose art lands.
 */

#include "FNaF3Render.h"
#include "FNaF3Game.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <cstdio>

namespace fnaf {

static const f32 kScaleX = 1280.0f / 1024.0f;   // full-stretch 16:9
static const f32 kScaleY = 720.0f / 768.0f;

void FNaF3Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

void FNaF3Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  fw * kScaleX, fh * kScaleY, color);
}

// v2.67: native-size draw (the pak cells carry their own dimensions)
void FNaF3Render::DrawNat(int handle, float fx, float fy, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  (f32)t->origWidth * kScaleX, (f32)t->origHeight * kScaleY, color);
}

// v2.67: Clickteam counter from its CNTR glyph list (alphabet 0-9-+.e);
// v2.69 anchor fix: the instance point is the string's BOTTOM-RIGHT corner,
// digit rows butt-join +1px leftwards — proven on frame_3's HUD cluster
// (the "AM text" plate @(980,49) pairs with the hour counter @(968,65),
// the "Night" plate @(914,14) with the night counter @(1002,32)).
void FNaF3Render::DrawCounterNat(int value, const int* strip, float fx, float fy,
                                 u32 color) {
    char digits[12];
    Snprintf(digits, sizeof(digits), "%d", value < 0 ? 0 : value);
    f32 total = -1.0f;
    for (const char* c = digits; *c; ++c) {
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", strip[*c - '0']);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (!t || !t->texture || !m_batch) return;
        total += (f32)t->origWidth + 1.0f;
    }
    f32 x = fx - total;
    for (const char* c = digits; *c; ++c) {
        const int h = strip[*c - '0'];
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", h);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        m_batch->Draw(t->texture, x * kScaleX, (fy - (f32)t->origHeight) * kScaleY,
                      (f32)t->origWidth * kScaleX, (f32)t->origHeight * kScaleY, color);
        x += (f32)t->origWidth + 1.0f;
    }
}

void FNaF3Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = (wx - pan) * kScaleX;
    if (sx >= 1280.0f || sx + fw * kScaleX <= 0.0f) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, sx, wy * kScaleY, fw * kScaleX, fh * kScaleY, color);
}

int FNaF3Render::StaticFrame(f32 time) const {
    static const int kFrames[6] = { 37, 620, 33, 34, 35, 36 };
    const f32 period = 100.0f / 99.0f / 60.0f;
    return kFrames[(int)(time / period) % 6];
}

// ---- title (frame 1 "title", 1024x768) --------------------------------

void FNaF3Render::RenderDisclaimer() {
    // frame 0: the dump's legal splash = ONE centered banner (Backdrop
    // img 329, 477x90 at (292,308)); its String child sits off-screen in
    // the export and is never moved (the old paragraphs were invented).
    Draw(329, 292.0f, 308.0f, 477.0f, 90.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderTitle(const FNaF3Game& game, f32 time) {
    if (!m_batch || !m_pak) return;
    const i32 optionSelected = game.GetOptionSelected();

    // The scene is ALWAYS visible (bg img_862 + menu); the static appears
    // only as short glitch bursts (the dump has a burst timer on the
    // static object, groups 72/73 — roll every 40 s, ticks down; here a
    // ~0.1 s burst per 1.6 s slot approximates the flicker).
    Draw(862, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(592,  97.0f, 428.0f, 215.0f,  49.0f, 0xFFFFFFFF);  // "new game"
    Draw(301,  97.0f, 500.0f, 243.0f,  49.0f, 0xFFFFFFFF);  // "load game"
    // dump names objInfo 20 "6th night" (625) and objInfo 27 "custom night"
    // (826): BOTH hide while ini beatgame==0 (groups 44/45) — the nightmare
    // row and the extras row unlock together after night 5, like the
    // shipped game. (v2.71: was drawn unconditionally.)
    if (game.IsBeat5()) {
        Draw(625,  97.0f, 572.0f, 257.0f,  49.0f, 0xFFFFFFFF);  // nightmare
        Draw(826,  96.0f, 641.0f, 145.0f,  49.0f, 0xFFFFFFFF);  // extras
    }
    // the selector (img 833) rides the selected row (row pitch 72)
    {
        static const f32 kRowY[4] = { 429.0f, 501.0f, 573.0f, 642.0f };
        Draw(833,  38.0f, kRowY[optionSelected & 3],  34.0f,  49.0f, 0xFFFFFFFF);
    }

    // v2.71 sweep: the title art the dump shows but the port never drew —
    // the fade-line rules (img 1043/1061, 1024x30 at the dump's own y
    // rows), the footer rows (1021@(376,734), 849@(833,731), version art
    // 160@(916,706)) and the four progress stars (objInfo 21/22/36/51 =
    // img 840, 56x53; title init grp 27 reads the ini: beatgame/beat6/
    // goodend/4thstar). The demo ribbon (630) and the parked off-frame
    // pieces stay out — demo-only art is out of scope per the project.
    {
        static const f32 kRuleY[9] = { 64, 112, 158, 202, 246, 436, 508, 577, 646 };
        for (int i = 0; i < 9; ++i) DrawNat(1043, 0.0f, kRuleY[i], 0xFFFFFFFF);
    }
    DrawNat(1061, -1.0f, 690.0f, 0xFFFFFFFF);
    DrawNat(1061, -1.0f, 725.0f, 0xFFFFFFFF);
    DrawNat(1021, 376.0f, 734.0f, 0xFFFFFFFF);
    DrawNat(849, 833.0f, 731.0f, 0xFFFFFFFF);
    DrawNat(160, 916.0f, 706.0f, 0xFFFFFFFF);
    if (game.IsBeat5())      DrawNat(840, 155.0f,  58.0f, 0xFFFFFFFF);
    if (game.IsBeat6())      DrawNat(840, 232.0f,  58.0f, 0xFFFFFFFF);
    if (game.IsGoodEnd())    DrawNat(840, 193.0f, 108.0f, 0xFFFFFFFF);
    if (game.IsFourthStar()) DrawNat(840, 270.0f, 108.0f, 0xFFFFFFFF);

    {
        const f32 kPeriod = 1.6f;
        const int slot = (int)(time / kPeriod);
        u32 h = (u32)slot * 2654435761u + 3u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        if ((h % 100u) < 8u)                       // ~8% of slots flash
            Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    }
}

// ---- the between-night cards ------------------------------------------

void FNaF3Render::RenderNightStart(i32 night, f32 t) {
    // frame 2 "what day" — the FULL composition per the layout (v2.71, the
    // port used to draw only the plate and the card read "not centered"):
    // the fullscreen "blip flash" bands (objInfo 53, frames [869,863,871,
    // 865,866,867,868] ~30 fps), then the two "fade line" bars (img 1126,
    // 1024x30 at (0,319) and (0,391)), then the per-night plate ON TOP
    // (Active 55: dirs 0,12..17 -> imgs 1098,837,838,842,843,844,430 = night
    // 1..7 like the act #17 block, 201x104 hotspot-centered at (512,374) ->
    // (412,322)). The plate draws LAST: its alpha rows sit at local 0-29 and
    // 74-103 (= frame 322-351 and 396-425) — exactly where the two 30-px
    // bars land, so in the shipped game the text rides OVER the bars (the
    // layout table is objInfo-sorted, not the runtime z-order — the dump's
    // own replay draws it slivered; we follow the shipped visual and say so).
    static const i32 kBlip[7] = { 869, 863, 871, 865, 866, 867, 868 };
    static const i32 kPlate[7] = { 1098, 837, 838, 842, 843, 844, 430 };
    Draw(kBlip[((int)(t * 30.0f)) % 7], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    DrawNat(1126, 0.0f, 319.0f, 0xFFFFFFFF);
    DrawNat(1126, 0.0f, 391.0f, 0xFFFFFFFF);
    const i32 img = kPlate[night < 1 ? 0 : (night > 7 ? 6 : night - 1)];
    Draw(img, 412.0f, 322.0f, 201.0f, 104.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderStatic6() {
    // the nightmare-win hold: the dump's HUD strip spells "6 AM" —
    // digit '6' = img 306 of the [318,195,247,248,249,250,306,310,311,312,
    // 313,314,315,316] strip (alphabet 0-9-+.e) + the "AM" art img 246.
    if (m_batch) Draw(StaticFrame(0.0f), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    DrawNat(306, 560.0f, 330.0f, 0xFFFFFFFF);
    DrawNat(246, 590.0f, 330.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderNextDay(i32 night) {
    // frame 5 "next day": the dump's own set — the green scan band
    // (img 1136, 1024x30 at top), the "5:59" clock plate (obj "5", img
    // 1129, 341x72 at (359,341)) and the confetti rows (img 501) that rain
    // while the crowd cheers; no text of ours (the old caption invented).
    (void)night;
    DrawNat(1136, 0.0f, 0.0f, 0xFFFFFFFF);
    DrawNat(1129, 359.0f, 341.0f, 0xFFFFFFFF);
    {
        static const f32 kConfY[6] = { 317.0f, 335.0f, 355.0f, 378.0f, 392.0f, 411.0f };
        for (int i = 0; i < 6; ++i)
            DrawNat(501, (f32)((i % 3) * 341), kConfY[i], 0xFFFFFFFF);
    }
}

// ---- office (frame 3 "Frame 1", 200x768, pan window) ------------------

void FNaF3Render::RenderOffice(const FNaF3Game& game, f32 time) {
    if (!m_batch || !m_pak) return;
    const f32 pan = game.GetPan();

    // v2.66: the dump's OWN perspective (frame 3, "Perspective" obj 60 at
    // (-8,-8): object 1048x790, EDATA zoom=150 curve=4) wraps the office
    // layer — the mild bend FNAF3's office really has.
    m_batch->BeginSceneCapture(0xFF000000);

    // the office "foreground" states (dump objInfo 123: anim 0 = 203 normal,
    // anim 12 = 204 — the VENT-ERROR variant; groups 337/338/344 switch it
    // while the vent meter is <= -10, flickering under the hallucinations).
    // v2.66: the frame's BACKDROP (img 205, 2000x768 @0,0) draws FIRST —
    // 203/204 carry the transparent holes (the window, the wall gaps) and
    // the original shows the back layer through them (the "transparent"
    // blackness in the sweep frames was this missing backdrop).
    DrawWorld(205, 0.0f, 0.0f, 2000.0f, 768.0f, pan, 0xFFFFFFFF);
    const bool ventError = game.GetVentMeter() <= -10;
    if (ventError) {
        // the error state: the 204 foreground at the dump's alpha coefficient
        // 50 (group 346 sets it during the hallucination window)
        DrawWorld(203, 0.0f, 0.0f, 2000.0f, 768.0f, pan, 0xFFFFFFFF);
        u32 a = 200u;
        if (game.IsHallucinating())
            a = 120u + ((u32)((int)(time * 9.0f)) % 80u);
        DrawWorld(204, 0.0f, 0.0f, 2000.0f, 768.0f, pan, (a << 24) | 0xFFFFFFFFu);
    } else {
        DrawWorld(203, 0.0f, 0.0f, 2000.0f, 768.0f, pan, 0xFFFFFFFF);
    }

    // Springtrap at the office window (img 206) — the stage-4 scare pose
    if (game.GetSpringtrapRoom() == FNaF3Game::R3_ST4)
        DrawWorld(206, 1180.0f, 25.0f, 299.0f, 715.0f, pan, 0xFFFFFFFF);

    // the hall dash (img 207) — sweeps right-to-left while the timer runs
    if (game.GetRunPastT() > 0.0f) {
        const f32 k = 1.0f - game.GetRunPastT() / 0.6f;
        DrawWorld(207, 1300.0f - 600.0f * k, 0.0f, 250.0f, 768.0f, pan, 0xFFFFFFFF);
    }

    // he is INSIDE (the GOT YOU markers): the head over the doorway
    if (game.GetSpringtrapRoom() == FNaF3Game::R3_GY ||
        game.GetSpringtrapRoom() == FNaF3Game::R3_GY2)
        DrawWorld(529, 1000.0f, 400.0f, 164.0f, 220.0f, pan, 0xFFFFFFFF);

    // Phantom Foxy in the office (img 302; DEVIATION: fixed spot)
    if (game.GetPhFoxy() != 0)
        DrawWorld(302, 700.0f, 40.0f, 313.0f, 709.0f, pan, 0xFFFFFFFF);

    // Phantom Chica at the left window (stage 3)
    if (game.GetPhChica() == 3)
        DrawWorld(399, 40.0f, 60.0f, 646.0f, 654.0f, pan, 0xFFFFFFFF);

    // Golden Freddy's walk-across (img 653)
    if (game.GetPhGF() == 2) {
        const f32 k = game.GetPhGFWalkT() / 3.0f;
        DrawWorld(653, 1500.0f - 800.0f * k, 200.0f, 310.0f, 413.0f, pan, 0xFFFFFFFF);
    }

    // v2.71: NO invented office static. The office frame's events carry no
    // fullscreen noise object at all — the only "static" is "smallstatic",
    // the MONITOR's feed noise, gated by its own alterable chain. The old
    // 10%/30% duty-cycle glitch bursts over the office were made up.
    (void)time;

    // the vent-error blackout (img 676) fades over everything
    if (game.GetBlackoutAlpha() > 0.5f) {
        const u32 a = (u32)game.GetBlackoutAlpha();
        Draw(676, -11.0f, -3.0f, 1046.0f, 775.0f, (a << 24) | 0xFFFFFFFFu);
    }

    // the phantom-scare white flash (the dump's thin "fscare" bar stretched)
    if (game.GetWhiteFlash() > 0.0f) {
        const u32 a = (u32)(255.0f * (game.GetWhiteFlash() > 1.0f ? 1.0f : game.GetWhiteFlash()));
        Draw(362, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0xFFFFFFFFu);
    }

    // the layer-0 warp: end the capture and run the dump's own bend
    // (zoom 150 / curve 4 over the 1048x790 object at (-8,-8)).
    m_batch->EndSceneCapture();
    {   // vc100 (XDK) has no braced-init args: explicit local
        SpriteBatch::PerspGeom g;
        g.objW = 1048.0f; g.objH = 790.0f; g.orgX = -8.0f; g.orgY = -8.0f;
        g.winW = 1280.0f; g.winH = 720.0f;
        m_batch->SetPerspectiveGeometry(g);
    }
    m_batch->DrawPerspective(150.0f, 355.0f, 4.0f);

    DrawHud(game);
}

// v2.69: the layer-4 HUD cluster is REAL frame art (the v2.66 "no HUD in
// the original" reading was wrong — the FrameReplay tool just never emits
// Counter objects): the "Night" word plate img 245 @(914,14) pairs with the
// night-number counter @(1002,32), the "AM text" plate img 246 @(980,49)
// with the hour counter "time of night" @(968,65) — the counters' dump
// points are their BOTTOM-RIGHT corner, and the plates land flush against
// the digit rows. v2.71 correction: layer 4 is SCREEN-FIXED — it never
// scrolls with the frame (the dump points 914..1002 sit inside the 1024
// window, not the 2000 world; riding the pan parked the cluster mid-screen
// — caught on the user's capture). The Perspective warp touches only the
// office layer, so the HUD draws after it, unwarped.
// Glyph strip [318,195,247..316] = 15x22 (alphabet 0-9-+.e).
void FNaF3Render::DrawHud(const FNaF3Game& game) {
    if (!m_batch || !m_pak) return;
    static const int kHud[14] = { 318, 195, 247, 248, 249, 250, 306,
                                  310, 311, 312, 313, 314, 315, 316 };
    const i32 hour = game.GetTimeOfNight();                // 0 = 12 AM
    DrawNat(245, 914.0f, 14.0f, 0xFFFFFFFF);
    DrawCounterNat(game.GetNight(), kHud, 1002.0f, 32.0f, 0xFFFFFFFF);
    DrawNat(246, 980.0f, 49.0f, 0xFFFFFFFF);
    DrawCounterNat(hour == 0 ? 12 : hour, kHud, 968.0f, 65.0f, 0xFFFFFFFF);
}

// ---- monitor (feed + map + maintenance panel) ---------------------------
// Screen-space layout (labeled console-port composition — the PC original
// pans the world under a mouse cursor; the pad uses the fixed map):

struct MapBtn { f32 x, y; };
static const MapBtn kRoomBtn[10] = {           // dump world coords - (1570,396)
    {  74.0f, 280.0f }, { 289.0f, 252.0f }, { 370.0f, 210.0f }, { 370.0f, 145.0f },
    { 206.0f, 161.0f }, {  67.0f, 169.0f }, {  67.0f, 103.0f }, { 172.0f,  81.0f },
    { 238.0f,  38.0f }, { 346.0f,  74.0f }
};
static const MapBtn kVentBtn[5] = {
    {  71.0f,  10.0f }, { 133.0f, 128.0f }, { 218.0f, 188.0f },
    { 303.0f, 108.0f }, { 328.0f, 248.0f }
};
static const MapBtn kVentMark[5] = {
    {  46.0f,  69.0f }, {  75.0f, 141.0f }, { 167.0f, 201.0f },
    { 355.0f, 172.0f }, { 277.0f, 272.0f }
};
static const f32 kMapX = 740.0f, kMapY = 70.0f;

void FNaF3Render::RenderMonitor(const FNaF3Game& game, f32 time) {
    if (!m_batch || !m_pak) return;

    // the monitor frame (img 75) centered; the feed inside it
    Draw(75, 204.0f, 44.0f, 873.0f, 679.0f, 0xFFFFFFFF);
    const i32 cam = game.GetYouIn();

    // the feed: shadow-Freddy overrides cams 02/10 (imgs 61/60); the room
    // cams draw the dump's own anim cell (FNaF3Game::FeedImg) fit inside the
    // 825x650 feed — the cells are the props the original composites over
    // the base, drawn here at native aspect from the pak's texture dims
    if (game.IsShadowFreddy() && cam == 2)      Draw(61, 228.0f, 58.0f, 845.0f, 679.0f, 0xFFFFFFFF);
    else if (game.IsShadowFreddy() && cam == 10) Draw(60, 228.0f, 58.0f, 869.0f, 680.0f, 0xFFFFFFFF);
    else {
        Draw(104, 228.0f, 58.0f, 825.0f, 650.0f, 0xFFFFFFFF);
        const bool stHere = game.SpringtrapOnCam(cam);
        const i32 piece = FNaF3Game::FeedImg(cam, stHere);
        if (piece != 0 && piece != 104) {
            // fit the piece inside a centered 40x330 box, native aspect
            char pname[32];
            Snprintf(pname, sizeof(pname), "img_%d", piece);
            PakLoadedTexture* pt = m_pak ? m_pak->FindTexture(pname) : 0;
            if (pt && pt->texture) {
                const f32 pw = (f32)pt->origWidth, ph = (f32)pt->origHeight;
                const f32 k = (pw > 0.0f && ph > 0.0f)
                            ? (pw / ph > 400.0f / 330.0f ? 400.0f / pw : 330.0f / ph)
                            : 1.0f;
                const f32 dw = pw * k, dh = ph * k;
                Draw(piece, 228.0f + (825.0f - dw) * 0.5f,
                     58.0f + (650.0f - dh) * 0.5f, dw, dh, 0xFFFFFFFF);
            }
        }
    }

    // the phantom feed overlays
    if (game.GetPhBB() == 2 &&
        (cam == 1 || cam == 7 || cam == 9 || cam == 10))
        Draw(70, 228.0f, 58.0f, 825.0f, 650.0f, 0xFFFFFFFF);
    if (game.GetPhMangle() == 2 && cam == 4)
        Draw(202, 560.0f, 280.0f, 167.0f, 206.0f, 0xFFFFFFFF);
    if (game.GetPhChica() == 2 && cam == 7)
        Draw(399, 320.0f, 60.0f, 646.0f, 654.0f, 0xFFFFFFFF);
    if (game.GetPhPuppetRushT() > 0.0f)
        Draw(320, 240.0f, 0.0f, 800.0f, 768.0f, 0xFFFFFFFF);

    // him in the monitor (GOT YOU 2 while watching, img 143 fullscreen)
    if (game.IsBigScare() && game.GetSpringtrapRoom() == FNaF3Game::R3_GY2)
        Draw(143, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // the feed static: the switch burst + the glitch flicker
    {
        u32 h = (u32)(int)(time * 9.0f) * 2654435761u + 9u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        u32 alpha = 60u + (h % 50u);
        if (game.GetMoveStatic() > 0.0f)
            alpha = (u32)(120.0f + 135.0f * game.GetMoveStatic());
        Draw(39, 228.0f, 58.0f, 825.0f, 650.0f, (alpha << 24) | 0xFFFFFFFFu);
    }

    // the map / vent map overlay
    if (!game.IsPanelOpen()) RenderMap(game);
    else                     RenderPanel(game);

    // the vent-error blackout also darkens the monitor
    if (game.GetBlackoutAlpha() > 0.5f) {
        const u32 a = (u32)game.GetBlackoutAlpha();
        Draw(676, -11.0f, -3.0f, 1046.0f, 775.0f, (a << 24) | 0xFFFFFFFFu);
    }

    // v2.66: the FNAF3 monitor shows no text in the original (the errors
    // surface as the maintenance-panel icons and the 204 office state) —
    // but the layer-4 night/hour HUD cluster stays above the feed (the
    // original never hides it); draw it here too.
    DrawHud(game);
}

void FNaF3Render::RenderMap(const FNaF3Game& game) {
    const i32 sel = game.GetSelIdx();
    if (!game.IsVentMap()) {
        // the room map: 10 cam buttons + labels, the toggle at the bottom
        Draw(532, kMapX, kMapY, 430.0f, 400.0f, 0xFFFFFFFF);
        for (i32 i = 0; i < 10; ++i) {
            const u32 tint = (sel == i) ? 0xFF50A0FF : 0xFFFFFFFF;
            Draw(81, kMapX + kRoomBtn[i].x, kMapY + kRoomBtn[i].y, 60.0f, 40.0f, tint);
            Draw(83 + i, kMapX + kRoomBtn[i].x + 14.0f, kMapY + kRoomBtn[i].y + 8.0f,
                 29.0f, 25.0f, 0xFFFFFFFF);
        }
        Draw(426, kMapX + 190.0f, kMapY + 355.0f, 50.0f, 30.0f,
             (sel == 15) ? 0xFF50A0FF : 0xFFFFFFFF);     // the toggle button
    } else {
        // the vent map: 5 vent cams + the sealed/open markers
        Draw(532, kMapX, kMapY, 430.0f, 400.0f, 0xFFFFFFFF);
        for (i32 i = 0; i < 5; ++i) {
            const i32 ventCam = 11 + i;
            const u32 tint = (sel == i) ? 0xFF50A0FF : 0xFFFFFFFF;
            Draw(81, kMapX + kVentBtn[i].x, kMapY + kVentBtn[i].y, 60.0f, 40.0f, tint);
            // the vent marker: sealed = red tint (anim 0/12 of img 609 —
            // only one image was dumped, so the state is a tint)
            const u32 mt = (game.GetSealedVent() == ventCam) ? 0xFF5050FF : 0xFFFFFFFF;
            Draw(609, kMapX + kVentMark[i].x, kMapY + kVentMark[i].y, 40.0f, 27.0f, mt);
        }
        Draw(426, kMapX + 190.0f, kMapY + 355.0f, 50.0f, 30.0f,
             (sel == 15) ? 0xFF50A0FF : 0xFFFFFFFF);
        // the sealing progress bar (img 611) while a seal is armed
        if (game.GetSealTarget() != 0 && game.GetSealDuration() > 0.0f) {
            const f32 w = 165.0f * (game.GetSealProgress() / game.GetSealDuration());
            Draw(611, kMapX + 130.0f, kMapY + 300.0f, w, 30.0f, 0xFFFFFFFF);
        }
    }
    // v2.67: the dump's own overlay plates replace the debug caption —
    // "play text" img 366 / "toggle text" img 431 (room map) and "seal vent
    // text" img 80 (vent map), parked next to our toggle button (the dump
    // sits them beside its own, at (1461..1508, 625..640) world space).
    if (game.IsVentMap()) DrawNat(80,  kMapX + 250.0f, kMapY + 362.0f, 0xFFFFFFFF);
    else {
        DrawNat(366, kMapX +  10.0f, kMapY + 338.0f, 0xFFFFFFFF);
        DrawNat(431, kMapX + 250.0f, kMapY + 362.0f, 0xFFFFFFFF);
    }
}

void FNaF3Render::RenderPanel(const FNaF3Game& game) {
    // the maintenance screen (img 734) + the dump's text rows at their
    // instance points (frame 3, layer 4): audio 544 (206,357), camera 545
    // (206,417), ventilation 555 (206,471), reboot all 594 (206,578),
    // exit 600 (206,627); the error badges (img 601) at (506,362/418/475).
    // The old eye-balled rows and the debug hint text are gone.
    Draw(734, 160.0f, 300.0f, 720.0f, 430.0f, 0xFFFFFFFF);
    static const struct { i32 img; f32 y; i32 meter; } kRows[5] = {
        { 544, 357.0f, 0 },   // audio devices   (img 544)
        { 545, 417.0f, 0 },   // camera devices
        { 555, 471.0f, 0 },   // ventilation
        { 594, 578.0f, 0 },   // reboot all
        { 600, 627.0f, 0 }    // exit
    };
    for (i32 i = 0; i < 5; ++i) {
        u32 tint = 0xFFFFFFFF;
        if (i < 3) {
            const i32 m = (i == 0) ? game.GetAudioMeter()
                        : (i == 1) ? game.GetCameraMeter() : game.GetVentMeter();
            if (m <= -10) tint = 0xFF5050FF;               // the error rows
        }
        DrawNat(kRows[i].img, 206.0f, kRows[i].y, tint);
    }
    // the cursor (img 596) beside the selected row
    DrawNat(596, 166.0f, kRows[game.GetPanelCursor() - 1].y + 4.0f, 0xFFFFFFFF);
    // the error icons (img 601) in the dump's own slots
    if (game.GetAudioMeter()  <= -10) DrawNat(601, 506.0f, 362.0f, 0xFFFFFFFF);
    if (game.GetCameraMeter() <= -10) DrawNat(601, 506.0f, 418.0f, 0xFFFFFFFF);
    if (game.GetVentMeter()   <= -10) DrawNat(601, 506.0f, 475.0f, 0xFFFFFFFF);
    // the reboot progress bar (img 611) rides under the reboot-all row while
    // it runs (panel-relative placement — the dump parks its marker object,
    // so the bar spot is our layout, the ART is the dump's cell).
    if (game.GetRebooting() != 0) {
        const f32 w = 165.0f * (game.GetRebootProgress() / 10.0f);
        Draw(611, 206.0f, 520.0f, w, 24.0f, 0xFFFFFFFF);
    }
}

// ---- the office jumpscare ----------------------------------------------

void FNaF3Render::DrawAttack(const FNaF3Game& game) {
    const i32 img = game.GetScareImg();
    if (img == 778)      Draw(img, 128.0f, -80.0f, 900.0f, 870.0f, 0xFFFFFFFF);
    else if (img == 792) Draw(img, 190.0f, -60.0f, 900.0f, 645.0f, 0xFFFFFFFF);
}

// ---- the dump's end screens (v2.62) ------------------------------------

void FNaF3Render::RenderAd() {
    // frame 8: the "COMING SOON! Fazbear's Fright" newspaper (img 0)
    Draw(0, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderRare2() {
    // frame 13: the post-night glitch screen (img 228)
    Draw(228, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderEndScreen(i32 which) {
    // frame 9 "bad end" = img 346 / frame 10 "good end" = img 172 /
    // frame 11 "the end 2" = img 123 (the backdrops from the layouts);
    // the chooser (frame 17) draws its OWN plate (img 505, 1044x124 at
    // (478,364) — the "the end" title art) over the backdrop img 29
    if (which == 1)      Draw(346, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (which == 2) Draw(172, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (which == 3) Draw(123, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else {
        Draw(29, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(505, 478.0f, 364.0f, 1044.0f, 124.0f, 0xFFFFFFFF);
    }
}

// ============================================================
// v2.63 — the new flow screens, the cutscenes frame, the six Atari
// minigames and the extras menu.
// ============================================================

void FNaF3Render::RenderWait() {
    // frame 7 "wait": plain black for 100 ms (the batch clears black)
}

void FNaF3Render::RenderStaticDeath() {
    // frame 4: the static cycle + the white flash (img 272) flicker
    if (m_batch) {
        Draw(StaticFrame((f32)((int)(m_pak ? 0 : 0))), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        static f32 s_t = 0.0f;
        s_t += 1.0f / 60.0f;
        const int f = (int)(s_t * 12.0f) % 10;
        if (f < 4) Draw(272, 0.0f, 0.0f, 1024.0f, 768.0f, 0x50FFFFFF);
    }
}

void FNaF3Render::RenderGameOver() {
    // frame 6 "gameover": the dump's own composition — the dark backdrop
    // (img 339) + the static wash (img 37) + the green line band
    // ("green line", img 709, 1024x30 at (0,376)). NO words — the original
    // shows none (the debug-font caption was invented).
    Draw(339, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw( 37, 0.0f, 0.0f, 1024.0f, 768.0f, 0xB0FFFFFF);
    DrawNat(709, 0.0f, 361.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderRare(i32 id) {
    // frame 12 = img 225 / frame 13 = img 228 / frame 14 = img 252
    const int img = (id == 1) ? 225 : (id == 2) ? 228 : 252;
    Draw(img, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderLoad(f32 t) {
    // frame 18: the glitch loader — the "loading" band (img 698) + the
    // crawling line-jump dots (img 696) on three scan rows
    Draw(698, 470.0f, 330.0f, 1044.0f, 50.0f, 0xFFFFFFFF);
    static const f32 kRowY[3] = { 420.0f, 457.0f, 494.0f };
    for (int i = 0; i < 3; ++i) {
        const f32 phase = t * (0.8f + 0.35f * i);
        const int step = (int)(phase * 6.0f) % 4;
        for (int k = 0; k <= step && k < 4; ++k)
            Draw(696, 470.0f + k * 33.0f, kRowY[i], 32.0f, 32.0f, 0xFFFFFFFF);
    }
    // v2.67: the frame draws NO text in the original (the bars/dots park
    // off-screen and the events move them in) — the "LOADING" debug line
    // is gone.
}

// ---- the cutscenes frame (16) ------------------------------------------

void FNaF3Render::RenderCutscene(const FNaF3Game& game, f32 time) {
    const FNaF3Game::CutsceneState& cs = game.Cs();
    if (!m_batch) return;

    // the room: wall band (img 31) + floor (img 42) + the checkered floor
    // in the dump's checker cells + the CRT vignette (1001)
    for (int i = 0; i < 4; ++i) Draw(31, (f32)(i * 310), -8.0f, 310.0f, 88.0f, 0xFFFFFFFF);
    for (int i = 0; i < 4; ++i) Draw(31, (f32)(i * 310), 680.0f, 310.0f, 88.0f, 0xFFFFFFFF);
    for (int x = 0; x < 10; ++x)
        for (int y = 0; y < 5; ++y)
            Draw(42, (f32)(x * 102), (f32)(80 + y * 120), 102.0f, 134.0f, 0xFFFFFFFF);
    {
        // checker cells: (2,2)(2,3)(2,4)(3,2)(3,4)(3,5)(1,5)(4,2)(4,4)(5,3)
        static const u8 kChecker[10][2] = {
            {2,2},{2,3},{2,4},{3,2},{3,4},{3,5},{1,5},{4,2},{4,4},{5,3}
        };
        for (int i = 0; i < 10; ++i)
            if (kChecker[i][0] == cs.v && kChecker[i][1] == cs.h)
                Draw(335, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    }

    // the stage room (2,3): stage + curtain + the fixed chica/bonnie
    if (cs.v == 2 && cs.h == 3) {
        Draw(333, 185.0f, 183.0f, 626.0f, 258.0f, 0xFFFFFFFF);
        Draw(327, -71.0f, 151.0f, 200.0f, 442.0f, 0xFFFFFFFF);
        Draw(330, 593.0f, 121.0f, 131.0f, 167.0f, 0xFFFFFFFF);
        Draw(331, 282.0f, 104.0f, 121.0f, 196.0f, 0xFFFFFFFF);
    }
    if (cs.v == 5 && cs.h == 3) Draw(326, 300.0f, 200.0f, 351.0f, 303.0f, 0xFFFFFFFF);
    if (cs.v == 2 && cs.h == 1) Draw(340, 100.0f, 200.0f, 447.0f, 379.0f, 0xFFFFFFFF);
    if (cs.v == 1 && cs.h == 5) {
        // the finale room: the arcades + the Springtrap suit
        Draw(457, 143.0f, 110.0f, 140.0f, 206.0f, 0xFFFFFFFF);
        Draw(457, 313.0f, 106.0f, 140.0f, 206.0f, 0xFFFFFFFF);
        Draw(457, 489.0f, 104.0f, 140.0f, 206.0f, 0xFFFFFFFF);
        Draw(364, 117.0f, 229.0f, 409.0f, 157.0f, 0xFFFFFFFF);
        Draw(364, 512.0f, 389.0f, 409.0f, 157.0f, 0xFFFFFFFF);
        // the suit + its crush stages (img 471 family)
        static const int kSuit[7] = { 471, 471, 472, 473, 486, 490, 495 };
        const int simg = kSuit[cs.suitStage < 6 ? cs.suitStage : 6];
        Draw(simg, 700.0f, 300.0f, 222.0f, 220.0f, 0xFFFFFFFF);
        if (cs.suitStage >= 1 && cs.suitStage < 6) {
            // the Purple Guy bolting for the suit (img 458/460 run cells)
            const int run = ((int)(time * 10.0f) % 2) ? 458 : 460;
            f32 rx = cs.patrolX;
            if (cs.suitStage >= 1) rx = 100.0f + 800.0f * (cs.suitT / 100.0f);
            Draw(run, rx, 480.0f, 200.0f, 230.0f, 0xFFFFFFFF);
        } else if (!cs.finaleOn || cs.suitStage == 0) {
            // the patrol (img 453/455 by leg)
            const int p = ((int)(time * 10.0f) % 2) ? 453 : 455;
            Draw(p, cs.patrolX, 480.0f, 200.0f, 230.0f, 0xFFFFFFFF);
        }
    } else if (cs.scene < 5) {
        // the decommission rooms: the shadow figure of this cell
        for (int i = 0; i < 3; ++i) {
            if (!cs.shadowOn[i]) continue;
            static const int kSh[3][2] = { {375, 376}, {379, 380}, {377, 378} };
            const int s = ((int)(time * 3.0f) % 2);
            f32 sx = 500.0f, sy = 400.0f;
            if (i == 0) { sx = 700.0f + cs.shadowX[i]; }
            if (i == 1) { sy = 120.0f + cs.shadowX[i]; }
            if (i == 2) { sy = 640.0f - cs.shadowX[i]; }
            Draw(kSh[i][s], sx, sy, 200.0f, 200.0f, 0x90FFFFFF);
        }
        // the Purple Guy hunting (img 382/383 @10)
        if (cs.manStage >= 1) {
            const int m = ((int)(time * 10.0f) % 2) ? 382 : 383;
            Draw(m, cs.manX, cs.manY, 200.0f, 230.0f, 0xFFFFFFFF);
        }
        // the take-apart + the parts of this scene
        if (cs.manStage == 2) {
            static const int kParts[5] = { 0, 401, 410, 420, 452 };
            const int kTake[2] = { 386, 400 };
            Draw(kTake[(int)(time * 10.0f) % 2], cs.px - 91.0f, cs.py - 101.0f,
                 200.0f, 230.0f, 0xFFFFFFFF);
            Draw(kParts[cs.scene], cs.px - 145.0f, cs.py - 60.0f, 348.0f, 107.0f,
                 0xFFFFFFFF);
        }
    }

    // the player (scene sprite: 1 = Freddy img 21, 2-4 = the walk pairs,
    // 5 = the ghost child), 20x230 like the dump hot boxes
    if (cs.manStage < 2 && !(cs.finaleOn)) {
        int img = 21;
        const int leg = ((int)(time * 4.0f) % 2);
        if (cs.scene == 2) img = leg ? 402 : 403;
        if (cs.scene == 3) img = leg ? 412 : 413;
        if (cs.scene == 4) img = leg ? 444 : 445;
        if (cs.scene == 5) img = leg ? 421 : 422;
        Draw(img, cs.px - 106.0f, cs.py - 99.0f, 200.0f, 200.0f, 0xFFFFFFFF);
    }

    // the ERR popup + the hint + the ambience
    if (cs.errT > 0.0f) Draw(371, 222.0f, 618.0f, 198.0f, 95.0f, 0xFFFFFFFF);
    if (cs.hintT > 0.0f) Draw(348, 414.0f, 590.0f, 573.0f, 72.0f, 0xFFFFFFFF);
    for (int i = 0; i < cs.rainN; ++i)
        Draw(368, cs.dropX[i], cs.dropY[i], 18.0f, 32.0f, 0xFF606060);
    if (cs.ratX > 0.0f && cs.ratX < 1024.0f)
        Draw(373, cs.ratX, 600.0f, 66.0f, 34.0f, 0xFFFFFFFF);
    // the scanline overlays + the CRT vignette
    Draw(268, -40.0f, (f32)((int)(time * 30.0f) % 800) - 16.0f, 1104.0f, 32.0f, 0x30FFFFFF);
    Draw(1001, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(291, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

// ---- the six minigames (frames 19-24) -----------------------------------

void FNaF3Render::RenderMinigame(const FNaF3Game& game, f32 time) {
    const FNaF3Game::Mg3State& mg = game.Mg();
    if (!m_batch) return;
    const f32 ox = -mg.camX, oy = -mg.camY;   // world -> screen offset

    // the black page tiles (img 721 family) over the 3072x2304 world
    static const int kBg[7] = { 0, 721, 724, 724, 724, 724, 724 };
    for (int x = 0; x < 3; ++x)
        for (int y = 0; y < 3; ++y)
            Draw(kBg[mg.game], ox + (f32)(x * 1024), oy + (f32)(y * 768),
                 1024.0f, 768.0f, 0xFFFFFFFF);

    // the platform kit (the dump's platform images at their layout spots —
    // the same numbers as the collision tables in FNaF3Game.cpp)
    {
        struct Plat { int img; f32 x, y, w, h; };
        static const Plat kBB[] = {
            { 712, 106, 668, 762, 30 }, { 712, 106, 132, 762, 30 },
            { 710,  36,  28, 543, 42 }, { 712, 1828, 1160, 762, 30 },
            { 712, 2180, 1652, 762, 30 }, { 712, 2180, 2198, 762, 30 },
            { 718, 610, 554, 164, 30 }, { 718, 346, 488, 164, 30 },
            { 718, 134, 400, 164, 30 }, { 718, 434, 320, 164, 30 },
            { 718, 700, 294, 164, 30 }, { 718, 2334, 2096, 164, 30 },
            { 718, 2536, 1992, 164, 30 }, { 718, 2758, 1904, 164, 30 },
            { 718, 2466, 1828, 164, 30 }, { 725, 839, 134, 32, 562 },
            { 733, 2910, 1652, 32, 572 }
        };
        static const Plat kMangle[] = {
            { 874, 106, 668, 2736, 30 }, { 874, 106, 132, 2736, 30 },
            { 904, 148, 810, 890, 30 },
            { 896, 1132, 196, 358, 92 }, { 896, 1382, 196, 358, 92 },
            { 896, 1698, 432, 358, 92 }, { 896, 2184, 182, 358, 92 },
            { 877, 610, 554, 164, 30 }, { 877, 1392, 342, 164, 30 },
            { 877, 1694, 374, 164, 30 }, { 877, 1334, 552, 164, 30 },
            { 877, 1106, 422, 164, 30 }, { 877, 2678, 412, 164, 30 },
            { 877, 2096, 456, 164, 30 }, { 877, 2346, 550, 164, 30 },
            { 877, 2386, 398, 164, 30 }, { 878, 2838, 340, 32, 358 }
        };
        static const Plat kTC[] = {
            { 918, 106, 670, 762, 30 }, { 906, 106, 132, 762, 30 },
            { 919, 1660, 670, 762, 30 }, { 906, 2180, 1652, 762, 30 },
            { 906, 2180, 2198, 762, 30 }, { 919, 346, 1358, 762, 30 },
            { 919, 904, 1358, 762, 30 }, { 919, 1669, 1357, 762, 30 },
            { 909, 1206, 560, 164, 30 }, { 909, 1476, 412, 164, 30 },
            { 909, 1632, 412, 164, 30 }, { 909, 1882, 280, 164, 30 },
            { 909, 1522, 752, 164, 30 }, { 909, 1328, 852, 164, 30 },
            { 909, 1074, 948, 164, 30 }, { 909, 1300, 1062, 164, 30 },
            { 909, 2334, 2096, 164, 30 }, { 909, 2536, 1992, 164, 30 },
            { 909, 2758, 1904, 164, 30 }, { 909, 2466, 1828, 164, 30 }
        };
        static const Plat kGF[] = {
            { 930, 106, 668, 762, 30 }, { 930, 106, 132, 762, 30 },
            { 930, 1146, 654, 762, 30 }, { 930, 1146, 118, 762, 30 },
            { 930, 2194, 650, 762, 30 }, { 930, 2194, 114, 762, 30 },
            { 930, 108, 884, 762, 30 }, { 930, 108, 1420, 762, 30 },
            { 930, 1154, 882, 762, 30 }, { 930, 1154, 1418, 762, 30 },
            { 933, 134, 580, 164, 30 }, { 933, 114, 1330, 164, 30 },
            { 933, 1160, 1328, 164, 30 }, { 933, 1152, 564, 164, 30 },
            { 729, 178, 210, 226, 128 }, { 729, 566, 352, 226, 128 },
            { 729, 1226, 960, 226, 128 }, { 729, 1614, 1102, 226, 128 },
            { 729, 1218, 196, 226, 128 }, { 729, 1606, 338, 226, 128 },
            { 934, 839, 134, 32, 562 }
        };
        static const Plat kRWQ[] = {
            { 930, 106, 668, 762, 30 }, { 930, 106, 132, 762, 30 },
            { 712, 2194, 648, 762, 30 }, { 712, 2194, 112, 762, 30 },
            { 933, 134, 580, 164, 30 },
            { 877, 714, 1150, 164, 30 }, { 877, 132, 1194, 164, 30 },
            { 877, 382, 1288, 164, 30 }, { 877, 422, 1136, 164, 30 },
            { 718, 2698, 534, 164, 30 }, { 718, 2434, 468, 164, 30 },
            { 718, 2222, 380, 164, 30 }, { 718, 2522, 300, 164, 30 },
            { 718, 2788, 274, 164, 30 },
            { 960, 873, 890, 762, 30 }, { 960, 102, 896, 762, 30 },
            { 896, 220, 920, 358, 92 },
            { 934, 839, 134, 32, 562 }, { 934, 2927, 114, 32, 562 }
        };
        static const Plat kMarion[] = {
            { 968, 106, 668, 762, 30 }, { 968, 106, 132, 762, 30 },
            { 968, 2180, 1652, 762, 30 }, { 968, 2180, 2198, 762, 30 },
            { 972, 598, 562, 164, 30 }, { 972, 1215, 562, 164, 30 },
            { 972, 1722, 561, 164, 30 }, { 998, 2554, 563, 164, 30 },
            { 972, 2334, 2096, 164, 30 }, { 972, 2536, 1992, 164, 30 },
            { 972, 2758, 1904, 164, 30 }, { 972, 2466, 1828, 164, 30 }
        };
        struct PlatSet { const Plat* p; int n; };
        static const PlatSet kSets[7] = {
            { 0, 0 }, { kBB, 17 }, { kMangle, 17 }, { kTC, 20 },
            { kGF, 21 }, { kRWQ, 19 }, { kMarion, 12 }
        };
        const PlatSet& ps = kSets[mg.game];
        for (int i = 0; i < ps.n; ++i)
            Draw(ps.p[i].img, ox + ps.p[i].x, oy + ps.p[i].y,
                 ps.p[i].w, ps.p[i].h, 0xFFFFFFFF);
    }

    // the balloons (after the BB minigame's balloon is taken)
    if (game.HasBB() || mg.game == FNaF3Game::MG_BB) {
        static const f32 kBal[8][2] = {
            {0,1073},{275,1183},{591,1271},{795,1213},
            {985,1165},{1177,1111},{1399,1109},{1633,1109}
        };
        for (int i = 0; i < 8; ++i)
            Draw(854, ox + kBal[i][0], oy + kBal[i][1], 130.0f, 114.0f, 0xFFFFFFFF);
    }

    // the pickups/exits (kind-matched sprites; ~ positions from the kit)
    {
        struct Item { f32 x, y, w, h; int kind; };
        static const Item kIBB[] = {
            { 612, 425, 68, 114, 0 }, { 708, 427, 68, 114, 0 },
            { 354, 359, 68, 114, 0 }, { 154, 275, 68, 114, 0 },
            { 440, 197, 68, 114, 0 }, { 514, 195, 68, 114, 0 },
            { 752, 173, 68, 114, 0 },
            { 151, 544, 103, 124, 1 }, { 2506, 1697, 68, 114, 3 },
            { 2247, 992, 159, 168, 4 }
        };
        static const Item kIMangle[] = {
            { 636, 435, 74, 69, 0 }, { 2115, 334, 76, 85, 0 },
            { 1347, 420, 74, 51, 0 }, { 1734, 304, 71, 30, 0 },
            { 2675, 543, 103, 124, 1 }, { 2860, 2100, 168, 168, 2 },
            { 2960, 2160, 68, 114, 3 }
        };
        static const Item kITC[] = {
            { 700, 560, 36, 33, 0 }, { 1240, 470, 36, 33, 0 },
            { 1800, 220, 36, 33, 0 }, { 2300, 560, 36, 33, 0 },
            { 500, 560, 92, 92, 5 }, { 1400, 300, 92, 92, 5 },
            { 1900, 180, 92, 92, 5 }, { 2450, 500, 92, 92, 5 },
            { 2900, 560, 103, 124, 1 }, { 2450, 2160, 159, 168, 4 }
        };
        static const Item kIGF[] = {
            { 612, 425, 68, 114, 0 }, { 354, 359, 68, 114, 0 },
            { 440, 197, 68, 114, 0 }, { 1546, 470, 68, 114, 0 },
            { 2634, 210, 68, 114, 0 },
            { 151, 544, 103, 124, 1 }, { 2560, 1560, 68, 114, 3 },
            { 2247, 992, 159, 168, 4 }
        };
        static const Item kIRWQ[] = {
            { 468, 190, 68, 114, 0 }, { 1300, 1060, 68, 114, 0 },
            { 2740, 380, 68, 114, 0 },
            { 151, 544, 103, 124, 1 }, { 2960, 2160, 68, 114, 3 },
            { 2609, 396, 159, 168, 4 }
        };
        static const Item kIMarion[] = {
            { 468, 425, 68, 114, 0 }, { 908, 427, 68, 114, 0 },
            { 1258, 394, 159, 168, 0 }, { 1761, 394, 159, 168, 0 },
            { 151, 544, 103, 124, 1 }, { 2609, 396, 159, 168, 4 }
        };
        struct ItemSet { const Item* p; int n; };
        static const ItemSet kISets[7] = {
            { 0, 0 }, { kIBB, 10 }, { kIMangle, 7 }, { kITC, 10 },
            { kIGF, 8 }, { kIRWQ, 6 }, { kIMarion, 6 }
        };
        const ItemSet& is = kISets[mg.game];
        for (int i = 0; i < is.n; ++i) {
            if (mg.taken[i] && is.p[i].kind != 1) continue;
            int img = 730;
            if (is.p[i].kind == 1) img = 856;
            else if (is.p[i].kind == 2) img = 892;
            else if (is.p[i].kind == 3) img = (((int)(time * 5.0f) % 4) == 0) ? 776 :
                                             (((int)(time * 5.0f) % 4) == 1) ? 777 : 730;
            else if (is.p[i].kind == 4) img = 872;
            else if (is.p[i].kind == 5) img = 916;
            else if (is.p[i].kind == 0 && mg.game == FNaF3Game::MG_TOYCHICA) img = 923;
            Draw(img, ox + is.p[i].x, oy + is.p[i].y, is.p[i].w, is.p[i].h, 0xFFFFFFFF);
        }
    }

    // the Mangle kids that joined the walk trail behind the player
    if (mg.game == FNaF3Game::MG_MANGLE) {
        static const int kKid[4][2] = { {882, 883}, {884, 885}, {886, 887}, {888, 890} };
        static const f32 kOff[4][2] = { {-61,41}, {-21,17}, {25,29}, {47,27} };
        const int kF = mg.facing & 1;              // (in-bounds for the table)
        for (int i = 0; i < 4; ++i) {
            if (!mg.kidFollow[i]) continue;
            const f32 kx = mg.px + (mg.facing == 0 ? -kOff[i][0] : kOff[i][0]);
            Draw(kKid[i][kF], ox + kx - 56.0f, oy + mg.py + kOff[i][1] - 60.0f,
                 112.0f, 119.0f, 0xFFFFFFFF);
        }
    }

    // the player sprite per game (dump cells; right/left pairs)
    const int kF2 = mg.facing & 1;
    static const int kPlayer[7][2] = {
        { 713, 713 }, { 716, 717 }, { 875, 876 }, { 907, 908 },
        { 931, 932 }, { 944, 945 }, { 969, 970 }
    };
    const int pimg = kPlayer[mg.game][kF2];
    const f32 pw = (mg.game == FNaF3Game::MG_TOYCHICA || mg.game == FNaF3Game::MG_GFREDDY ||
                    mg.game == FNaF3Game::MG_RWQ || mg.game == FNaF3Game::MG_MARION) ? 112.0f : 112.0f;
    Draw(pimg, ox + mg.px - pw * 0.5f, oy + mg.py - 60.0f, pw, 119.0f, 0xFFFFFFFF);
    // the feeding scene: the kid eats (img 870) over the cake
    if (mg.feeding || mg.party > 0) {
        if (mg.game == FNaF3Game::MG_MARION) {
            // the float finale: the child (870/974/975) + the ghost kids
            static const int kCh[4] = { 0, 870, 974, 975 };
            const int st = mg.partyT > 300.0f ? 3 : (mg.partyT > 200.0f ? 2 :
                          (mg.partyT > 100.0f ? 1 : 0));
            Draw(kCh[st], ox + 2500.0f, oy + 340.0f, 122.0f, 127.0f, 0xFFFFFFFF);
            static const int kGh[4] = { 982, 989, 991, 993 };
            const f32 rise = mg.partyT * 0.15f;
            for (int i = 0; i < 4; ++i)
                Draw(kGh[i], ox + (2350.0f + i * 120.0f), oy + (420.0f - rise),
                     122.0f, 127.0f, 0xFFFFFFFF);
            if (mg.partyT > 300.0f) {
                // the balloons drift up (img 978-981)
                for (int i = 0; i < 4; ++i) {
                    const f32 cyc = mg.partyT * 0.2f + (f32)(i * 90);
                    const f32 by = 700.0f - (cyc - ((int)(cyc / 700.0f)) * 700.0f);
                    Draw(978 + (i & 3), ox + (600.0f + i * 240.0f), oy + by,
                         68.0f, 114.0f, 0xFFFFFFFF);
                }
            }
        } else {
            Draw(870, ox + mg.px - 47.0f, oy + mg.py - 49.0f, 94.0f, 98.0f, 0xFFFFFFFF);
            Draw(872, ox + mg.px - 80.0f, oy + mg.py - 84.0f, 159.0f, 168.0f, 0xFFFFFFFF);
        }
    }

    // the win overlay (img 775) + the CRT frame
    if (mg.won) Draw(775, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    // v2.67: no hint text in the original minigames (the BB/frame layouts
    // carry none) — the "A/UP: jump X: leave" debug line is gone.
}

// ---- the extras menu (frame 25) -----------------------------------------

void FNaF3Render::RenderExtras(const FNaF3Game& game, f32 time) {
    const FNaF3Game::ExtrasState& ex = game.Ex();
    if (!m_batch) return;

    // v2.67: frame 25 "extra" from the dump — the row plates are the dump's
    // OWN images (animatronics 357 / minigames 359 / jumpscares 360 /
    // cheats 1010 / exit 361) at their instance spots; the cursor is the
    // dump's ">>>" (img 833, like the title); the green row bands (img
    // 1043) and the static wash stay. The invented debug-font menu is gone.
    Draw(1017, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);     // the backdrop
    Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0x18FFFFFF);

    static const i32 kRowImg[5] = { 357, 359, 360, 1010, 361 };
    static const f32 kRowY[5]  = { 138.0f, 200.0f, 262.0f, 324.0f, 433.0f };
    // the dump's row-band strips (img 1043) at their frame spots
    static const f32 kBandY[5] = { 143.0f, 206.0f, 269.0f, 438.0f, 46.0f };
    static const f32 kBandX[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 460.0f };
    for (int i = 0; i < 5; ++i)
        DrawNat(1043, kBandX[i], kBandY[i], 0xFFFFFFFF);
    for (int i = 0; i < 5; ++i) {
        const bool locked = (i == 1 && !game.IsGoodEnd()) ||
                            (i == 2 && !game.IsBeat6()) ||
                            (i == 3 && !(game.IsGoodEnd() && game.IsBeat6()));
        u32 tint = 0xFFFFFFFF;
        if (locked) tint = 0xFF505050;
        DrawNat(kRowImg[i], 72.0f, kRowY[i], tint);
    }
    // cursor 833 (the dump's "cursor" object) at (34, rowY-4)
    DrawNat(833, 34.0f, kRowY[ex.row] - 4.0f, 0xFFFFFFFF);

    if (ex.row == 0) {
        // the animatronics viewer (anims 0,12-17 = imgs 393/395/438/1015/1014/828/1008);
        // the name rides the frame's own Text object spot ("name", (416,694))
        // — its strings are the dump's set-text actions (act #88).
        static const int kViewer[7] = { 393, 395, 438, 1015, 1014, 828, 1008 };
        static const char* kNames[7] = {
            "Springtrap", "Springtrap", "Phantom Foxy", "Phantom BB",
            "Phantom Chica", "Phantom Freddy", "Phantom Puppet"
        };
        Draw(kViewer[ex.viewer], 412.0f, 120.0f, 200.0f, 220.0f, 0xFFFFFFFF);
        if (m_text)
            m_text->DrawText((int)(416.0f * kScaleX), (int)(694.0f * kScaleY),
                             kNames[ex.viewer], 0xFFFFFFFF);
    } else if (ex.row == 1 && game.IsGoodEnd()) {
        // "play game" plate (img 436) at its dump spot; the pick name sits
        // on the same "name" object spot (no dump strings exist for these —
        // our names from the frame events, labeled).
        Draw(436, 541.0f, 468.0f, 218.0f, 43.0f, 0xFFFFFFFF);
        static const char* kMg[5] = {
            "BB's Air Adventure", "Mangle's Quest", "Chica's Party",
            "Stage01", "Glitch Minigame"
        };
        if (m_text)
            m_text->DrawText((int)(416.0f * kScaleX), (int)(694.0f * kScaleY),
                             kMg[ex.mgPick], 0xFFFFFFFF);
    } else if (ex.row == 2 && game.IsBeat6()) {
        // the jumpscare viewer (the six anims; play the pick as a hold)
        if (ex.jsT > 0.0f) {
            static const int kScare[6] = { 778, 792, 428, 184, 362, 226 };
            Draw(kScare[ex.jsPick], 112.0f, -40.0f, 800.0f, 840.0f, 0xFFFFFFFF);
        }
    } else if (ex.row == 3 && game.IsGoodEnd() && game.IsBeat6()) {
        // v2.67: the cheat names are the dump's OWN plates ("cheats 2..5" =
        // imgs 469/1022/1020/1026 at (469,190/257/324/391)); the ON badge is
        // the dump indicator cell (img 352) beside the enabled row.
        static const i32 kCheatImg[4] = { 469, 1022, 1020, 1026 };
        static const f32 kCheatY[4]  = { 190.0f, 257.0f, 324.0f, 391.0f };
        bool vals[4] = { game.CheatFast(), game.CheatVentProof(),
                         game.CheatHyper(), game.CheatNoCams() };
        for (int i = 0; i < 4; ++i) {
            DrawNat(kCheatImg[i], 469.0f, kCheatY[i],
                    (i == (ex.viewer % 4)) ? 0xFFFFFFFF : 0xFF909090);
            if (vals[i])
                DrawNat(352, 850.0f, kCheatY[i] + 4.0f, 0xFFFFFFFF);
        }
    }
}


} // namespace fnaf
