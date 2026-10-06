/**
 * FNaF3Render.h: v2.61 — the FNAF3 renderer (title + the night loop).
 * Same pattern as FNaF2Render: stateless, fed the game state by the module.
 * Dump data: frame 1 "title" (1024x768), frame 3 "Frame 1" (2000x768).
 */

#ifndef FNAF3_RENDER_H
#define FNAF3_RENDER_H

#include "Types.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;
class FNaF3Game;

class FNaF3Render {
public:
    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);

    void RenderTitle(f32 time, i32 optionSelected);
    void RenderNightStart(i32 night);
    void RenderStatic6();
    void RenderNextDay(i32 night);
    void RenderOffice(const FNaF3Game& game, f32 time);
    void RenderMonitor(const FNaF3Game& game, f32 time);
    void DrawAttack(const FNaF3Game& game);      // the office jumpscares

    // v2.62: the dump's end screens
    void RenderAd();                             // frame 8: COMING SOON newspaper
    void RenderRare2();                          // frame 13: the glitch screen
    void RenderEndScreen(i32 which);             // 0 chooser / 1 bad / 2 good / 3 end2

private:
    void Draw(int handle, float fx, float fy, float fw, float fh, u32 color);
    void DrawWorld(int handle, float wx, float wy, float fw, float fh,
                   float pan, u32 color);
    int  StaticFrame(f32 time) const;
    void RenderMap(const FNaF3Game& game);       // map overlay (screen space)
    void RenderPanel(const FNaF3Game& game);     // maintenance panel

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // FNAF3_RENDER_H
