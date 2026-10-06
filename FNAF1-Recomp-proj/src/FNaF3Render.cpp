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

// ---- office (frame 3 "Frame 1", 2000x768, pan window) ------------------

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
        DrawWorld(204, 0.0f, 0.0f, 2000.0f, 768.0f, pan, (a << 24) | 0x00FFFFFFu);
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
        Draw(676, -11.0f, -3.0f, 1046.0f, 775.0f, (a << 24) | 0x00FFFFFFu);
    }

    // the phantom-scare white flash (the dump's thin "fscare" bar stretched)
    if (game.GetWhiteFlash() > 0.0f) {
        const u32 a = (u32)(255.0f * (game.GetWhiteFlash() > 1.0f ? 1.0f : game.GetWhiteFlash()));
        Draw(362, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
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
            // fit the piece inside a centered 400x330 box, native aspect
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
        Draw(39, 228.0f, 58.0f, 825.0f, 650.0f, (alpha << 24) | 0x00FFFFFFu);
    }

    // the map / vent map overlay
    if (!game.IsPanelOpen()) RenderMap(game);
    else                     RenderPanel(game);

    // the vent-error blackout also darkens the monitor
    if (game.GetBlackoutAlpha() > 0.5f) {
        const u32 a = (u32)game.GetBlackoutAlpha();
        Draw(676, -11.0f, -3.0f, 1046.0f, 775.0f, (a << 24) | 0x00FFFFFFu);
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
    // the night-5 chooser (frame 17) plays an anim whose cells are not
    // dumped — a black hold with the caption stand-in
    if (which == 1)      Draw(346, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (which == 2) Draw(172, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (which == 3) Draw(123, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else {
        if (m_batch) Draw(225, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        if (m_text) m_text->DrawText((int)(520.0f * kScaleX), (int)(340.0f * kScaleY),
                                     "THE END", 0xFFC0C0C0);
    }
}

} // namespace fnaf
