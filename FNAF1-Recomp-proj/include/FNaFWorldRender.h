/**
 * FNaFWorldRender.h: v2.90 — FNAF World module renderer.
 *
 * Stateless like FNaF2Render/SLRender: receives const FNaFWorldGame& and
 * draws. Same core trio (SpriteBatch + PakLoader + TextRenderer); textures
 * are addressed by pak index ("img_N", the RecompPack naming) through the
 * SLRender D()/DN() helpers (UV clamp to the ORIGINAL size — pak padding
 * decodes to junk). The dump frames are 800x480; every coordinate scales
 * x*1.6 / y*1.5 to the 1280x720 screen (full-stretch, the F2 convention).
 *
 * VS2010/C++03.
 */

#ifndef FNAF_FNAFWORLD_RENDER_H
#define FNAF_FNAFWORLD_RENDER_H

#include "Types.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;
class FNaFWorldGame;

class FNaFWorldRender {
public:
    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);

    // frame 0 "Frame 32": the splash art centered, 3 s hold
    void RenderSplash();

    // frame 2 "title screen": bg 4573, logo 4597 slide-in, party 4574
    // slide-up, START 4569 slide-in, sparkles 1207, Demo/version texts
    void RenderTitle(const FNaFWorldGame& game, f32 time);

    // v2.91: exe frame 3 "file setup" (3 stages) + frame 6 "game over"
    void RenderFileSetup(const FNaFWorldGame& game);
    void RenderGameOver();
    // v2.91 WIP: the world menus (frames 4/7/12; text panels for now)
    void RenderMenu(const FNaFWorldGame& game);

    // frame 5 "overworld" — the exe 6000x4000 map, painter's order
    void RenderWorld(const FNaFWorldGame& game);

    // SCR_BATTLE placeholder (the step-3 Battle_Rework port replaces it)
    void RenderBattle(const FNaFWorldGame& game);

    // the frame transition overlay (LAST, outside any capture)
    void DrawFade(f32 alpha);

private:
    // SLRender's draw helpers verbatim (index -> "img_N" -> UV-clamped quad)
    void D(int handle, f32 x, f32 y, f32 w, f32 h, u32 color);
    void DN(int handle, f32 x, f32 y, u32 color);
    // native dims scaled by the 800x480 -> 1280x720 factor, at a dump position
    void DNS(int handle, f32 dx, f32 dy, u32 color);
    // point-sampled D — tiny bright details stay dots, not DXT squares
    void DP(int handle, f32 x, f32 y, f32 w, f32 h, u32 color);
    // native draw centred on a dump-space point (the file-setup menu)
    void DrawCentre(int img, f32 dcx, f32 dcy, u32 color);
    // frame 2 g7-11: the title sparkles (renderer-local cosmetic particles)
    void TickSparkles(f32 time);
    void RenderSparkles(f32 time);

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // FNAF_FNAFWORLD_RENDER_H