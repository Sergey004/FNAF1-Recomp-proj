/**
 * FIS2Render.cpp: v2.127 — Freddy in Space 2 module renderer.
 *
 * The art of every screen is dump-derived (FIS2Scenes.h, generated from the
 * FrameLayout files of fis2.exe). The original window is 720x480 (3:2); the
 * port renders 1280x720, so the game is pillarboxed: 1080x720 with 100 px
 * bars on the sides.
 *
 * v2.127: the platformer levels now draw the generated map (FIS2Map.h,
 * painter's order per layer) plus the player, enemies, projectiles and
 * pickups relative to the camera, and a HUD (health, ammo, coins, xp).
 * VS2010/C++03.
 */

#include "FIS2Render.h"
#include "FIS2Game.h"
#include "FIS2Map.h"
#include "FIS2Scenes.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <cstdio>
#include <cmath>

namespace fnaf {

// 720x480 -> 1080x720 pillarboxed inside 1280x720
static const f32 kFisScale = 720.0f / 480.0f;      // 1.5
static const f32 kFisOffsetX = (1280.0f - 720.0f * kFisScale) * 0.5f;   // 100

void FIS2Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak;
    m_batch = batch;
    m_text = text;
}

void FIS2Render::Draw(int img, float x, float y, float w, float h, u32 color) {
    if (!m_batch || !m_pak || img < 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", img);
    PakLoadedTexture* t = m_pak->FindTexture(name);
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x * kFisScale + kFisOffsetX, y * kFisScale,
                  w * kFisScale, h * kFisScale, 0.0f, 0.0f, u1, v1, color);
}

void FIS2Render::DrawNat(int img, float x, float y, u32 color) {
    if (!m_batch || !m_pak || img < 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", img);
    PakLoadedTexture* t = m_pak->FindTexture(name);
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x * kFisScale + kFisOffsetX, y * kFisScale,
                  (f32)t->origWidth * kFisScale, (f32)t->origHeight * kFisScale,
                  0.0f, 0.0f, u1, v1, color);
}

// dump-space -> screen-space for a level (camera-relative)
static f32 FisSx(f32 dx) { return dx * kFisScale + kFisOffsetX; }
static f32 FisSy(f32 dy) { return dy * kFisScale; }

void FIS2Render::DrawScene(int frameIndex) {
    for (int i = 0; i < FIS2SceneFrameCount; ++i) {
        const FIS2SceneFrame& f = FIS2SceneFrames[i];
        if (f.index != frameIndex) continue;
        for (int k = 0; k < f.count; ++k) {
            const FIS2SceneInst& s = FIS2Scenes[f.first + k];
            if (s.img == 3) continue;        // the counter placeholder image
            if (s.w <= 0 || s.h <= 0) continue;
            Draw(s.img, (f32)s.left, (f32)s.top, (f32)s.w, (f32)s.h, 0xFFFFFFFF);
        }
        return;
    }
}

void FIS2Render::RenderTitle(const FIS2Game& game) {
    (void)game;
    // The dump's frame 2 "title screen" starts empty: its art is a slow
    // scroll-in (the events move "Active 7" from x=-1810, "Active 8" up
    // and "Active 9" down - that animation is the gameplay milestone).
    // The title LOGO itself is the art of frame 1 "intro" (the seven
    // overlapping images 3668/3670/3675/3590/3589/3676/720), so the
    // title screen draws that frame and then the title frame on top.
    DrawScene(FIS2_INTRO);
    DrawScene(FIS2_TITLE);
}

void FIS2Render::RenderStageSelect(const FIS2Game& game) {
    (void)game;
    DrawScene(FIS2_STAGE_SELECT);
}

// draw a level map instance relative to the camera
static void DrawMapInst(FIS2Render* r, PakLoader* pak, SpriteBatch* batch,
                        const FisMapInst& it, f32 camX, f32 camY) {
    const f32 drawX = FisSx((f32)it.left - camX * (f32)kFisTile);
    const f32 drawY = FisSy((f32)it.top  - camY * (f32)kFisTile);
    const f32 drawW = (f32)it.w * kFisScale;
    const f32 drawH = (f32)it.h * kFisScale;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", it.img);
    PakLoadedTexture* t = pak ? pak->FindTexture(name) : 0;
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    if (it.kind == 1) {
        // quick backdrop: tile the image over its rect
        const f32 tw = (f32)t->origWidth * kFisScale;
        const f32 th = (f32)t->origHeight * kFisScale;
        if (tw <= 0.0f || th <= 0.0f) return;
        const i32 cols = (i32)((drawW + tw - 0.5f) / tw);
        const i32 rows = (i32)((drawH + th - 0.5f) / th);
        for (i32 cy = 0; cy < rows; ++cy) {
            for (i32 cx = 0; cx < cols; ++cx) {
                batch->Draw(t->texture, drawX + cx * tw, drawY + cy * th,
                            tw, th, 0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
            }
        }
    } else {
        // sprite / backdrop: draw at its own size (or the rect if kind==2)
        const f32 w = (it.kind == 2 && it.w) ? drawW : (f32)t->origWidth * kFisScale;
        const f32 h = (it.kind == 2 && it.h) ? drawH : (f32)t->origHeight * kFisScale;
        batch->Draw(t->texture, drawX, drawY, w, h, 0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
    }
}

void FIS2Render::RenderLevel(const FIS2Game& game) {
    if (!m_batch) return;
    const f32 camX = game.GetCamX();
    const f32 camY = game.GetCamY();

    // ---- map (painter's order per layer, then kind rank) ----
    for (i32 i = 0; i < kFisMapInstCount; ++i) {
        const FisMapInst& it = kFisMapInsts[i];
        // cull: skip objects fully off-screen (with a 1 tile margin)
        const f32 rl = (f32)it.left - camX * (f32)kFisTile;
        const f32 rt = (f32)it.top  - camY * (f32)kFisTile;
        const f32 rr = rl + (f32)it.w;
        const f32 rb = rt + (f32)it.h;
        if (rr < -kFisTile || rl > (f32)kFisMapW * (f32)kFisTile + kFisTile) continue;
        if (rb < -kFisTile || rt > (f32)kFisMapH * (f32)kFisTile + kFisTile) continue;
        DrawMapInst(this, m_pak, m_batch, it, camX, camY);
    }

    // ---- entities ----
    const FisEntity& pl = game.GetPlayer();
    if (!pl.dead && pl.img >= 0) {
        const f32 px = FisSx(pl.x * (f32)kFisTile - camX * (f32)kFisTile);
        const f32 py = FisSy(pl.y * (f32)kFisTile - camY * (f32)kFisTile);
        Draw(pl.img, px, py,
             (f32)pl.w * (f32)kFisTile * 0.01f * kFisScale,
             (f32)pl.h * (f32)kFisTile * 0.01f * kFisScale, 0xFFFFFFFF);
    }
    for (i32 i = 0; i < game.GetEnemyCount(); ++i) {
        const FisEntity& e = game.GetEnemy(i);
        if (e.dead || e.img < 0) continue;
        const f32 ex = FisSx(e.x * (f32)kFisTile - camX * (f32)kFisTile);
        const f32 ey = FisSy(e.y * (f32)kFisTile - camY * (f32)kFisTile);
        Draw(e.img, ex, ey,
             (f32)e.w * (f32)kFisTile * 0.01f * kFisScale,
             (f32)e.h * (f32)kFisTile * 0.01f * kFisScale, 0xFFFFFFFF);
    }
    for (i32 i = 0; i < game.GetProjectileCount(); ++i) {
        const FisProjectile& p = game.GetProjectile(i);
        if (p.dead || p.img < 0) continue;
        const f32 px = FisSx(p.x * (f32)kFisTile - camX * (f32)kFisTile);
        const f32 py = FisSy(p.y * (f32)kFisTile - camY * (f32)kFisTile);
        Draw(p.img, px, py, 30.0f * kFisScale, 30.0f * kFisScale, 0xFFFFFFFF);
    }
    for (i32 i = 0; i < game.GetPickupCount(); ++i) {
        const FisEntity& p = game.GetPickup(i);
        if (p.dead || p.img < 0) continue;
        const f32 px = FisSx(p.x * (f32)kFisTile - camX * (f32)kFisTile);
        const f32 py = FisSy(p.y * (f32)kFisTile - camY * (f32)kFisTile);
        Draw(p.img, px, py,
             (f32)p.w * (f32)kFisTile * 0.01f * kFisScale,
             (f32)p.h * (f32)kFisTile * 0.01f * kFisScale, 0xFFFFFFFF);
    }

    // ---- HUD ----
    if (m_text) {
        char line[96];
        Snprintf(line, sizeof(line), "HP %3d/%3d  AMMO %3d/%3d",
                 game.GetHealth(), game.GetMaxHealth(), game.GetAmmo(), game.GetMaxAmmo());
        m_text->DrawText(20, 20, line, 0xFFFFFFFF);
        Snprintf(line, sizeof(line), "COINS %d   XP %d/100   LVL %d",
                 game.GetCoins(), game.GetLevelXP(), game.GetLevel()+1);
        m_text->DrawText(20, 40, line, 0xFFFFFFFF);
        Snprintf(line, sizeof(line), "AREA %d", game.GetLevel()+1);
        m_text->DrawText(20, 60, line, 0xFFFFFFFF);
    }

    // health bar (red) above ammo bar (yellow) at top-left
    if (m_batch) {
        const f32 hx = 140.0f, hy = 24.0f, hw = 200.0f, hh = 10.0f;
        m_batch->DrawSolid(hx, hy, hw, hh, 0xFF000000);
        const f32 hfrac = game.GetMaxHealth() ? (f32)game.GetHealth() / (f32)game.GetMaxHealth() : 0.0f;
        m_batch->DrawSolid(hx + 2.0f, hy + 2.0f, (hw - 4.0f) * hfrac, hh - 4.0f, 0xFFFF4040);
        const f32 ay = hy + 14.0f;
        m_batch->DrawSolid(hx, ay, hw, hh, 0xFF000000);
        const f32 afrac = game.GetMaxAmmo() ? (f32)game.GetAmmo() / (f32)game.GetMaxAmmo() : 0.0f;
        m_batch->DrawSolid(hx + 2.0f, ay + 2.0f, (hw - 4.0f) * afrac, hh - 4.0f, 0xFFFFD040);
    }
}

} // namespace fnaf
