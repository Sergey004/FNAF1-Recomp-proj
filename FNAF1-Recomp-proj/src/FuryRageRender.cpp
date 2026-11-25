/**
 * FuryRageRender.cpp: v2.129 — Fury's Rage module renderer.
 *
 * Every screen is the dump's own art: the generated scene table carries the
 * image id, the draw origin (the instance point minus the hotspot) and the
 * natural size; the original window is 1920x1080 and the port renders
 * 1280x720, so the whole world scales by 2/3.
 *
 * v2.129: the character select is rebuilt from the dump's frame 3: the
 * background (750), the three unselected fighter cards at the dump's
 * coordinates with the dump's own alpha (action 65 = 100), the selected
 * fighter's stat panel (Active 6: 761 Freddy / 762 Monty / 763 Roxanne /
 * 760 Chica) at (370, 605) and the control hints (829). The roster is
 * Freddy / Monty / Roxanne / Chica — this game has no Bonnie and no Foxy.
 *
 * v2.130: the level draws the map with the dump's own instances (the
 * fade overlay and the invisible collision boxes stay out), the object's
 * own blend as its alpha and the fighters in the dump's scale - the
 * picture stands on the fighter's feet.
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
    // Frame 3 "character select" of the dump (1920x1080):
    //   * "Active"        750 - the background
    //   * "Active 2/5/3/4" - the four fighter cards (524x460) at
    //                        x = 110 / 512 / 903 / 1303, y = 170
    //   * "Active 6"      - the stat panel (1047x460) at (370, 605); its
    //                        animation picks the panel of the chosen fighter
    //   * "Active 8"      829 - the control hints (349x284) at (1548, 787)
    // The events hide the card of the selected fighter (alpha 0) and show it
    // big inside the panel instead, so the card row only carries the others.
    Draw(750, 0.0f, 0.0f, 1920.0f, 1080.0f, 0xFFFFFFFF);

    static const struct { i32 img; f32 x; } kCards[4] = {
        { 752, 110.0f },   // Freddy
        { 758, 512.0f },   // Monty
        { 754, 903.0f },   // Roxanne
        { 756, 1303.0f },  // Chica
    };
    // "Active 6" animation 0..3 = Chica, Freddy, Monty, Roxanne
    static const i32 kPanels[4] = { 761, 762, 763, 760 };

    // The events set the unselected cards' alpha (action 65) to 100 - the
    // dump's own frame renders them at 100/255, so the row reads as dimmed
    // while the chosen fighter shows big in the panel.
    const u32 kCardColor = 0x64FFFFFF;

    const int sel = game.GetCharSel();
    for (int i = 0; i < 4; ++i) {
        if (i == sel) continue;          // the dump hides the selected card
        Draw(kCards[i].img, kCards[i].x, 170.0f, 524.0f, 460.0f, kCardColor);
    }
    if (sel >= 0 && sel < 4)
        Draw(kPanels[sel], 370.0f, 605.0f, 1047.0f, 460.0f, 0xFFFFFFFF);

    Draw(829, 1548.0f, 787.0f, 349.0f, 284.0f, 0xFFFFFFFF);
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
    // the object's own blend (the dump's alpha)
    const u32 color = 0xFF000000u | ((u32)it.alpha << 16) |
                      ((u32)it.alpha << 8) | (u32)it.alpha;
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
                            tw, th, 0.0f, 0.0f, u1, v1, color);
            }
        }
    } else {
        const f32 w = (it.kind == 2 && it.w) ? drawW : (f32)t->origWidth * kFuryScale;
        const f32 h = (it.kind == 2 && it.h) ? drawH : (f32)t->origHeight * kFuryScale;
        batch->Draw(t->texture, drawX, drawY, w, h, 0.0f, 0.0f, u1, v1, color);
    }
}

// A fighter sprite in the dump's own scale: the picture stands on the
// fighter's feet (the dump anchors it with the object's hotspot).
static void FuryDrawFighter(PakLoader* pak, SpriteBatch* batch, int img,
                            f32 worldX, f32 worldY, f32 camX, f32 camY, u32 color) {
    if (!pak || !batch || img < 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", img);
    PakLoadedTexture* t = pak->FindTexture(name);
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    const f32 w = (f32)t->origWidth * kFuryScale;
    const f32 h = (f32)t->origHeight * kFuryScale;
    // the feet: the hitbox's lower edge
    const f32 feet = worldY + 31.0f * kFuryScale - camY * (f32)kFuryTile * kFuryScale;
    const f32 cx = worldX * kFuryScale - camX * (f32)kFuryTile * kFuryScale;
    batch->Draw(t->texture, cx - w * 0.5f, feet - h, w, h, 0.0f, 0.0f, u1, v1, color);
}

// The dump's child object "Active 6" carries the chosen fighter's picture;
// it hangs off the player object with its own hotspot (161, 305).
static void FuryDrawOverlay(PakLoader* pak, SpriteBatch* batch, int img,
                             f32 worldX, f32 worldY, f32 camX, f32 camY, u32 color) {
    if (!pak || !batch || img < 0) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", img);
    PakLoadedTexture* t = pak->FindTexture(name);
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    const f32 w = (f32)t->origWidth * kFuryScale;
    const f32 h = (f32)t->origHeight * kFuryScale;
    const f32 x = (worldX - 161.0f) * kFuryScale - camX * (f32)kFuryTile * kFuryScale;
    const f32 y = (worldY - 305.0f) * kFuryScale - camY * (f32)kFuryTile * kFuryScale;
    batch->Draw(t->texture, x, y, w, h, 0.0f, 0.0f, u1, v1, color);
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
        // the fighter and the chosen character's picture (the dump's child
        // object "Active 6")
        const f32 fx = pl.x * (f32)kFuryTile;
        const f32 fy = pl.y * (f32)kFuryTile + 31.0f;   // the feet
        FuryDrawFighter(m_pak, m_batch, pl.img,
                        pl.x * (f32)kFuryTile, pl.y * (f32)kFuryTile,
                        camX, camY, 0xFFFFFFFF);
        if (pl.overlay >= 0)
            FuryDrawOverlay(m_pak, m_batch, pl.overlay, fx, fy, camX, camY, 0xFFFFFFFF);
    }
    for (i32 i = 0; i < game.GetEnemyCount(); ++i) {
        const FuryFighter& e = game.GetEnemy(i);
        if (e.dead || e.img < 0) continue;
        FuryDrawFighter(m_pak, m_batch, e.img,
                        e.x * (f32)kFuryTile, e.y * (f32)kFuryTile,
                        camX, camY, 0xFFFFFFFF);
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
