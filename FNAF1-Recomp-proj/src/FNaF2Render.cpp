/**
 * FNaF2Render.cpp: v2.31 — the FNAF2 renderer (see FNaF2Render.h).
 * All coordinates/handles are dump data: frame layouts, placement
 * tables (FrameLayout/frame_N_*.txt) and asset_mapping_fnaf2.hpp.
 */

#include "FNaF2Render.h"
#include "FNaF2Game.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"   // Snprintf — XDK CRT predates C99 snprintf
#include <cstdio>

namespace fnaf {

// frame 1024x768 -> 720p pillarbox: scale 0.9375, 160 px bars
static const f32 kScale = 720.0f / 768.0f;
static const f32 kOffX  = (1280.0f - 1024.0f * kScale) * 0.5f;   // 160

void FNaF2Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

// ---- private helpers --------------------------------------------------

void FNaF2Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, kOffX + fx * kScale, fy * kScale,
                  fw * kScale, fh * kScale, color);
}

void FNaF2Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = kOffX + (wx - pan) * kScale;
    if (sx >= 1280.0f || sx + fw * kScale <= kOffX) return;   // off-window
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, sx, wy * kScale, fw * kScale, fh * kScale, color);
}

int FNaF2Render::StaticFrame(f32 time) const {
    // obj "static" anim 0 = [332,334,328,329,330,331] @ speed 99 = 59.4 FPS
    static const int kFrames[6] = { 332, 334, 328, 329, 330, 331 };
    const f32 period = 100.0f / 99.0f / 60.0f;   // speed->fps = ×0.6
    return kFrames[(int)(time / period) % 6];
}

// ---- title (frame 1 "title", 1024x768) --------------------------------

void FNaF2Render::RenderTitle(const FNaF2Game& game, f32 time) {
    if (!m_batch || !m_pak) return;

    // background (obj "Active 2", img_321, opaque)
    Draw(321, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // static UNDER the logo/menu per the frame's instance order
    Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // the "Freddy glitch" (anims 12/13/14 = imgs 65/73/210).
    // DEVIATION: timed approximation (~7 s, one frame ~0.25 s) until the
    // dump's trigger group is pinned.
    {
        const f32 period = 7.0f;
        const f32 phase  = time - period * (f32)(int)(time / period);
        if (phase < 0.25f) {
            static const int kGlitch[3] = { 65, 73, 210 };
            Draw(kGlitch[(int)(time / period) % 3], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }

    // logo (obj "Active", img_469, hotspot-corrected (96,39))
    Draw(469, 96.0f, 39.0f, 300.0f, 280.0f, 0xFFFFFFFF);

    // menu (hotspot-corrected left/top from frame_1_title.txt)
    Draw(301,  86.0f, 437.0f, 222.0f,  31.0f, 0xFFFFFFFF);   // "new game"
    // "continue word" + "night word" + night digit appear only with a
    // save (dump coords: 303 (86,507), 270 (97,549), digit (185,567)) —
    // hidden while FNAF2 has no save system.
    // selector parks next to the selected item
    if (game.GetOptionSelected() == 0)
        Draw(229,  33.0f, 442.0f,  43.0f,  26.0f, 0xFFFFFFFF);
    else
        Draw(229,  33.0f, 655.0f,  43.0f,  26.0f, 0xFFFFFFFF);
    // 6th night (298) / stars (593) / demo (487) hidden at start
    // custom night (img_438, (89,650), VisibleAtStart=TRUE)
    Draw(438,  89.0f, 650.0f, 339.0f,  42.0f, 0xFFFFFFFF);

    // TEXT objects (raw instance coords are junk — placement per the
    // composite; debug font stopgap, consolas glyphs later)
    if (m_text) {
        m_text->DrawText((int)(160.0f +  90.0f * 0.9375f), (int)( 40.0f * 0.9375f), "Five",    0xFFFFFFFF);
        m_text->DrawText((int)(160.0f +  90.0f * 0.9375f), (int)( 88.0f * 0.9375f), "Nights",  0xFFFFFFFF);
        m_text->DrawText((int)(160.0f +  90.0f * 0.9375f), (int)(136.0f * 0.9375f), "at",      0xFFFFFFFF);
        m_text->DrawText((int)(160.0f +  90.0f * 0.9375f), (int)(184.0f * 0.9375f), "Freddy's",0xFFFFFFFF);
        m_text->DrawText((int)(160.0f +  90.0f * 0.9375f), (int)(232.0f * 0.9375f), "2",      0xFFFFFFFF);
        m_text->DrawText((int)(160.0f +  25.0f * 0.9375f), (int)(738.0f * 0.9375f), "v 1.033", 0xFFFFFFFF);
        m_text->DrawText((int)(160.0f + 335.0f * 0.9375f), (int)(737.0f * 0.9375f), "Press and hold delete to reset all data.", 0xFFFFFFFF);
        m_text->DrawText((int)(160.0f + 845.0f * 0.9375f), (int)(738.0f * 0.9375f), "(c)2014 Scott Cawthon", 0xFFFFFFFF);
    }
}

// ---- office (frame 3 "Frame 1", 1600x768, panning window) -------------

void FNaF2Render::RenderOffice(const FNaF2Game& game, f32 time, f32 pan) {
    if (!m_batch || !m_pak) return;

    // dark unless the flashlight is held (approx of the light groups):
    // world tinted to ~28%, strips 507 + LIGHT buttons always lit.
    const u32 worldCol = game.IsLit() ? 0xFFFFFFFF : 0xFF484848;

    // layer 0 — the room
    DrawWorld( 92,   0.0f,   0.0f, 1600.0f, 768.0f, pan, worldCol);  // bg
    DrawWorld(507,   0.0f,   0.0f, 1600.0f, 255.0f, pan, 0xFFFFFFFF);// "lights" strips
    DrawWorld(218, 637.0f,   0.0f,  899.0f, 389.0f, pan, worldCol);  // "Active 20" wall piece
    DrawWorld( 90, 105.0f, 361.0f,   92.0f, 141.0f, pan, 0xFFFFFFFF);// LEFT light button
    DrawWorld( 98, 1395.0f, 359.0f,  92.0f, 141.0f, pan, 0xFFFFFFFF);// RIGHT light button

    // layer 1 — the desk (plushie row, fan, mic)
    DrawWorld(293, 568.0f, 332.0f, 851.0f, 435.0f, pan, worldCol);   // table fan
    DrawWorld(601, 656.0f, 480.0f, 113.0f, 149.0f, pan, worldCol);   // freddy plush
    DrawWorld(604, 675.0f, 512.0f, 131.0f, 144.0f, pan, worldCol);   // chica
    DrawWorld(603, 754.0f, 472.0f, 116.0f, 173.0f, pan, worldCol);   // bonnie
    DrawWorld(606, 810.0f, 515.0f, 133.0f, 154.0f, pan, worldCol);   // foxy
    DrawWorld(612, 870.0f, 613.0f, 236.0f,  86.0f, pan, worldCol);   // mic
    DrawWorld(610, 991.0f, 517.0f, 122.0f, 147.0f, pan, worldCol);   // BB
    DrawWorld(608, 1038.0f, 476.0f, 117.0f, 172.0f, pan, worldCol);  // toy bonnie
    DrawWorld(555, 1122.0f, 509.0f,  84.0f, 147.0f, pan, worldCol);  // cupcake
    DrawWorld(611, 1218.0f, 522.0f, 129.0f, 156.0f, pan, worldCol);  // golden fred

    // layer 2 — viewport-space static over everything
    Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // ---- HUD (debug font for now): night, hour, battery % ----
    if (m_text) {
        char buf[40];
        Snprintf(buf, sizeof(buf), "Night %d", game.GetNight());
        m_text->DrawText(180, 20, buf, 0xFFFFFFFF);
        Snprintf(buf, sizeof(buf), "%d AM", game.GetHour());
        m_text->DrawText(600, 20, buf, 0xFFFFFFFF);
        const int pct = (int)((f32)game.GetBatteryLife() * 100.0f / 7000.0f + 0.5f);
        Snprintf(buf, sizeof(buf), "Battery %d%%", pct);
        m_text->DrawText(950, 20, buf, game.IsLit() ? 0xFF80FF80 : 0xFFB0B0B0);
    }
}

// ---- camera monitor (viewing 1..12) ------------------------------------
// Feed per viewing id pinned from the office map-button groups 100-123
// ("Active 16" set-anim): the EMPTY variants; animatronic frames come
// with the AI stage. 1600-wide feeds are squeezed into the window for
// now (TODO: sub-rect crop or scissor in the core batch).

void FNaF2Render::RenderMonitor(const FNaF2Game& game, f32 time, f32 sinceSwitch) {
    if (!m_batch || !m_pak) return;
    static const int kFeedImg[13] = {
        0, 174, 80, 83, 43, 38, 32, 51, 37, 117, 41, 76, 50
    };
    const i32 v = game.GetViewing();
    if (v < 1 || v > 12) return;

    char name[32];
    Snprintf(name, sizeof(name), "img_%d", kFeedImg[v]);
    PakLoadedTexture* t = m_pak->FindTexture(name);
    if (t && t->texture && m_batch) {
        const f32 w = (f32)t->origWidth * kScale;
        m_batch->Draw(t->texture, kOffX + (960.0f - w) * 0.5f, 0.0f,
                      w, 768.0f * kScale, 0xFFFFFFFF);
    }

    // feed-switch interference burst
    if (sinceSwitch < 0.12f)
        Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // ---- the MAP (layer 3, screen-fixed): panel img_2 (416x312) at its
    // instance (727,499) + 12 buttons img_16/img_17 (60x40) at their
    // instance positions; the selected cam shows the pressed look. The
    // numbers are our text for now (the dump buttons share one image —
    // per-cam labels are separate graphics, identify from a screenshot).
    Draw(2, 727.0f, 499.0f, 416.0f, 312.0f, 0xFFFFFFFF);
    {
        static const struct { int cam; float x, y; } kButtons[12] = {
            { 1,  602.0f, 557.0f }, { 2,  734.0f, 558.0f },
            { 3,  602.0f, 491.0f }, { 4,  737.0f, 491.0f },
            { 5,  608.0f, 651.0f }, { 6,  723.0f, 651.0f },
            { 7,  758.0f, 431.0f }, { 8,  603.0f, 420.0f },
            { 9,  915.0f, 390.0f }, { 10, 847.0f, 509.0f },
            { 11, 950.0f, 465.0f }, { 12, 933.0f, 558.0f },
        };
        for (int i = 0; i < 12; ++i) {
            Draw(kButtons[i].cam == v ? 17 : 16,
                 kButtons[i].x, kButtons[i].y, 60.0f, 40.0f, 0xFFFFFFFF);
        }
        if (m_text) {
            for (int i = 0; i < 12; ++i) {
                char cb[4];
                Snprintf(cb, sizeof(cb), "%02d", kButtons[i].cam);
                m_text->DrawText((int)(kOffX + (kButtons[i].x + 18.0f) * kScale),
                                 (int)((kButtons[i].y + 13.0f) * kScale), cb,
                                 kButtons[i].cam == v ? 0xFF202020 : 0xFF303030);
            }
        }
    }

    if (m_text) {
        char buf[24];
        Snprintf(buf, sizeof(buf), "CAM %02d", v);
        m_text->DrawText(180, 20, buf, 0xFFFFFFFF);
        for (int c = 1; c <= 12; ++c) {   // placeholder strip (real map later)
            char cb[4];
            Snprintf(cb, sizeof(cb), "%02d", c);
            m_text->DrawText(430 + c * 26, 690, cb, c == v ? 0xFF80FF80 : 0xFF909090);
        }
        Snprintf(buf, sizeof(buf), "%d AM", game.GetHour());
        m_text->DrawText(600, 20, buf, 0xFFFFFFFF);
        const int pct = (int)((f32)game.GetBatteryLife() * 100.0f / 7000.0f + 0.5f);
        Snprintf(buf, sizeof(buf), "Battery %d%%", pct);
        m_text->DrawText(950, 20, buf, 0xFFB0B0B0);
    }
}

} // namespace fnaf
