/**
 * FNaF2Render.h: v2.31 — the FNAF2 renderer, SEPARATE from GameRender.
 *
 * GameRender stays FNAF1-only (its panorama warp, HUD images and hotspot
 * tables are fnaf1.pak-specific). Each game module owns a renderer of its
 * own; both sit on the same core trio (SpriteBatch + PakLoader +
 * TextRenderer) handed over via AppServices — the pak is just the shared
 * texture warehouse, the renderer decides what and where to draw.
 *
 * The renderer is STATELESS about the game: it receives `const FNaF2Game&`
 * plus the module's clocks (anim time, pan, switch timer) and draws. All
 * coordinates come from the dump (frame layouts + placement tables), the
 * sizes from include/assets/asset_mapping_fnaf2.hpp.
 *
 * VS2010/C++03.
 */

#ifndef FNAF2_RENDER_H
#define FNAF2_RENDER_H

#include "Types.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;
class FNaF2Game;

class FNaF2Render {
public:
    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);

    // frame 1 "title" (1024x768): bg, static cycle + Freddy glitch, logo,
    // menu (new game / selector / custom night). `time` drives the cycles.
    void RenderTitle(const FNaF2Game& game, f32 time);

    // frame 3 "Frame 1" (1600x768): the panning office (dark unless lit),
    // desk row, wall LIGHT buttons, viewport static, debug-font HUD.
    void RenderOffice(const FNaF2Game& game, f32 time, f32 pan);

    // camera monitor (viewing 1..12): feed from the dump table, switch
    // static burst, cam strip + HUD.
    void RenderMonitor(const FNaF2Game& game, f32 time, f32 sinceSwitch);

private:
    // pak texture by handle, FNAF2 frame coords (pillarbox 960x720)
    void Draw(int handle, float fx, float fy, float fw, float fh, u32 color);
    // world-space draw for the panning office (wx 0..1600, pan 0..576)
    void DrawWorld(int handle, float wx, float wy, float fw, float fh,
                   float pan, u32 color);
    int  StaticFrame(f32 time) const;   // the shared 6-frame cycle

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // FNAF2_RENDER_H
