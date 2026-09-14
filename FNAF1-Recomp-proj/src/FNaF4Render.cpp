/**
 * FNaF4Render.cpp: v2.31 — the FNAF4 renderer, test level (title+office).
 * Dump data: frame 1 "titlescreen" placement table + asset_mapping_fnaf4.hpp.
 * Hidden at start (per dump/progress): stars 937, DEMO. The "vertical"
 * decorative strips (img_680 xN) are parked mostly offscreen — skipped.
 */

#include "FNaF4Render.h"
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

void FNaF4Render::RenderTitle(f32 time) {
    if (!m_batch || !m_pak) return;
    (void)time;

    // bg (Backdrop img_626 — the red sky)
    Draw(626, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // the big heading (obj "title", img_658, hotspot-corrected (589,-2))
    Draw(658, 589.0f, -2.0f, 408.0f, 765.0f, 0xFFFFFFFF);

    // menu (hotspot-corrected left/top; sizes from the mapping)
    Draw(730, 442.0f, 379.0f, 158.0f, 17.0f, 0xFFFFFFFF);   // "New Game"
    Draw(737, 442.0f, 425.0f, 151.0f, 17.0f, 0xFFFFFFFF);   // "Continue"
    Draw(738, 441.0f, 470.0f, 161.0f, 17.0f, 0xFFFFFFFF);   // "6th Night"
    Draw(731, 472.0f, 517.0f,  98.0f, 17.0f, 0xFFFFFFFF);   // "Extra"

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

void FNaF4Render::RenderOffice(f32 time, f32 pan) {
    if (!m_batch || !m_pak) return;
    (void)time;

    // the bedroom (1300x768; img_4 is the base room state — the
    // night/closet/bed sub-views come with the FNAF4 logic stage)
    DrawWorld(4, 0.0f, 0.0f, 1300.0f, 768.0f, pan, 0xFFFFFFFF);

    if (m_text) m_text->DrawText(180, 20, "FNAF4 bedroom (test)", 0xFF80FF80);
}

} // namespace fnaf
