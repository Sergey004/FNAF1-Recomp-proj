/**
 * FuryRageRender.cpp: v2.128 — Fury's Rage module renderer.
 *
 * Every screen is the dump's own art: the generated scene table carries the
 * image id, the draw origin (the instance point minus the hotspot) and the
 * natural size; the original window is 1920x1080 and the port renders
 * 1280x720, so the whole world scales by 2/3.
 *
 * v2.128: the beat-'em-up level now draws the generated map (FuryRageMap.h,
 * painter's order per layer) plus the fighter, enemies, and a HUD
 * (health, score, wave) relative to the camera.
 * VS2010/C++03.
 */

#include "FuryRageRender.h"
#include "FuryRageGame.h"
#include "FuryRageMap.h"
#include "FuryRageScenes.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <cstdio>

namespace fnaf {

// 1920x1080 -> 1280x720
static const f32 kFuryScale = 1280.0f / 1920.0f;

void FuryRageRender::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak;
    m_batch = batch;
    m_text = text;
}

void FuryRageRender::Draw(int img, float x, float y, float w, float h, u32 color) {
    if (!m_batch || !m_pak || img < 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", img);
    PakLoadedTexture* t = m_pak->FindTexture(name);
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x * kFuryScale, y * kFuryScale,
                  w * kFuryScale, h * kFuryScale, 0.0f, 0.0f, u1, v1, color);
}

void FuryRageRender::DrawNat(int img, float x, float y, u32 color) {
    if (!m_batch || !m_pak || img < 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", img);
    PakLoadedTexture* t = m_pak->FindTexture(name);
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x * kFuryScale, y * kFuryScale,
                  (f32)t->origWidth * kFuryScale, (f32)t->origHeight * kFuryScale,
                  0.0f, 0.0f, u1, v1, color);
}

void FuryRageRender::DrawScene(int frameIndex) {
    // the generated table: one block per dump frame
    for (int i = 0; i < FuryRageSceneFrameCount; ++i) {
        const FuryRageSceneFrame& f = FuryRageSceneFrames[i];
        if (f.index != frameIndex) continue;
        for (int k = 0; k < f.count; ++k) {
            const FuryRageSceneInst& s = FuryRageScenes[f.first + k];
            if (s.img == 3) continue;      // the counter placeholder image
            if (s.img == 736) continue;    // the dump's fade overlay
            if (s.w <= 0 || s.h <= 0) continue;
            Draw(s.img, (f32)s.left, (f32)s.top, (f32)s.w, (f32)s.h, 0xFFFFFFFF);
        }
        return;
    }
}

void FuryRageRender::RenderTitle(const FuryRageGame& game) {
    DrawScene(FR_TITLE);
    // the dump's hard-mode badge ("Active 2" is hidden unless the save's
    // "hard mode?" counter is 2) and the three coins (the c1/c2/c3 counts)
    if (game.GetHardMode())
        DrawNat(773, 1000.0f, 335.0f, 0xFFFFFFFF);
    for (int i = 0; i < 3; ++i) {
        if (game.GetCoin(i) > 0)
            DrawNat(766, 25.0f + (f32)i * 160.0f, 913.0f, 0xFFFFFFFF);
    }
}

void FuryRageRender::RenderCharacterSelect(const FuryRageGame& game) {
    DrawScene(FR_CHARACTER_SELECT);
    // the selection cursor (the dump's "Active 6", moved by the events)
    const f32 kX[4] = { 110.0f, 370.0f, 512.0f, 903.0f };
    const int sel = game.GetCharSel();
    if (sel >= 0 && sel < 4)
        DrawNat(760, kX[sel], 605.0f, 0xFFFFFFFF);
}

// dump-space -> screen-space for a level (camera-relative)
static void FuryDrawMapInst(FuryRageRender* r, PakLoader* pak, SpriteBatch* batch,
                            const FuryMapInst& it, f32 camX, f32 camY) {
    const f32 drawX = (f32)it.left * kFuryScale - camX * (f32)kFuryTile * kFuryScale;
    const f32 drawY = (f32)it.top  * kFuryScale - camY * (f32)kFuryTile * kFuryScale;
    const f32 drawW = (f32)it.w * kFuryScale;
    const f32 drawH = (f32)it.h * kFuryScale;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", it.img);
    PakLoadedTexture* t = pak ? pak->FindTexture(name) : 0;
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    if (it.kind == 1) {
        // quick backdrop: tile the image over its rect
        const f32 tw = (f32)t->origWidth * kFuryScale;
        const f32 th = (f32)t->origHeight * kFuryScale;
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
        const f32 w = (it.kind == 2 && it.w) ? drawW : (f32)t->origWidth * kFuryScale;
        const f32 h = (it.kind == 2 && it.h) ? drawH : (f32)t->origHeight * kFuryScale;
        batch->Draw(t->texture, drawX, drawY, w, h, 0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
    }
}

void FuryRageRender::RenderLevels(const FuryRageGame& game) {
    if (!m_batch) return;
    const f32 camX = game.GetCamX();
    const f32 camY = game.GetCamY();

    // ---- map (painter's order per layer, then kind rank) ----
    for (i32 i = 0; i < kFuryMapInstCount; ++i) {
        const FuryMapInst& it = kFuryMapInsts[i];
        const f32 rl = (f32)it.left - camX * (f32)kFuryTile;
        const f32 rt = (f32)it.top  - camY * (f32)kFuryTile;
        const f32 rr = rl + (f32)it.w;
        const f32 rb = rt + (f32)it.h;
        if (rr < -kFuryTile || rl > (f32)kFuryMapW * (f32)kFuryTile + kFuryTile) continue;
        if (rb < -kFuryTile || rt > (f32)kFuryMapH * (f32)kFuryTile + kFuryTile) continue;
        FuryDrawMapInst(this, m_pak, m_batch, it, camX, camY);
    }

    // ---- entities ----
    const FuryFighter& pl = game.GetPlayer();
    if (!pl.dead && pl.img >= 0) {
        const f32 px = pl.x * kFuryScale - camX * (f32)kFuryTile * kFuryScale;
        const f32 py = pl.y * kFuryScale - camY * (f32)kFuryTile * kFuryScale;
        Draw(pl.img, px, py,
             (f32)pl.w * (f32)kFuryTile * 0.01f * kFuryScale,
             (f32)pl.h * (f32)kFuryTile * 0.01f * kFuryScale, 0xFFFFFFFF);
    }
    for (i32 i = 0; i < game.GetEnemyCount(); ++i) {
        const FuryFighter& e = game.GetEnemy(i);
        if (e.dead || e.img < 0) continue;
        const f32 ex = e.x * kFuryScale - camX * (f32)kFuryTile * kFuryScale;
        const f32 ey = e.y * kFuryScale - camY * (f32)kFuryTile * kFuryScale;
        Draw(e.img, ex, ey,
             (f32)e.w * (f32)kFuryTile * 0.01f * kFuryScale,
             (f32)e.h * (f32)kFuryTile * 0.01f * kFuryScale, 0xFFFFFFFF);
    }

    // ---- HUD ----
    if (m_text) {
        char line[96];
        Snprintf(line, sizeof(line), "WAVE %d   SCORE %d", game.GetWave()+1, game.GetScore());
        m_text->DrawText(20, 20, line, 0xFFFFFFFF);
        Snprintf(line, sizeof(line), "HP %3d/%3d", game.GetPlayer().hp, game.GetPlayer().maxHp);
        m_text->DrawText(20, 40, line, 0xFFFFFFFF);
        Snprintf(line, sizeof(line), "ENEMY %3d/%3d", game.GetEnemyHp(), game.GetEnemyMaxHp());
        m_text->DrawText(20, 60, line, 0xFFFFFFFF);
    }

    // health bar (red) at top-left
    if (m_batch) {
        const f32 hx = 140.0f, hy = 44.0f, hw = 200.0f, hh = 10.0f;
        m_batch->DrawSolid(hx, hy, hw, hh, 0xFF000000);
        const f32 hfrac = game.GetPlayer().maxHp ? (f32)game.GetPlayer().hp / (f32)game.GetPlayer().maxHp : 0.0f;
        m_batch->DrawSolid(hx + 2.0f, hy + 2.0f, (hw - 4.0f) * hfrac, hh - 4.0f, 0xFFFF4040);
    }
}

} // namespace fnaf
