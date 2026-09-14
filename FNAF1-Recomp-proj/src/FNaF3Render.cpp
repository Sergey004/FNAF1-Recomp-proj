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

static const f32 kScaleX = 1280.0f / 1024.0f;   // full-stretch 16:9
static const f32 kScaleY = 720.0f / 768.0f;

void FNaF3Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

void FNaF3Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  fw * kScaleX, fh * kScaleY, color);
}

void FNaF3Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = (wx - pan) * kScaleX;
    if (sx >= 1280.0f || sx + fw * kScaleX <= 0.0f) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, sx, wy * kScaleY, fw * kScaleX, fh * kScaleY, color);
}

int FNaF3Render::StaticFrame(f32 time) const {
    static const int kFrames[6] = { 37, 620, 33, 34, 35, 36 };
    const f32 period = 100.0f / 99.0f / 60.0f;
    return kFrames[(int)(time / period) % 6];
}

void FNaF3Render::RenderTitle(f32 time) {
    if (!m_batch || !m_pak) return;

    // The scene is ALWAYS visible (bg img_862 + menu); the static appears
    // only as short glitch bursts (the dump has a burst timer on the
    // static object, groups 72/73 — roll every 40 s, ticks down; here a
    // ~0.1 s burst per 1.6 s slot approximates the flicker).
    Draw(862, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(592,  97.0f, 428.0f, 215.0f,  49.0f, 0xFFFFFFFF);  // "new game"
    Draw(301,  97.0f, 500.0f, 243.0f,  49.0f, 0xFFFFFFFF);  // "load game"
    Draw(625,  97.0f, 572.0f, 257.0f,  49.0f, 0xFFFFFFFF);  // "nightmare"
    Draw(826,  96.0f, 641.0f, 145.0f,  49.0f, 0xFFFFFFFF);  // "extra"
    Draw(833,  38.0f, 501.0f,  34.0f,  49.0f, 0xFFFFFFFF);  // selector

    {
        const f32 kPeriod = 1.6f;
        const int slot = (int)(time / kPeriod);
        u32 h = (u32)slot * 2654435761u + 3u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        if ((h % 100u) < 8u)                       // ~8% of slots flash
            Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    }
}

void FNaF3Render::RenderOffice(f32 time, f32 pan) {
    if (!m_batch || !m_pak) return;

    // 2000x768 room; the three 2000-wide images (203/204/205) are the
    // office light states — base state 203 (variants come with the
    // vent/light logic stage).
    DrawWorld(203, 0.0f, 0.0f, 2000.0f, 768.0f, pan, 0xFFFFFFFF);

    // office static: short glitch bursts, not a constant cover
    {
        const f32 kPeriod = 1.4f;
        const int slot = (int)(time / kPeriod);
        u32 h = (u32)slot * 2654435761u + 5u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        if ((h % 100u) < 10u)
            Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    }

    if (m_text) m_text->DrawText(180, 20, "FNAF3 office (test)", 0xFF80FF80);
}

} // namespace fnaf
