/**
 * FuryRageRender.h: v2.108 — Fury's Rage module renderer.
 *
 * The art of every screen is dump-derived (FuryRageScenes.h, generated from
 * the FrameLayout files of furys-rage.exe). The original window is
 * 1920x1080; the port renders 1280x720, so the whole world scales by
 * 2/3 (the same convention as the other 4:3 titles).
 */

#ifndef FNAF_FURYRAGE_RENDER_H
#define FNAF_FURYRAGE_RENDER_H

#include "Types.h"
#include "FuryRageGame.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;

class FuryRageRender {
public:
    FuryRageRender() : m_pak(0), m_batch(0), m_text(0) {}

    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);

    void RenderTitle(const FuryRageGame& game);
    void RenderCharacterSelect(const FuryRageGame& game);
    void RenderLevels(const FuryRageGame& game);

    // the dump's frame art (the generated scene table)
    void DrawScene(int frameIndex);

private:
    void Draw(int img, float x, float y, float w, float h, u32 color);
    void DrawNat(int img, float x, float y, u32 color);

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // FNAF_FURYRAGE_RENDER_H
