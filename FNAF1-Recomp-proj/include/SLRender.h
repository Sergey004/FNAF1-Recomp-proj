/**
 * SLRender.h: v2.65 — the Sister Location renderer (wave 1). The game
 * window is 1280x720 NATIVE (no pillarbox scaling unlike FNAF2/3/4);
 * rooms are wider and pan ("you are the camera").
 */

#ifndef SL_RENDER_H
#define SL_RENDER_H

#include "Types.h"

namespace fnaf {

class PakLoader;
class SpriteBatch;
class TextRenderer;
class SLGame;

class SLRender {
public:
    void Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text);

    void RenderWarning();
    void RenderTitle(const SLGame& game, f32 time);
    void RenderElevator(const SLGame& game, f32 time);
    void RenderVent(const SLGame& game);
    void RenderHub(const SLGame& game);
    void RenderBaby(const SLGame& game);
    void RenderBallora(const SLGame& game, f32 time);
    void RenderFuntime(const SLGame& game, f32 time);
    void RenderBreaker(const SLGame& game);
    void RenderPS(const SLGame& game);
    void RenderDesk(const SLGame& game);
    void RenderChain(const SLGame& game);
    void RenderExtrasMenu(const SLGame& game);
    void RenderHold(const char* caption);        // wave-2 hold screens
    void RenderWinNight(const SLGame& game);
    void RenderTvShow();
    void RenderGirlVoice();
    void RenderDeath();
    void RenderGameOver();

private:
    void Draw(int handle, float fx, float fy, float fw, float fh, u32 color);
    void DrawWorld(int handle, float wx, float wy, float fw, float fh,
                   float pan, u32 color);

    PakLoader*    m_pak;
    SpriteBatch*  m_batch;
    TextRenderer* m_text;
};

} // namespace fnaf

#endif // SL_RENDER_H
