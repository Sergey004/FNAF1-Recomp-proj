/**
 * Five Nights at Freddy's 1 — Recompilation
 * GameRender.h: Presentation layer — draws every game screen from fnaf1.pak
 *
 * All texture handles below were extracted from the original game data with
 * ctfak-cpp (Frame Layout Renderer + JSON export + Events Listing):
 *  - Title frame "title" (1280x720): object table with exact positions
 *  - Office frame "Frame 1" (1600x720): rendered at 0.8 scale
 *  - Active 3 (obj 44) animations 51/52/65 = power-out / Foxy / Freddy scares
 *  - Menu static = object "static" animation frames [18,20,12..17]
 */

#ifndef FNAF_GAME_RENDER_H
#define FNAF_GAME_RENDER_H

#include "Types.h"

namespace fnaf {

class SpriteBatch;
class TextRenderer;
class PakLoader;
struct PakLoadedTexture;   // defined as struct in PakLoader.h (C4099)
class MenuSystem;
class Game;
class AudioSystem;

class GameRender {
public:
    GameRender();

    void Init(SpriteBatch* batch, TextRenderer* text, PakLoader* pak);

    // Advance internal animation clocks (static noise, blink timers)
    void Tick(f32 dt);

    // --- Screens ---

    // "WARNING! This game contains flashing lights..." — the very first
    // screen (title frame, String obj 0, shown centered by boot events).
    void RenderDisclaimer(bool blinkOn);

    // Full title menu, drawn with the original button art and static overlay
    void RenderTitle(const MenuSystem& menu, bool hasSave, i32 stars);

    // "Night N / 12 AM" title card (frame "wait" reconstruction)
    void RenderNightStart(i32 night);

    // Office view: background, doors, button panels, mute-call, HUD
    void RenderOffice(const Game& game, bool phonePlaying);

    // Camera monitor: screen frame, room feed, static, map, cam label
    void RenderCamera(const Game& game);

    // Power out: dark office + flickering Freddy sequence
    void RenderPowerOut(const Game& game);

    // Full-screen jump scare sequence per animatronic (real anim frames)
    void RenderJumpscare(AnimatronicId anim, f32 elapsed);

    // 6 AM win screen
    void RenderNightComplete(i32 night);

    // Static + GAME OVER
    void RenderGameOver();

    // Draw the fullscreen static overlay on top of anything
    void DrawStaticOverlay(float alpha);

    // v2.8.0: fullscreen CRT post-pass (scanlines + vignette + fine grain),
    // drawn on top of EVERYTHING incl. menu text (authentic TV look).
    // Strengths 0..1; see docs/CRT_EFFECT.md for the per-screen values.
    void DrawCRTOverlay(f32 scan, f32 vignette, f32 grain);

private:
    PakLoadedTexture* Tex(const char* name);   // cached FindTexture
    void DrawTex(const char* name, float x, float y, float w, float h, u32 color);
    void DrawFrame(int imgHandle, float x, float y, float w, float h, u32 color);
    void StaticFrame(char out[32]);            // current static frame name

    SpriteBatch*      m_batch;
    TextRenderer*     m_text;
    PakLoader*        m_pak;

    f32  m_time;            // total render time
    f32  m_staticTime;      // static cycle clock
    int  m_staticIndex;     // current static frame

    // tiny cache: last 8 lookups
    struct TexCacheEntry { char name[64]; PakLoadedTexture* tex; };
    TexCacheEntry m_cache[8];
    int  m_cacheCount;
};

} // namespace fnaf

#endif // FNAF_GAME_RENDER_H
