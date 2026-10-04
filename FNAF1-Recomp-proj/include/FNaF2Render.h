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
    void RenderDisclaimer(const FNaF2Game& game);   // frame 0 "Frame 17"
    void RenderTitle(const FNaF2Game& game, f32 time);

    // frame 3 "Frame 1" (1600x768): the panning office (dark unless lit),
    // desk row, wall LIGHT buttons, viewport static, debug-font HUD.
    void RenderOffice(const FNaF2Game& game, f32 time, f32 pan, i32 sceneValue);

    // camera monitor (viewing 1..12): feed from the dump scene selector
    // (Active 16 value), switch static burst, cam strip + HUD; wide feeds
    // pan with `pan`. sceneValue 0 = keep the previous feed (dump behavior).
    void RenderMonitor(const FNaF2Game& game, f32 time, f32 sinceSwitch, f32 pan,
                       i32 sceneValue, i32 lastSceneValue);

    // v2.59: the jumpscare overlay (attack animation 12..21) — draw AFTER
    // the office/monitor, full-screen.
    void DrawAttack(const FNaF2Game& game);

    // ---- v2.62: the rest of the dump's frame flow + the minigames ----
    void RenderAd();                             // frame 8: the newspaper
    void RenderCard(const FNaF2Game& game);      // frame 2: the night card
    void RenderStatic();                         // frame 4
    void RenderNextDay(const FNaF2Game& game);   // frame 5: the 6 AM clock
    void RenderDream(const FNaF2Game& game);     // frame 13
    void RenderError(bool second);               // frames 14/15
    void RenderEnd(i32 which);                   // frames 9/10/11
    void RenderCustomize(const FNaF2Game& game); // frame 12
    void RenderRare(i32 which);                  // frames 16/17/18
    void RenderGameOver();                       // frame 6
    void RenderEightBit(const FNaF2Game& game);  // frame 19: SAVETHEM
    void RenderMgLoad();                         // frame 21
    void RenderMinigame(const FNaF2Game& game);  // frames 23/24/25
    void RenderEndBars();                        // frames 20/22

private:
    // pak texture by handle, FNAF2 frame coords (1024x768 -> 1280x720 full-stretch)
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
