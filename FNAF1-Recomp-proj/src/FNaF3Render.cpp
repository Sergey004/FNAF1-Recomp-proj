/**
 * FNaF3Render.cpp: v2.31 — the FNAF3 renderer, test level (title+office).
 * Dump data: frame 1 "title" placement table + asset_mapping_fnaf3.hpp;
 * the title static cycle [37,620,33,34,35,36]@99 from the JSON.
 */

#include "FNaF3Render.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <cstdio>

namespace fnaf {

static const f32 kScale = 720.0f / 768.0f;
static const f32 kOffX  = (1280.0f - 1024.0f * kScale) * 0.5f;   // 160

void FNaF3Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

void FNaF3Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, kOffX + fx * kScale, fy * kScale,
                  fw * kScale, fh * kScale, color);
}

void FNaF3Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = kOffX + (wx - pan) * kScale;
    if (sx >= 1280.0f || sx + fw * kScale <= kOffX) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, sx, wy * kScale, fw * kScale, fh * kScale, color);
}

int FNaF3Render::StaticFrame(f32 time) const {
    static const int kFrames[6] = { 37, 620, 33, 34, 35, 36 };
    const f32 period = 100.0f / 99.0f / 60.0f;
    return kFrames[(int)(time / period) % 6];
}

void FNaF3Render::RenderTitle(f32 time) {
    if (!m_batch || !m_pak) return;

    // The FNAF3 title at frame start is FULL STATIC (the composite is
    // covered; the menu flickers through it at runtime). Approximation:
    // every ~2.5 s the menu shows through for ~0.18 s (DEVIATION until
    // the flicker groups are pinned).
    const f32 period = 2.5f;
    const f32 phase  = time - period * (f32)(int)(time / period);
    const bool menuFlash = phase < 0.18f;

    if (menuFlash) {
        // bg (obj "Active 2", img_862) + the menu (sizes from the mapping)
        Draw(862, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(592,  97.0f, 428.0f, 215.0f,  49.0f, 0xFFFFFFFF);  // "new game"
        Draw(301,  97.0f, 500.0f, 243.0f,  49.0f, 0xFFFFFFFF);  // "continue word"
        Draw(625,  97.0f, 572.0f, 257.0f,  49.0f, 0xFFFFFFFF);  // "6th night"
        Draw(826,  96.0f, 641.0f, 145.0f,  49.0f, 0xFFFFFFFF);  // "custom night"
        Draw(833,  38.0f, 501.0f,  34.0f,  49.0f, 0xFFFFFFFF);  // selector
    }
    // static over everything (obj "static" @ (0,0), layer 2)
    Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF3Render::RenderOffice(f32 time, f32 pan) {
    if (!m_batch || !m_pak) return;

    // 2000x768 room; the three 2000-wide images (203/204/205) are the
    // office light states — base state 203 (variants come with the
    // vent/light logic stage).
    DrawWorld(203, 0.0f, 0.0f, 2000.0f, 768.0f, pan, 0xFFFFFFFF);

    // office static
    Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    if (m_text) m_text->DrawText(180, 20, "FNAF3 office (test)", 0xFF80FF80);
}

} // namespace fnaf
