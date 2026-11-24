/**
 * FIS2Render.cpp: v2.108 — Freddy in Space 2 module renderer.
 *
 * Every screen is the dump's own art: the generated scene table carries the
 * image id, the draw origin (the instance point minus the hotspot) and the
 * natural size. The original window is 720x480 and the port renders
 * 1280x720, so the game is pillarboxed (1080x720, 100 px bars).
 */

#include "FIS2Render.h"
#include "FIS2Scenes.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"

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

void FIS2Render::RenderLevel(const FIS2Game& game) {
    // the platformer levels: the world is 7200x4800 and scrolls with the
    // camera; the static art is the generated scene (the event-positioned
    // sprites come with the gameplay milestone).
    DrawScene(game.GetScreen());
}

} // namespace fnaf
