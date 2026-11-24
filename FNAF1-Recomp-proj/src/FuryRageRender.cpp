/**
 * FuryRageRender.cpp: v2.108 — Fury's Rage module renderer.
 *
 * Every screen is the dump's own art: the generated scene table carries the
 * image id, the draw origin (the instance point minus the hotspot) and the
 * natural size; the original window is 1920x1080 and the port renders
 * 1280x720, so the whole world scales by 2/3.
 */

#include "FuryRageRender.h"
#include "FuryRageScenes.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"

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
            if (s.img == 3) continue;        // the counter placeholder image
            if (s.img == 736) continue;      // the dump's fade overlay
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

void FuryRageRender::RenderLevels(const FuryRageGame& game) {
    (void)game;
    DrawScene(FR_LEVELS);
}

} // namespace fnaf
