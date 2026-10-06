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
// frame 1024x768 -> FULL-STRETCH 1280x720 (PC-window behaviour):
// X scale 1.25, Y scale 0.9375, no bars
static const f32 kScaleX = 1280.0f / 1024.0f;   // 1.25
static const f32 kScaleY = 720.0f / 768.0f;     // 0.9375

void FNaF2Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

// ---- private helpers --------------------------------------------------

void FNaF2Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  fw * kScaleX, fh * kScaleY, color);
}

void FNaF2Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = (wx - pan) * kScaleX;
    if (sx >= 1280.0f || sx + fw * kScaleX <= 0.0f) return;   // off-window
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, sx, wy * kScaleY, fw * kScaleX, fh * kScaleY, color);
}

int FNaF2Render::StaticFrame(f32 time) const {
    // obj "static" anim 0 = [332,334,328,329,330,331] @ speed 99 = 59.4 FPS
    static const int kFrames[6] = { 332, 334, 328, 329, 330, 331 };
    const f32 period = 100.0f / 99.0f / 60.0f;   // speed->fps = ×0.6
    return kFrames[(int)(time / period) % 6];
}

// ---- disclaimer (frame 0 "Frame 17"): black + white mono warning ----
void FNaF2Render::RenderDisclaimer(const FNaF2Game& game) {
    if (!m_text) return;
    (void)game;
    m_text->DrawText((int)(455.0f * 1.25f), (int)(288.0f * kScaleY), "WARNING!", 0xFFFFFFFF);
    m_text->DrawText((int)(283.0f * 1.25f), (int)(345.0f * kScaleY),
                     "This game contains flashing lights, loud", 0xFFFFFFFF);
    m_text->DrawText((int)(341.0f * 1.25f), (int)(382.0f * kScaleY),
                     "noises, and lots of jumpscares!", 0xFFFFFFFF);
}

// ---- title (frame 1 "title", 1024x768) --------------------------------

void FNaF2Render::RenderTitle(const FNaF2Game& game, f32 time) {
    if (!m_batch || !m_pak) return;

    // ---- the background "video FROM Freddy's eyes" (dump groups 4+8-11):
    // at boot alterable[0] == 0 -> anim 12 = img_362 (the eye view IS the
    // first scene); every 2 s a Random(50) re-roll maps 0/1/2 to the eye
    // views 362/470/215 and anything else to the normal bg 321. We use a
    // stateless hash per 2 s slot (same 3/51 duty as Random(50)).
    int bgImg = 321;
    {
        const f32 kRollPeriod = 2.0f;
        const int slot = (int)(time / kRollPeriod);
        if (slot == 0) {
            bgImg = 362;                                // boot state: eye view
        } else {
            u32 h = (u32)slot * 2654435761u + 1u;
            h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
            const int roll = (int)(h % 51u);
            bgImg = (roll == 0) ? 362 : (roll == 1) ? 470 : (roll == 2) ? 215 : 321;
        }
    }
    // group 5: the bg's alpha coefficient := Random(250) every 6 s — the
    // FNAF2 "lamp" (the backdrop breathes dim-to-bright; coeff -> alpha =
    // (255-c)/255, the same convention as FNAF1's v2.57 title lamp).
    {
        u32 h = (u32)(int)(time / 6.0f) * 2654435761u + 5u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const u32 lampAlpha = 255u - (u32)(h % 250u);
        Draw(bgImg, 0.0f, 0.0f, 1024.0f, 768.0f, (lampAlpha << 24) | 0x00FFFFFFu);
    }

    // ---- the "static" obj (dump group 1): alpha coefficient := 50+Random(100)
    // every 1.8 s -> alpha 0.42..0.80 (the old draw was OPAQUE — the bg
    // drowned); position (0,0), fullscreen 1024x768, no X jitter (the dump
    // sets only the alpha). Point-sampled so the grain survives the
    // 1024->1280 stretch (FNAF1's v2.57 overlay). REMOVED DEVIATION: the
    // invented "glitch" (imgs 65/73/210 every ~7 s) — those images are the
    // PUPPET (full body / head) and the title dump never fires the static
    // object's anims 12-15 (they are office cam events); that was the
    // "Puppet appearing out of nowhere" on the title.
    {
        u32 h = (u32)(int)(time / 1.8f) * 2654435761u + 7u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const u32 sAlpha = 255u - (50u + h % 100u);
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", StaticFrame(time));
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (t && t->texture && m_batch) {
            const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
            const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
            m_batch->DrawPoint(t->texture, 0.0f, 0.0f, 1280.0f, 720.0f,
                               0.0f, 0.0f, u1, v1, (sAlpha << 24) | 0x00FFFFFFu);
        }
    }

    // ---- "blip flash 2" (img_68, fullscreen band flash): group 5 rolls
    // alterable[0] := Random(3) every 6 s and groups 6/7 show it only on 1;
    // group 4 re-rolls its alpha := 200+Random(50) every 2 s (very faint,
    // 0.02..0.22). Was missing entirely.
    {
        u32 hw = (u32)(int)(time / 6.0f) * 2654435761u + 11u;
        hw ^= hw >> 13;  hw *= 3266489917u;  hw ^= hw >> 16;
        if (hw % 3u == 1u) {
            u32 h = (u32)(int)(time / 2.0f) * 2654435761u + 13u;
            h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
            const u32 bAlpha = 255u - (200u + h % 50u);
            Draw(68, 0.0f, 0.0f, 1024.0f, 768.0f, (bAlpha << 24) | 0x00FFFFFFu);
        }
    }

    // logo (obj "Active", img_469, hotspot-corrected (96,39))
    Draw(469, 96.0f, 39.0f, 300.0f, 280.0f, 0xFFFFFFFF);

    // menu rows — dump layout positions (hotspot-corrected left/top):
    // new game 301 (86,437) 222x31 / continue 303 (86,507) 222x34 /
    // 6th night 298 (90,582) 252x42 / custom 438 (89,650) 339x42.
    // Continue is ALWAYS visible (dump groups 45-48 gate only the 6th
    // night+star1 by beatgame and custom+star2 by beat6). Star 3 (246,341)
    // needs the all-20 flag — not tracked yet, skipped.
    Draw(301,  86.0f, 437.0f, 222.0f,  31.0f, 0xFFFFFFFF);   // "new game"
    Draw(303,  86.0f, 507.0f, 222.0f,  34.0f, 0xFFFFFFFF);   // "continue"
    if (game.IsBeat5()) Draw(298,  90.0f, 582.0f, 252.0f,  42.0f, 0xFFFFFFFF);   // "6th night"
    if (game.IsBeat6()) Draw(438,  89.0f, 650.0f, 339.0f,  42.0f, 0xFFFFFFFF);   // "custom night"
    if (game.IsBeat5()) Draw(593,  94.0f, 341.0f,  57.0f,  55.0f, 0xFFFFFFFF);   // star 1
    if (game.IsBeat6()) Draw(593, 171.0f, 341.0f,  57.0f,  55.0f, 0xFFFFFFFF);   // star 2
    // the selector (dump "Active 4", img 229 at (33,512) with continue at
    // 507) rides the selected row (+5 px onto each row's y)
    {
        static const f32 kRowY[4] = { 442.0f, 512.0f, 587.0f, 655.0f };
        const i32 sel = game.GetOptionSelected();
        Draw(229,  33.0f, kRowY[sel],  43.0f,  26.0f, 0xFFFFFFFF);
    }

    // "Night N" under the Continue row (dump groups 51/52: night word 270 +
    // the night number counter at (185,567) show only while Continue is
    // selected; the counter value is the Ini level capped at 5, g58)
    if (game.GetOptionSelected() == 1) {
        Draw(270,  97.0f, 549.0f,  63.0f,  22.0f, 0xFFFFFFFF);   // "Night" art
        if (m_text) {
            char num[8];
            const i32 lv = game.GetLastNight();
            Snprintf(num, sizeof(num), "%d", lv < 1 ? 1 : (lv > 5 ? 5 : lv));
            m_text->DrawText((int)(185.0f * kScaleX), (int)(567.0f * kScaleY),
                             num, 0xFFFFFFFF);
        }
    }

    // footer — img_294 "v 1.033" (26,738) and img_631 "Press and hold delete
    // to reset all data." (335,736) are ART (verified the PNGs); only
    // "(c)2014 Scott Cawthon" is a Text object (debug font stopgap, placed
    // so it fits inside 1280). The "Demo" tag (img_487) is hidden in the
    // full game (DEMO? == 0) — never drawn.
    Draw(294,  26.0f, 738.0f,  69.0f,  12.0f, 0xFFFFFFFF);
    Draw(631, 335.0f, 736.0f, 396.0f,  13.0f, 0xFFFFFFFF);
    if (m_text)
        m_text->DrawText((int)(790.0f * kScaleX), (int)(738.0f * kScaleY),
                         "(c)2014 Scott Cawthon", 0xFFFFFFFF);
}

// ---- office (frame 3 "Frame 1", 1600x768, panning window) -------------

void FNaF2Render::RenderOffice(const FNaF2Game& game, f32 time, f32 pan, i32 sceneValue) {
    if (!m_batch || !m_pak) return;

    // v2.59: the scene selector drives the office view — dark office (35)
    // keeps the panned world; any hall/lit view (36/73/93/97/84/76/50/55/56/
    // 58/99) REPLACES the room image with its own full-frame art (the mask,
    // static, sprites and HUD still draw on top).
    bool hallView = false;
    if (sceneValue != 35 && sceneValue != 0) {
        const i32 img = FNaF2Game::SceneValueImg(sceneValue);
        if (img > 0) {
            char name[32];
            Snprintf(name, sizeof(name), "img_%d", img);
            PakLoadedTexture* t = m_pak->FindTexture(name);
            if (t && t->texture) {
                const f32 w = (t->origWidth  > 0) ? (f32)t->origWidth  : 1024.0f;
                const f32 h = (t->origHeight > 0) ? (f32)t->origHeight : 768.0f;
                m_batch->Draw(t->texture, 0.0f, 0.0f, w * kScaleX, h * kScaleY, 0xFFFFFFFF);
            }
            hallView = true;
        }
    }

    // dark unless the flashlight is held (approx of the light groups):
    // world tinted to ~28%, strips 507 + LIGHT buttons always lit.
    const u32 worldCol = game.IsLit() ? 0xFFFFFFFF : 0xFF484848;
    if (!hallView) {

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
    }   // end of the panned room (skipped during hall views)

    // layer 2 — v2.59: office sprites from the encounter pipeline
    if (game.GetFreddyOfficeView())
        DrawWorld(512, 620.0f, 320.0f, 480.0f, 300.0f, pan, 0xFFFFFFFF);  // Freddy under the table (pos approx)
    if (game.IsToyBonnieScare())
        DrawWorld(187, 620.0f, 260.0f, 520.0f, 340.0f, pan, 0xFFFFFFFF);  // toy Bonnie office pose
    if (game.HasBBInOffice())
        DrawWorld(221, 700.0f, 300.0f, 380.0f, 320.0f, pan, 0xFFFFFFFF);  // BB at the desk
    // danger darkening ("blackout" img_225): alpha ramps over the 300-frame window
    {
        const f32 df = game.GetDangerDark();
        if (df > 0.0f) {
            f32 a = df / 300.0f; if (a > 0.85f) a = 0.85f;
            const u32 A = (u32)(a * 255.0f);
            DrawWorld(225, 0.0f, 0.0f, 1600.0f, 768.0f, pan, (A << 24) | 0x00FFFFFFu);
        }
    }

    // layer 3 — viewport-space static over everything
    Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // v2.59: the mask overlay (dump "mask come down"/"mask"/"mask up")
    {
        const i32 ms = game.GetMaskState();
        static const int kMaskDown[9] = { 140, 139, 138, 137, 136, 135, 134, 143, 88 };
        static const int kMaskUp[11]  = { 88, 143, 134, 135, 136, 137, 138, 139, 140, 133, 133 };
        if (ms == 1) {
            int fi = (int)(game.GetMaskT() * 45.0f); if (fi > 8) fi = 8;
            Draw(kMaskDown[fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        } else if (ms == 2) {
            Draw(125, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        } else if (ms == 3) {
            int fi = (int)(game.GetMaskT() * 45.0f); if (fi > 10) fi = 10;
            Draw(kMaskUp[fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }

    // ---- HUD (debug font for now): night, hour, battery % ----
    if (m_text) {
        char buf[40];
        Snprintf(buf, sizeof(buf), "Night %d", game.GetNight());
        m_text->DrawText(180, 20, buf, 0xFFFFFFFF);
        Snprintf(buf, sizeof(buf), "%d AM", game.GetHour());
        m_text->DrawText(600, 20, buf, 0xFFFFFFFF);
        const int pct = (int)((f32)game.GetBatteryLife() * 100.0f / (f32)game.GetBatteryMax() + 0.5f);
        Snprintf(buf, sizeof(buf), "Battery %d%%", pct);
        m_text->DrawText(950, 20, buf, game.IsLit() ? 0xFF80FF80 : 0xFFB0B0B0);
    }

    // v2.60: the office-side danger face ("danger 1", img_494/495, angry 496/497)
    if (game.IsMusicBoxDanger()) {
        const bool angry = game.GetMusicGauge() <= 200.0f;
        Draw(angry ? 497 : 495, 40.0f, 120.0f, 220.0f, 220.0f, 0xFFFFFFFF);
    }
}

// ---- camera monitor (viewing 1..12) ------------------------------------
// v2.59: the feed image comes from the game's scene selector (the dump's
// "Active 16" value = viewing + lit? + presence); value 0 ("no matching
// view") KEEPS the previous feed image, exactly like the dump's unmatched
// states. Fallback = the pinned empty feed. Wide (1600px) feeds pan with
// the office pan, 1024-wide ones pin.

void FNaF2Render::RenderMonitor(const FNaF2Game& game, f32 time, f32 sinceSwitch, f32 pan,
                                i32 sceneValue, i32 lastSceneValue) {
    if (!m_batch || !m_pak) return;
    static const int kFeedImg[13] = {
        0, 174, 80, 83, 43, 38, 32, 51, 37, 117, 41, 76, 50
    };
    const i32 v = game.GetViewing();
    if (v < 1 || v > 12) return;

    i32 value = (sceneValue != 0) ? sceneValue : lastSceneValue;
    i32 img = (value > 0) ? FNaF2Game::SceneValueImg(value) : 0;
    if (img <= 0) img = kFeedImg[v];   // fallback: the pinned empty feed
    if (img > 0) {
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", img);
        PakLoadedTexture* t = m_pak->FindTexture(name);
        if (t && t->texture) {
            if (t->origWidth > 1024)
                DrawWorld(img, 0.0f, 0.0f, (f32)t->origWidth, 768.0f, pan, 0xFFFFFFFF);
            else
                Draw(img, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
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
                m_text->DrawText((int)((kButtons[i].x + 18.0f) * kScaleX),
                                 (int)((kButtons[i].y + 13.0f) * kScaleY), cb,
                                 kButtons[i].cam == v ? 0xFF202020 : 0xFF303030);
            }
        }
    }

    // CAM 11 (Prize Corner): the MUSIC BOX — wind button img_251/273, the
    // gauge (0..2000) and the danger faces (dump "danger 2": img_307/308,
    // angry 489/490) once the Puppet has left the box.
    if (v == 11) {
        Draw(251, 90.0f, 560.0f, 156.0f, 65.0f, 0xFFFFFFFF);
        const int gpct = (int)(game.GetMusicGauge() * 100.0f / 2000.0f + 0.5f);
        if (m_text) {
            m_text->DrawText((int)(20.0f), (int)(640.0f),
                             "Hold X to wind the music box", 0xFFC0C0C0);
            char gb[32];
            Snprintf(gb, sizeof(gb), "Music box %d%%", gpct);
            m_text->DrawText((int)(20.0f), (int)(665.0f), gb,
                             gpct <= 20 ? 0xFFFF6060 : 0xFFC0C0C0);
        }
        if (game.IsMusicBoxDanger()) {
            const bool angry = game.GetMusicGauge() <= 200.0f;
            Draw(angry ? 490 : 308, 700.0f, 120.0f, 220.0f, 220.0f, 0xFFFFFFFF);
        }
    }

    // v2.59: movement static burst — a character moved in the watched feed
    if (game.GetMoveStatic() > 0.0f)
        Draw(20, 0.0f, 0.0f, 1024.0f, 768.0f,
             (u32)((int)(game.GetMoveStatic() * 255.0f) << 24) | 0x00FFFFFFu);

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
        const int pct = (int)((f32)game.GetBatteryLife() * 100.0f / (f32)game.GetBatteryMax() + 0.5f);
        Snprintf(buf, sizeof(buf), "Battery %d%%", pct);
        m_text->DrawText(950, 20, buf, 0xFFB0B0B0);
    }
}
// v2.59: the jumpscare — "attack animation" values 12..21 (anim per attacker,
// dump g440-449) drawn full-screen over whatever is underneath. Frame lists
// verbatim from the FNAF2 application.json (h=156).
void FNaF2Render::DrawAttack(const FNaF2Game& game) {
    if (!m_batch) return;
    static const int kFrames[10][16] = {
        { 377,365,379,367,368,369,370,371,372,373,374,375,376,0,0,0 },
        { 744,729,746,731,732,733,734,735,736,737,738,739,740,741,742,743 },
        { 458,447,460,449,450,451,452,453,454,455,456,457,0,0,0,0 },
        { 401,385,386,388,389,390,391,392,393,394,395,396,397,398,0,0 },
        { 712,700,714,702,703,704,705,706,707,708,709,710,711,0,0,0 },
        { 757,745,759,747,748,749,750,751,752,753,754,755,756,0,0,0 },
        { 446,414,448,415,416,419,420,421,422,423,424,445,0,0,0,0 },
        { 812,387,814,404,785,792,802,803,804,805,806,807,808,809,810,811 },
        { 515,195,517,313,315,316,317,318,324,340,342,343,345,347,514,0 },
        { 320,246,516,247,248,249,250,306,310,311,312,314,319,0,0,0 },
    };
    static const i32 kCount[10] = { 13, 16, 12, 14, 13, 13, 12, 16, 15, 13 };
    static const i32 kSpd[10]   = { 50, 50, 40, 60, 50, 40, 40, 50, 50, 50 };
    const i32 anim = game.GetScareAnim();
    if (anim < 12 || anim > 21) return;
    const i32 idx = anim - 12;
    const f32 fps = (f32)kSpd[idx] * 0.6f;
    int fi = (int)(game.GetScareTimer() * fps);
    if (fi < 0) fi = 0;
    if (fi >= kCount[idx]) fi = kCount[idx] - 1;
    Draw(kFrames[idx][fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

// ===========================================================================
// v2.62 — the rest of the dump's frame flow + the 8-bit minigames.
// Coordinates from the frame layouts; the anim-cell art that the dumper
// could not split into separate images (night names, the 5->6 roll, walk
// cycles) stands in as labeled debug-font/approximation stops.
// ===========================================================================

void FNaF2Render::RenderAd() {
    // frame 8: the HELP WANTED newspaper, fullscreen
    Draw(272, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderCard(const FNaF2Game& game) {
    // frame 2: the "12:00 AM / Nst Night" card (429 @ 512,374) — the
    // per-night cells ARE in application.json (objInfo 45 anims 0/12-17):
    // night 1->429, 2->430, 3->431, 4->436, 5->437, 6->426, 7->425
    static const i32 kNightCell[7] = { 429, 430, 431, 436, 437, 426, 425 };
    const i32 n = game.GetNight();
    const i32 cell = kNightCell[(n < 1) ? 0 : (n > 7 ? 6 : n - 1)];
    Draw(cell, 394.0f, 316.0f, 235.0f, 116.0f, 0xFFFFFFFF);
    if (m_text)
        m_text->DrawText((int)(480.0f * kScaleX), (int)(200.0f * kScaleY), "12:00 AM", 0xFFB0B0B0);
}

void FNaF2Render::RenderStatic() {
    // frame 4: the post-night static (img 361 fullscreen)
    Draw(361, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderNextDay(const FNaF2Game& game) {
    // frame 5: the "5 -> 6" roll IS in application.json (objInfo 224 anim 0,
    // 18 cells @ speed 10 = 6 fps = 3 s), ending on the "6"; the AM plate
    // (295 @ 595,363) sits beside it. The crowd cheer rides the anim end.
    static const i32 kRoll[18] = { 297, 297, 297, 290, 300, 302, 304, 309, 309,
                                   309, 309, 309, 433, 336, 335, 427, 364, 432 };
    Draw(295, 595.0f, 363.0f, 179.0f, 84.0f, 0xFFFFFFFF);
    int fi = (int)(game.GetCardTimer() * 6.0f);          // speed 10 -> 6 fps
    if (fi < 0) fi = 0;
    if (fi > 17) fi = 17;                                // hold the last cell
    Draw(kRoll[fi], 415.0f, 363.0f, 57.0f, 86.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderDream(const FNaF2Game& game) {
    // frame 13: the 2500x768 panning room (616), the slumped pair
    // (609 Bonnie @243 / 614 Chica @2348), the night props, the static
    // flicker and the slow blackout
    const f32 pan = game.GetDreamPan();
    DrawWorld(616, 0.0f, 0.0f, 2500.0f, 768.0f, pan, 0xFFFFFFFF);
    DrawWorld(609, 243.0f, 300.0f, 400.0f, 460.0f, pan, 0xFFFFFFFF);   // Bonnie
    DrawWorld(614, 2348.0f, 300.0f, 400.0f, 460.0f, pan, 0xFFFFFFFF);  // Chica + balloons
    if (game.GetNight() == 4)
        DrawWorld(624, 1514.0f, 340.0f, 220.0f, 300.0f, pan, 0xFFFFFFFF);  // golden Freddy
    if (game.GetNight() == 5) {
        // the Puppet slides toward the anchor (dump groups 26/27)
        const f32 px = 2170.0f - pan;
        if (px > -300.0f && px < 1330.0f)
            DrawWorld(626, 2170.0f, 330.0f, 260.0f, 420.0f, pan, 0xFFFFFFFF);
    }
    // the static flicker (alpha tiers per the re-roll)
    {
        const i32 roll = game.GetRareRoll();
        const u32 a = (u32)(roll <= 6 ? 250 : (roll <= 9 ? 225 : 100));
        Draw(361, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }
    // the blackout fade
    if (game.GetBlackout() > 0.5f) {
        const u32 a = (u32)game.GetBlackout();
        Draw(225, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }
}

void FNaF2Render::RenderError(bool second) {
    Draw(second ? 630 : 628, second ? 33.0f : 20.0f, second ? 19.0f : 16.0f,
         512.0f, 384.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderEnd(i32 which) {
    const i32 img = (which == 5) ? 587 : (which == 6) ? 590 : 622;
    Draw(img, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderCustomize(const FNaF2Game& game) {
    // frame 12: the header, the ten AI rows (icon + arrows + number), the
    // challenge mode, READY; the icons are the dumped row images
    static const i32 kIcons[10] = { 551, 339, 338, 341, 344, 352, 346, 576, 577, 578 };
    Draw(524, 262.0f, 20.0f, 500.0f, 90.0f, 0xFFFFFFFF);    // "Customize Night"
    for (i32 i = 0; i < 10; ++i) {
        const f32 y = 130.0f + 46.0f * i;
        const u32 tint = (game.GetOptionSelected() == i) ? 0xFF50A0FF : 0xFFFFFFFF;
        Draw(kIcons[i], 320.0f, y, 44.0f, 44.0f, tint);
        Draw(557, 420.0f, y + 4.0f, 34.0f, 34.0f, 0xFFFFFFFF);   // left arrow
        Draw(556, 700.0f, y + 4.0f, 34.0f, 34.0f, 0xFFFFFFFF);   // right arrow
        if (m_text) {
            char buf[8];
            Snprintf(buf, sizeof(buf), "%d", game.GetCustomAI(i));
            m_text->DrawText((int)(600.0f * kScaleX), (int)((y + 6.0f) * kScaleY), buf, tint);
        }
    }
    // the challenge mode row (the dumped arrows 595/602)
    Draw(602, 320.0f, 620.0f, 34.0f, 34.0f, 0xFFFFFFFF);
    Draw(595, 700.0f, 620.0f, 34.0f, 34.0f, 0xFFFFFFFF);
    if (m_text) {
        char buf[64];
        static const char* const kModes[10] = {
            "20/20/20/20", "New and Shiny", "Double Trouble", "Night of Misfits",
            "Foxy Foxy", "Ladies Night", "Freddy's Circus", "Cupcake Challenge",
            "Fazbear Fever", "Golden Freddy"
        };
        const i32 mode = game.GetCustomMode();
        if (mode >= 1 && mode <= 10) {
            Snprintf(buf, sizeof(buf), "%s%s", kModes[mode - 1],
                     game.GetDoingCustom() ? "" : " (not armed)");
            m_text->DrawText((int)(380.0f * kScaleX), (int)(628.0f * kScaleY), buf, 0xFFB0B0B0);
        }
        if (game.GetAllAre20())
            m_text->DrawText((int)(460.0f * kScaleX), (int)(90.0f * kScaleY), "ALL 20", 0xFF60FF60);
        if (game.Is1987())
            m_text->DrawText((int)(560.0f * kScaleX), (int)(120.0f * kScaleY), "1987", 0xFF6060FF);
    }
    Draw(546, 818.0f, 671.0f, 150.0f, 56.0f, 0xFFFFFFFF);    // READY
    // the did-challenge stars (dump groups 78/79: the INI c<mode> check)
    const i32 mode = game.GetCustomMode();
    if (mode >= 1 && mode <= 10 && game.GetDoingCustom() == 0 && m_text) {
        // the beat flags ride the game (c-flags); the star shows for the
        // beaten challenge of the CURRENT mode slot
        m_text->DrawText((int)(560.0f * kScaleX), (int)(580.0f * kScaleY),
                         "* *", 0xFFC0C040);
    }
}

void FNaF2Render::RenderRare(i32 which) {
    const i32 img = (which == 1) ? 625 : (which == 2) ? 623 : 627;
    Draw(img, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderGameOver() {
    // frame 6: the withered Freddy face + the "Game Over" plate
    Draw(226, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(228, 180.0f, 700.0f, 320.0f, 48.0f, 0xFFFFFFFF);
}

// ---- the 8-bit minigames ------------------------------------------------

void FNaF2Render::RenderEightBit(const FNaF2Game& game) {
    // frame 19 SAVETHEM: the fixed-screen room re-dressed per grid cell
    const FNaF2MgState& mg = game.Mg();
    Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    // the border walls (dump: 639/640 strips on all four edges)
    Draw(639,  -2.0f,  -8.0f, 1024.0f, 84.0f, 0xFFFFFFFF);
    Draw(639,  -2.0f, 680.0f, 1024.0f, 84.0f, 0xFFFFFFFF);
    Draw(640,  -2.0f,  80.0f, 30.0f, 570.0f, 0xFFFFFFFF);
    Draw(640, 926.0f,  79.0f, 30.0f, 570.0f, 0xFFFFFFFF);
    // the per-room dressing (the dump's fire-once groups, keyed (h,v))
    const i32 h = mg.h, v = mg.v;
    if ((h == 3 && v == 1) || (h == 4 && v == 1) || (h == 4 && v == 3) ||
        (h == 3 && v == 3) || (h == 2 && v == 4)) {
        Draw(652, 294.0f, 190.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(652, 737.0f, 190.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(652, 737.0f, 410.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(652, 294.0f, 410.0f, 90.0f, 60.0f, 0xFFFFFFFF);
    }
    if (h == 5 && v == 2) Draw(655, 720.0f, 420.0f, 60.0f, 70.0f, 0xFFFFFFFF);
    if (h == 1 && v == 4) Draw(660, 523.0f, 296.0f, 190.0f, 100.0f, 0xFFFFFFFF);
    if (h == 2 && v == 1) {
        Draw(656, 184.0f, 524.0f, 70.0f, 70.0f, 0xFFFFFFFF);   // dead chica
        Draw(657, 280.0f, 288.0f, 70.0f, 70.0f, 0xFFFFFFFF);   // dead bonnie
        Draw(658, 832.0f, 190.0f, 70.0f, 70.0f, 0xFFFFFFFF);
    }
    if ((h == 2 && v == 3) || (h == 4 && v == 2) || (h == 2 && v == 4))
        Draw(659, 500.0f, 600.0f, 50.0f, 50.0f, 0xFFFFFFFF);   // trash
    if ((h == 3 && v == 2) || (h == 4 && v == 2) || (h == 2 && v == 4) || (h == 3 && v == 4))
        Draw(662, 700.0f, 250.0f, 60.0f, 60.0f, 0xFFFFFFFF);   // checker
    if (h == 3 && v == 5) Draw(664, 704.0f, 400.0f, 44.0f, 50.0f, 0xFFFFFFFF);  // gift
    if (h == 4 && v == 2) Draw(665, (f32)mg.heX, (f32)mg.heY, 60.0f, 80.0f, 0xFFFFFFFF);
    // the blood spots (group 40 shows them every room after the first change)
    Draw(380, 316.0f, 308.0f, 30.0f, 30.0f, 0xFFFFFFFF);
    Draw(380, 711.0f, 468.0f, 30.0f, 30.0f, 0xFFFFFFFF);
    // the NPCs
    if (mg.youCantOn) Draw(673, (f32)mg.youCantX, (f32)mg.youCantY, 40.0f, 56.0f, 0xFFFFFFFF);
    if (mg.manOn)     Draw(671, (f32)mg.manX, (f32)mg.manY, 46.0f, 70.0f, 0xFFFFFFFF);
    if (mg.gfOn)      Draw(669, (f32)mg.gfX, (f32)mg.gfY, 70.0f, 90.0f, 0xFFFFFFFF);
    if (mg.chaserOn)  Draw(675, (f32)mg.chX, (f32)mg.chY, 56.0f, 80.0f, 0xFFFFFFFF);
    // the player (8-bit Freddy 245 glued to the hit box)
    Draw(245, (f32)mg.px - 50.0f, (f32)mg.py - 55.0f, 99.0f, 100.0f, 0xFFFFFFFF);
    // the WASD hint rides above the player for the first 5 s (group 104)
    if (mg.t < 5.0f) Draw(646, (f32)mg.px - 40.0f, (f32)mg.py - 130.0f, 84.0f, 30.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderMgLoad() {
    // frame 21: black + the two loading bars
    Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(678, 0.0f, 300.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
    Draw(680, 0.0f, 420.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderMinigame(const FNaF2Game& game) {
    const FNaF2MgState& mg = game.Mg();
    const bool attack = (mg.attackT >= 0.0f);
    // the walk cycles (application.json, the player objects' anim 0):
    // mg1 [412,417]@speed5, mg2 [760], mg3 [722,726] — 3 fps waddles
    int walkFrame = 0;
    if (mg.game == 2)      walkFrame = ((int)(mg.t * 3.0f)) % 2;   // 412/417
    else if (mg.game == 4) walkFrame = ((int)(mg.t * 3.0f)) % 2;   // 722/726
    if (mg.game == 2) {
        // frame 23 TAKE CAKE
        Draw(400, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(698, 77.0f, 659.0f, 870.0f, 60.0f, 0xFFFFFFFF);
        // the kids (application.json objInfo 393): 0=[692,685]@3fps content,
        // 12=[519,482]@6fps crying, 13=[693,696]@18fps past-20 — the real
        // sadness cells, picked per the kid's counter
        static const f32 kKidX[6] = { 290,290,290,776,770,766 };
        static const f32 kKidY[6] = { 280,416,548,278,410,538 };
        for (i32 i = 0; i < 6; ++i) {
            i32 img = 692; f32 fps = 3.0f;
            if (mg.kidSad[i] >= 20)      { img = (int)(mg.t * 18.0f) % 2 ? 696 : 693; fps = 18.0f; }
            else if (mg.kidSad[i] >= 10) { img = (int)(mg.t * 6.0f)  % 2 ? 482 : 519; }
            else                         { img = (int)(mg.t * 3.0f)  % 2 ? 685 : 692; }
            Draw(img, kKidX[i], kKidY[i], 56.0f, 64.0f, 0xFFFFFFFF);
        }
        // the crying kid 2 (objInfo 394): idle [683,697]@1.5fps; the murder
        // reaction anim 14 = [683,713,716,717,718,701,720,719] @ speed 1
        if (mg.carStage >= 2) {
            static const i32 kReact[8] = { 683, 713, 716, 717, 718, 701, 720, 719 };
            int fi = (int)(mg.manStageT * 0.6f);
            if (fi < 0) fi = 0;
            if (fi > 7) fi = 7;
            Draw(kReact[fi], 404.0f, 118.0f, 56.0f, 64.0f, 0xFFFFFFFF);
        } else {
            Draw((int)(mg.t * 1.5f) % 2 ? 697 : 683, 404.0f, 118.0f, 56.0f, 64.0f, 0xFFFFFFFF);
        }
        if (mg.murder && mg.carStage >= 1)
            Draw(715, (f32)mg.carX, 67.0f, 150.0f, 70.0f, 0xFFFFFFFF);
        if (mg.carStage == 2) Draw(699, 467.0f, 300.0f, 46.0f, 72.0f, 0xFFFFFFFF);
        Draw(walkFrame ? 417 : 412, (f32)mg.px - 45.0f, (f32)mg.py - 55.0f,
             90.0f, 100.0f, 0xFFFFFFFF);
    } else if (mg.game == 3) {
        // frame 24 GIVE GIFTS, GIVE LIFE
        Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(723, 391.0f, 8.0f, 250.0f, 50.0f, 0xFFFFFFFF);
        static const f32 kHeadX[4] = { 235, 200, 814, 818 };
        static const f32 kHeadY[4] = { 204, 594, 190, 580 };
        static const i32 kHeadImg[4] = { 728, 763, 762, 765 };
        for (i32 i = 0; i < 4; ++i) {
            if (!mg.phaseB || !mg.headGifted[i] || mg.kidSad[i] == 0)
                Draw(kHeadImg[i], kHeadX[i], kHeadY[i], 52.0f, 52.0f, 0xFFFFFFFF);
        }
        for (i32 i = 0; i < 4; ++i) {
            static const f32 kHelpX[4] = { 228, 204, 830, 830 };
            static const f32 kHelpY[4] = { 239, 635, 239, 623 };
            Draw(761, kHelpX[i], kHelpY[i], 46.0f, 60.0f, 0xFFFFFFFF);
        }
        if (!mg.phaseB) Draw(779, 526.0f, 388.0f, 79.0f, 57.0f, 0xFFFFFFFF);
        Draw(760, (f32)mg.px - 45.0f, (f32)mg.py - 60.0f, 90.0f, 110.0f, 0xFFFFFFFF);
    } else {
        // frame 25 FOXY'S PARTY: the 2048-wide world, the camera snaps at
        // the x=1024 line (dump groups 44/45)
        const f32 cam = (mg.px >= 1024) ? -1024.0f : 0.0f;
        Draw(778, cam + 0.0f,    0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(382, cam + 1024.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(784, cam + 273.0f, 268.0f, 120.0f, 140.0f, 0xFFFFFFFF);
        Draw(784, cam + 784.0f, 268.0f, 120.0f, 140.0f, 0xFFFFFFFF);
        static const f32 kKidsX[5] = { 1608, 1778, 1550, 1766, 1602 };
        static const f32 kKidsY[5] = { 217, 322, 404, 508, 594 };
        for (i32 i = 0; i < 5; ++i) {
            // the kids go silent on the second visit (group 57)
            const u32 tint = (mg.cycles >= 2) ? 0xFF808080 : 0xFFFFFFFF;
            Draw(794, cam + kKidsX[i], (f32)kKidsY[i], 56.0f, 64.0f, tint);
        }
        if (mg.cycles >= 2) Draw(799, cam + 1560.0f, 430.0f, 46.0f, 72.0f, 0xFFFFFFFF);
        if (mg.phase == 1 && mg.cycles < 2)
            Draw(787, cam + 819.0f, 566.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(walkFrame ? 726 : 722, (f32)mg.px + cam - 45.0f, (f32)mg.py - 55.0f,
             90.0f, 100.0f, 0xFFFFFFFF);
    }
    // the scripted attack — the REAL frame lists from the "attack animation"
    // object (application.json): 20 = the car/man scare, 21 = the Puppet,
    // 15 = Foxy; fullscreen cells at speed*0.6 fps, then the load jump
    if (attack) {
        static const i32 kFrames[3][16] = {
            { 401, 385, 386, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 0, 0 },  // 15
            { 515, 195, 517, 313, 315, 316, 317, 318, 324, 340, 342, 343, 345, 347, 514, 0 },// 20
            { 320, 246, 516, 247, 248, 249, 250, 306, 310, 311, 312, 314, 319, 0, 0, 0 }     // 21
        };
        static const i32 kCount[3] = { 14, 15, 13 };
        static const i32 kSpd[3]   = { 60, 50, 40 };
        int idx = -1;
        if      (mg.attackAnim == 15) idx = 0;
        else if (mg.attackAnim == 20) idx = 1;
        else if (mg.attackAnim == 21) idx = 2;
        if (idx >= 0) {
            const f32 fps = (f32)kSpd[idx] * 0.6f;
            int fi = (int)(mg.attackT * fps);
            if (fi < 0) fi = 0;
            if (fi >= kCount[idx]) fi = kCount[idx] - 1;
            Draw(kFrames[idx][fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        } else {
            Draw(361, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }
    // the CRT scanline flicker (the dump's scanline groups: every 500 ms the
    // alpha coefficient := 200+Random(100) -> alpha 0.02..0.22)
    {
        u32 h = (u32)(int)(mg.t / 0.5f) * 2654435761u + 21u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const u32 coeff = 200u + h % 100u;
        const u32 a = 255u - coeff;
        if (a > 0u)
            Draw(649, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }
}

void FNaF2Render::RenderEndBars() {
    // frames 20/22: black + the two CRT bars
    Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(678, 0.0f, 300.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
    Draw(680, 0.0f, 420.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
}

} // namespace fnaf
