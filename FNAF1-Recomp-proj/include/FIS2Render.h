/**
 * FIS2Render.h: v2.108 — Freddy in Space 2 module renderer.
 *
 * The art of every screen is dump-derived (FIS2Scenes.h, generated from the
 * FrameLayout files of fis2.exe). The original window is 720x480 (3:2); the
 * port renders 1280x720, so the game is pillarboxed: 1080x720 with 100 px
 * bars on the sides.
 */

#ifndef FNAF_FIS2_RENDER_H
#define FNAF_FIS2_RENDER_H

#include "Types.h"
#include "FIS2Game.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;

class FIS2Render {
public:
    FIS2Render() : m_pak(0), m_batch(0), m_text(0) {}

    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);

    void RenderTitle(const FIS2Game& game);
    void RenderStageSelect(const FIS2Game& game);
    void RenderLevel(const FIS2Game& game);

    void DrawScene(int frameIndex);

private:
    void Draw(int img, float x, float y, float w, float h, u32 color);
    void DrawNat(int img, float x, float y, u32 color);

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // FNAF_FIS2_RENDER_H
