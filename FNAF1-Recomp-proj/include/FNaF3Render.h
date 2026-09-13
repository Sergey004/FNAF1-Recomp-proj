/**
 * FNaF3Render.h: v2.31 — the FNAF3 renderer (test level: title + office).
 * Same pattern as FNaF2Render: stateless, fed clocks/pan by the module.
 * Dump data: frame 1 "title" (1024x768), frame 3 "Frame 1" (2000x768).
 */

#ifndef FNAF3_RENDER_H
#define FNAF3_RENDER_H

#include "Types.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;

class FNaF3Render {
public:
    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);
    void RenderTitle(f32 time);
    void RenderOffice(f32 time, f32 pan);   // pan 0..976 (2000-wide world)

private:
    void Draw(int handle, float fx, float fy, float fw, float fh, u32 color);
    void DrawWorld(int handle, float wx, float wy, float fw, float fh,
                   float pan, u32 color);
    int  StaticFrame(f32 time) const;

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // FNAF3_RENDER_H
