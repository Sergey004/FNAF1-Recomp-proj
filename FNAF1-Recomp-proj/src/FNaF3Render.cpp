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

void FNaF3Render::RenderTitle(f32 time, i32 optionSelected) {
    if (!m_batch || !m_pak) return;

    // The scene is ALWAYS visible (bg img_862 + menu); the static appears
    // only as short glitch bursts (the dump has a burst timer on the
    // static object, groups 72/73 — roll every 40 s, ticks down; here a
    // ~0.1 s burst per 1.6 s slot approximates the flicker).
    Draw(862, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(592,  97.0f, 428.0f, 215.0f,  49.0f, 0xFFFFFFFF);  // "new game"
    Draw(301,  97.0f, 500.0f, 243.0f,  49.0f, 0xFFFFFFFF);  // "load game"
    Draw(625,  97.0f, 572.0f, 257.0f,  49.0f, 0xFFFFFFFF);  // "nightmare"
    Draw(826,  96.0f, 641.0f, 145.0f,  49.0f, 0xFFFFFFFF);  // "extra"
    // the selector (img 833) rides the selected row (row pitch 72)
    {
        static const f32 kRowY[4] = { 429.0f, 501.0f, 573.0f, 642.0f };
        Draw(833,  38.0f, kRowY[optionSelected & 3],  34.0f,  49.0f, 0xFFFFFFFF);
    }

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

void FNaF3Render::RenderNightStart(i32 night) {
    // frame 2 "what day": the "Night N" art (img 1098) — debug-font digits
    // under it until the card anims land
    Draw(1098, 411.0f, 300.0f, 201.0f, 104.0f, 0xFFFFFFFF);
    if (m_text) {
        char buf[24];
        Snprintf(buf, sizeof(buf), "%d", night);
        m_text->DrawText((int)(620.0f * kScaleX), (int)(420.0f * kScaleY), buf, 0xFFFFFFFF);
    }
}

void FNaF3Render::RenderStatic6() {
    if (m_batch) Draw(StaticFrame(0.0f), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    if (m_text) m_text->DrawText((int)(590.0f * kScaleX), (int)(350.0f * kScaleY),
                                 "6 AM", 0xFFFFFFFF);
}

void FNaF3Render::RenderNextDay(i32 night) {
    if (m_text) {
        char buf[32];
        Snprintf(buf, sizeof(buf), "Night %d complete", night);
        m_text->DrawText((int)(500.0f * kScaleX), (int)(330.0f * kScaleY), buf, 0xFFFFFFFF);
        m_text->DrawText((int)(560.0f * kScaleX), (int)(380.0f * kScaleY),
                         "$4 earned", 0xFF909090);
    }
}

// ---- office (frame 3 "Frame 1", 200x768, pan window) ------------------

void FNaF3Render::RenderOffice(const FNaF3Game& game, f32 time) {
    if (!m_batch || !m_pak) return;
    const f32 pan = game.GetPan();

    // the office "foreground" states (dump objInfo 123: anim 0 = 203 normal,
    // anim 12 = 204 — the VENT-ERROR variant; groups 337/338/344 switch it
    // while the vent meter is <= -10, flickering under the hallucinations)
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

    // office static: short glitch bursts, more while hallucinating
    {
        const f32 kPeriod = 1.4f;
        const int slot = (int)(time / kPeriod);
        u32 h = (u32)slot * 2654435761u + 5u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const int duty = game.IsHallucinating() ? 30 : 10;
        if ((h % 100u) < (u32)duty)
            Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    }

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

    // HUD (debug-font stopgap)
    if (m_text) {
        char buf[40];
        const i32 ton = game.GetTimeOfNight();
        Snprintf(buf, sizeof(buf), "%d AM", ton == 0 ? 12 : ton);
        m_text->DrawText(40, 20, buf, 0xFFB0B0B0);
        Snprintf(buf, sizeof(buf), "Night %d", game.GetNight());
        m_text->DrawText(40, 44, buf, 0xFF707070);
    }
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

    // HUD (debug-font stopgap)
    if (m_text) {
        char buf[40];
        const i32 ton = game.GetTimeOfNight();
        Snprintf(buf, sizeof(buf), "%d AM   CAM %02d", ton == 0 ? 12 : ton, cam);
        m_text->DrawText(40, 20, buf, 0xFFB0B0B0);
        if (game.GetAudioMeter() <= -10 || game.GetCameraMeter() <= -10 ||
            game.GetVentMeter() <= -10)
            m_text->DrawText(40, 44, "SYSTEM ERROR", 0xFF6060FF);
    }
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
    if (m_text) {
        m_text->DrawText((int)(kMapX * kScaleX), (int)((kMapY + 405.0f) * kScaleY),
                         game.IsVentMap() ? "VENT MAP  (X: seal)" : "ROOM MAP  (X: lure)",
                         0xFF909090);
    }
}

void FNaF3Render::RenderPanel(const FNaF3Game& game) {
    // the maintenance screen (img 734) + the dump's text rows
    Draw(734, 262.0f, 80.0f, 756.0f, 611.0f, 0xFFFFFFFF);
    static const struct { i32 img; f32 y; i32 meter; } kRows[5] = {
        { 544, 180.0f, 0 },   // audio devices   (img 544)
        { 545, 270.0f, 0 },   // camera devices
        { 555, 360.0f, 0 },   // ventilation
        { 594, 450.0f, 0 },   // reboot all
        { 600, 520.0f, 0 }    // exit
    };
    for (i32 i = 0; i < 5; ++i) {
        u32 tint = 0xFFFFFFFF;
        if (i < 3) {
            const i32 m = (i == 0) ? game.GetAudioMeter()
                        : (i == 1) ? game.GetCameraMeter() : game.GetVentMeter();
            if (m <= -10) tint = 0xFF5050FF;               // the error rows
        }
        Draw(kRows[i].img, 400.0f, kRows[i].y, 286.0f, 39.0f, tint);
    }
    // the cursor (img 596) beside the selected row
    Draw(596, 330.0f, kRows[game.GetPanelCursor() - 1].y + 4.0f, 73.0f, 31.0f,
         0xFFFFFFFF);
    // the error icons (img 601) on the far side of the broken rows
    if (game.GetAudioMeter()  <= -10) Draw(601, 720.0f, 182.0f, 120.0f, 34.0f, 0xFFFFFFFF);
    if (game.GetCameraMeter() <= -10) Draw(601, 720.0f, 272.0f, 120.0f, 34.0f, 0xFFFFFFFF);
    if (game.GetVentMeter()   <= -10) Draw(601, 720.0f, 362.0f, 120.0f, 34.0f, 0xFFFFFFFF);
    // the reboot progress bar (img 611)
    if (game.GetRebooting() != 0) {
        const f32 w = 165.0f * (game.GetRebootProgress() / 10.0f);
        Draw(611, 560.0f, 560.0f, w, 50.0f, 0xFFFFFFFF);
    }
    if (m_text)
        m_text->DrawText((int)(330.0f * kScaleX), (int)(660.0f * kScaleY),
                         "UP/DOWN: select   A: reboot   Y: exit", 0xFF909090);
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
    // the chooser (frame 17) holds its backdrop img 29 while the sting plays
    if (which == 1)      Draw(346, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (which == 2) Draw(172, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (which == 3) Draw(123, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else                 Draw(29, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
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
    // frame 6: black + the game-over words (the frame's art rows were not
    // captured — the debug-font caption stands in, labeled)
    if (m_text) {
        m_text->DrawText((int)(430.0f * kScaleX), (int)(300.0f * kScaleY),
                         "G A M E   O V E R", 0xFFC0C0C0 & 0xFFFFFFFF);
        m_text->DrawText((int)(390.0f * kScaleX), (int)(420.0f * kScaleY),
                         "Press A to continue", 0xFF808080 & 0xFFFFFFFF);
    }
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
    if (m_text)
        m_text->DrawText((int)(470.0f * kScaleX), (int)(560.0f * kScaleY),
                         "LOADING", 0xFF909090 & 0xFFFFFFFF);
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
        for (int i = 0; i < 4; ++i) {
            if (!mg.kidFollow[i]) continue;
            const f32 kx = mg.px + (mg.facing == 0 ? -kOff[i][0] : kOff[i][0]);
            Draw(kKid[i][mg.facing], ox + kx - 56.0f, oy + mg.py + kOff[i][1] - 60.0f,
                 112.0f, 119.0f, 0xFFFFFFFF);
        }
    }

    // the player sprite per game (dump cells; right/left pairs)
    static const int kPlayer[7][2] = {
        { 713, 713 }, { 716, 717 }, { 875, 876 }, { 907, 908 },
        { 931, 932 }, { 944, 945 }, { 969, 970 }
    };
    const int pimg = kPlayer[mg.game][mg.facing];
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
    if (m_text)
        m_text->DrawText((int)(40.0f * kScaleX), (int)(700.0f * kScaleY),
                         "A/UP: jump   X: leave", 0xFF909090 & 0xFFFFFFFF);
}

// ---- the extras menu (frame 25) -----------------------------------------

void FNaF3Render::RenderExtras(const FNaF3Game& game, f32 time) {
    const FNaF3Game::ExtrasState& ex = game.Ex();
    if (!m_batch) return;

    // the static wash (alpha ~225 per dump g63) over black
    Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0x18FFFFFF);

    static const char* kRows[5] = {
        "ANIMATRONICS", "MINIGAMES", "JUMPSCARES", "CHEATS", "EXIT"
    };
    if (m_text) {
        for (int i = 0; i < 5; ++i) {
            const bool locked = (i == 1 && !game.IsGoodEnd()) ||
                                (i == 2 && !game.IsBeat6()) ||
                                (i == 3 && !(game.IsGoodEnd() && game.IsBeat6()));
            u32 col = (ex.row == i) ? 0xFFFFFFFF : 0xFF909090 & 0xFFFFFFFF;
            if (locked) col = 0xFF404040;
            m_text->DrawText((int)(420.0f * kScaleX),
                             (int)((220.0f + i * 60.0f) * kScaleY), kRows[i], col);
        }
        Draw(596, 384.0f, 222.0f + ex.row * 60.0f, 32.0f, 32.0f, 0xFFFFFFFF);
    }

    if (ex.row == 0) {
        // the animatronics viewer (anims 0,12-17 = imgs 393/395/438/1015/1014/828/1008)
        static const int kViewer[7] = { 393, 395, 438, 1015, 1014, 828, 1008 };
        static const char* kNames[7] = {
            "Springtrap", "Springtrap", "Phantom Foxy", "Phantom BB",
            "Phantom Chica", "Phantom Freddy", "Phantom Puppet"
        };
        Draw(kViewer[ex.viewer], 412.0f, 120.0f, 200.0f, 220.0f, 0xFFFFFFFF);
        if (m_text)
            m_text->DrawText((int)(440.0f * kScaleX), (int)(560.0f * kScaleY),
                             kNames[ex.viewer], 0xFFFFFFFF);
    } else if (ex.row == 1 && game.IsGoodEnd()) {
        static const char* kMg[5] = {
            "BB's Air Adventure", "Mangle's Quest", "Chica's Party",
            "Stage01", "Glitch Minigame"
        };
        Draw(436, 430.0f, 540.0f, 218.0f, 43.0f, 0xFFFFFFFF);   // "play game"
        if (m_text)
            m_text->DrawText((int)(400.0f * kScaleX), (int)(500.0f * kScaleY),
                             kMg[ex.mgPick], 0xFFFFFFFF);
    } else if (ex.row == 2 && game.IsBeat6()) {
        // the jumpscare viewer (the six anims; play the pick as a hold)
        if (ex.jsT > 0.0f) {
            static const int kScare[6] = { 778, 792, 428, 184, 362, 226 };
            Draw(kScare[ex.jsPick], 112.0f, -40.0f, 800.0f, 840.0f, 0xFFFFFFFF);
        } else if (m_text) {
            m_text->DrawText((int)(380.0f * kScaleX), (int)(520.0f * kScaleY),
                             "A: play the jumpscare", 0xFF909090 & 0xFFFFFFFF);
        }
    } else if (ex.row == 3 && game.IsGoodEnd() && game.IsBeat6()) {
        static const char* kCheat[4] = { "Fast nights", "Vent proof", "Hyper Springtrap", "No cameras" };
        static const bool* kVal[4];
        bool vals[4] = { game.CheatFast(), game.CheatVentProof(),
                         game.CheatHyper(), game.CheatNoCams() };
        (void)kVal;
        for (int i = 0; i < 4; ++i) {
            char buf[64];
            Snprintf(buf, sizeof(buf), "%s: %s", kCheat[i], vals[i] ? "ON" : "OFF");
            if (m_text)
                m_text->DrawText((int)(360.0f * kScaleX),
                                 (int)((440.0f + i * 40.0f) * kScaleY), buf,
                                 i == (ex.viewer % 4) ? 0xFFFFFFFF : (0xFF909090 & 0xFFFFFFFF));
        }
    }
}


} // namespace fnaf
