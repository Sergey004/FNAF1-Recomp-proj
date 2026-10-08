/**
 * FNaF4Render.h: v2.61 — the FNAF4 renderer (title + the night loop).
 * Dump data: frame 1 "titlescreen" (1024x768), frame 3 "level" (1300x768),
 * and the animation frame tables of the view overlays (peek left/right,
 * closet, bed) from JSON/application.json.
 */

#ifndef FNAF4_RENDER_H
#define FNAF4_RENDER_H

#include "Types.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;
class FNaF4Game;

class FNaF4Render {
public:
    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);

    void RenderTitle(f32 time, i32 optionSelected, bool beat5);
    void RenderDisclaimer();                     // frame 0: the red banner (img 962)
    void RenderNightStart(i32 night);
    void RenderNightWin();
    void RenderBedroom(const FNaF4Game& game, f32 time, f32 pan);
    void DrawAttack(const FNaF4Game& game);      // jumpscares + the bite

    // v2.64: the new flow screens
    void RenderGameOver();                       // frame 4
    void RenderGameOver2();                      // frame 8
    void RenderIntro();                          // frames 6/17
    void RenderNightWinDigits(const FNaF4Game& game);       // the 6 AM digits
    void RenderMinigame(const FNaF4Game& game);  // Plushtrap + BB
    void RenderLockbox(const FNaF4Game& game);   // frame 9
    void RenderExtras(const FNaF4Game& game);    // frame 10
    void RenderCutscene(const FNaF4Game& game);  // frame 12
    void RenderEnding(const FNaF4Game& game);    // frame 13

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
