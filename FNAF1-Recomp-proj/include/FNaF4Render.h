/**
 * FNaF4Render.h: v2.31 — the FNAF4 renderer (test level: title + office).
 * Dump data: frame 1 "titlescreen" (1024x768), frame 3 "level" (1300x768).
 */

#ifndef FNAF4_RENDER_H
#define FNAF4_RENDER_H

#include "Types.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;

class FNaF4Render {
public:
    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);
    void RenderTitle(f32 time);
    void RenderOffice(f32 time, f32 pan);   // pan 0..276 (1300-wide world)

private:
    void Draw(int handle, float fx, float fy, float fw, float fh, u32 color);
    void DrawWorld(int handle, float wx, float wy, float fw, float fh,
                   float pan, u32 color);

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // FNAF4_RENDER_H
