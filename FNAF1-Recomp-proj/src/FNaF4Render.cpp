/**
 * FNaF4Render.cpp: v2.61 — the FNAF4 renderer (title + the night loop).
 * Dump data: frame 1 "titlescreen" placement table + the bedroom frame 3
 * "level" (1300x768). The view overlays are the full-frame art of the
 * dump's animation tables: hall peeks 89/255 (shut doors 88/375), the
 * closet 422 with the Foxy stages 304/286/288/290 (Fredbear 266), the bed
 * 511 with the Freddy-counter states 492/805/806/807 (<=10), 423 (11-20),
 * 386 (21-30), 391 (>30), the walk darks 45/160/57 and the paranoia flash
 * 99. Jumpscares: 450/487/620 (img 180's anims), the bedroom attacks
 * 592/358/368, the bite 642.
 */

#include "FNaF4Render.h"
#include "FNaF4Game.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <cstdio>

namespace fnaf {

static const f32 kScaleX = 1280.0f / 1024.0f;   // full-stretch 16:9
static const f32 kScaleY = 720.0f / 768.0f;

void FNaF4Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

void FNaF4Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  fw * kScaleX, fh * kScaleY, color);
}

void FNaF4Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = (wx - pan) * kScaleX;
    if (sx >= 1280.0f || sx + fw * kScaleX <= 0.0f) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, sx, wy * kScaleY, fw * kScaleX, fh * kScaleY, color);
}

// ---- title (frame 1 "titlescreen") -------------------------------------

void FNaF4Render::RenderTitle(f32 time, i32 optionSelected, bool beat5) {
    if (!m_batch || !m_pak) return;
    (void)time;

    // bg (Backdrop img_626 — the red sky)
    Draw(626, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // the big heading (obj "title", img_658, hotspot-corrected (589,-2))
    Draw(658, 589.0f, -2.0f, 408.0f, 765.0f, 0xFFFFFFFF);

    // menu (hotspot-corrected left/top). No selector object was dumped for
    // this frame — the selected row is a red-tint highlight (labeled).
    static const struct { i32 img; f32 x, y, w, h; } kRows[4] = {
        { 730, 442.0f, 379.0f, 158.0f, 17.0f },   // "New Game"
        { 737, 442.0f, 425.0f, 151.0f, 17.0f },   // "Continue"
        { 738, 441.0f, 470.0f, 161.0f, 17.0f },   // "6th Night"
        { 731, 472.0f, 517.0f,  98.0f, 17.0f }    // "Extra"
    };
    for (i32 i = 0; i < 4; ++i) {
        u32 tint = 0xFFFFFFFF;
        if (i == (optionSelected & 3)) tint = 0xFF5050E0;         // selected
        if (i == 2 && !beat5)          tint = 0xFF606060;         // locked
        if (i == 3)                    tint = 0xFF909090;         // deferred
        Draw(kRows[i].img, kRows[i].x, kRows[i].y, kRows[i].w, kRows[i].h, tint);
    }

    // footer texts (per the composite)
    if (m_text) {
        m_text->DrawText((int)(160.0f + 398.0f * 0.9375f), (int)(737.0f * 0.9375f),
                         "Press and hold DELETE to erase all data.", 0xFFC04040);
        m_text->DrawText((int)(160.0f + 770.0f * 0.9375f), (int)(737.0f * 0.9375f),
                         "Copyright (c) 2015 Scott Cawthon", 0xFFC04040);
        m_text->DrawText((int)(160.0f + 975.0f * 0.9375f), (int)(715.0f * 0.9375f),
                         "v1.1", 0xFFC04040);
    }
}

// ---- the between-night cards -------------------------------------------

void FNaF4Render::RenderNightStart(i32 night) {
    if (m_text) {
        char buf[24];
        Snprintf(buf, sizeof(buf), "Night %d", night);
        m_text->DrawText((int)(540.0f * kScaleX), (int)(340.0f * kScaleY), buf, 0xFFFFFFFF);
        m_text->DrawText((int)(560.0f * kScaleX), (int)(390.0f * kScaleY), "12 AM", 0xFF909090);
    }
}

void FNaF4Render::RenderNightWin() {
    // frame 5 "night win": the clock digits (img 663/678) animate 5 -> 6;
    // debug-font stopgap until the digit art is wired
    if (m_text) {
        m_text->DrawText((int)(560.0f * kScaleX), (int)(320.0f * kScaleY), "6 AM", 0xFFFFFFFF);
        m_text->DrawText((int)(520.0f * kScaleX), (int)(380.0f * kScaleY),
                         "night complete", 0xFF909090);
    }
}

// ---- bedroom (frame 3 "level", 1300x768) --------------------------------

void FNaF4Render::RenderBedroom(const FNaF4Game& game, f32 time, f32 pan) {
    if (!m_batch || !m_pak) return;
    (void)time;

    // the bedroom base
    DrawWorld(4, 0.0f, 0.0f, 1300.0f, 768.0f, pan, 0xFFFFFFFF);

    // the walk darks (world-size overlays; first frames 45/160/57)
    if (game.IsWalking()) {
        const f32 k = game.GetWalkT() / 0.45f;          // 1 -> 0
        const u32 a = (u32)(255.0f * (k > 0.5f ? (1.0f - k) * 2.0f : k * 2.0f));
        DrawWorld(game.GetWalkDir(), 0.0f, 0.0f, 1300.0f, 768.0f, pan,
                  (a << 24) | 0x00FFFFFFu);
    }

    // the flashlight peeks (full-frame art)
    if (game.Peeking()) {
        switch (game.GetPosition()) {
            case FNaF4Game::P_LEFT:
                Draw(game.IsDoorShut(0) ? 88 : 89, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                break;
            case FNaF4Game::P_RIGHT:
                Draw(game.IsDoorShut(1) ? 375 : 255, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                break;
            case FNaF4Game::P_CLOSET: {
                // the closet base pan (422) + the occupant art by stage
                Draw(422, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                if (game.IsFoxyInCloset()) {
                    if (game.IsFredbearCloset()) {
                        Draw(266, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                    } else {
                        // stage 0 -> 304, 1 -> 286, 2 -> 288, 3 -> 290
                        const i32 c = game.GetClosetCounter();
                        const i32 art = (c >= 6) ? 290 : (c >= 4) ? 288
                                      : (c >= 2) ? 286 : 304;
                        Draw(art, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                    }
                }
                break;
            }
            case FNaF4Game::P_BED: {
                // the bed art by the Freddy counter (g232-238)
                const i32 c = game.GetFreddyCounter();
                if (c <= 0)       Draw(511, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                else if (c <= 10) {
                    static const i32 kMini[4] = { 492, 805, 806, 807 };
                    Draw(kMini[c % 4], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                }
                else if (c <= 20) Draw(423, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                else if (c <= 30) Draw(386, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                else              Draw(391, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                break;
            }
            default:
                break;
        }
    }
    // the forced-flashlight closet view (you cannot look away)
    else if (game.GetFoxyGotYou() && game.GetPosition() == FNaF4Game::P_CLOSET) {
        Draw(422, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        if (game.IsFredbearCloset()) Draw(266, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        else {
            const i32 c = game.GetClosetCounter();
            const i32 art = (c >= 6) ? 290 : (c >= 4) ? 288 : (c >= 2) ? 286 : 304;
            Draw(art, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }

    // the paranoia black flash (img 99)
    if (game.GetBlackFlashA() > 0.5f) {
        const u32 a = (u32)game.GetBlackFlashA();
        Draw(99, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }

    // HUD (debug-font stopgap)
    if (m_text) {
        char buf[40];
        const i32 h = game.GetHour();
        Snprintf(buf, sizeof(buf), "%d AM", h == 0 ? 12 : h);
        m_text->DrawText(40, 20, buf, 0xFFB0B0B0);
        Snprintf(buf, sizeof(buf), "Night %d", game.GetNight());
        m_text->DrawText(40, 44, buf, 0xFF707070);
        if (game.GetFoxyGotYou())
            m_text->DrawText(480, 20, "FLASH THE CLOSET", 0xFF6060FF);
    }
}

// ---- the jumpscare / bite overlays ---------------------------------------

void FNaF4Render::DrawAttack(const FNaF4Game& game) {
    if (game.GetAttackImg() != 0)
        Draw(game.GetAttackImg(), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (game.GetBiteT() > 0.0f)
        Draw(642, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

} // namespace fnaf
