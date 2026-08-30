/**
 * Five Nights at Freddy's 1 — Recompilation
 * GameRender.h: Presentation layer — draws every game screen from fnaf1.pak
 *
 * v2.5: every screen is composed from the REAL pak images with the original
 * object names, instance positions and hotspots (see PakAssets.h):
 *  - Disclaimer   : img 605 (the warning text image)
 *  - Title menu   : logo/buttons/stars/version at hotspot-correct positions
 *  - Night start  : the "12:00 AM / Nth Night" card images (453/454/...)
 *  - Office       : TRUE 1600x720 pan window (left stick), data-exact doors,
 *                   button panels, desk fan + pumpkin, sprite-font HUD
 *                   (AM img, CLOCK digits, Power left:/Usage: labels, bars)
 *  - Camera       : presence-aware feeds, location label images, cam map with
 *                   blinking button, AUDIO ONLY banner, flip-down bar
 *  - 6 AM         : real "5/6/AM" digit images; nights 5/6/7 show the
 *                   paycheck / overtime / termination screens
 *  - Game over    : img 358 backdrop
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

    // Office view panning (original pans a 1280x720 window across the
    // 1600x720 office frame). dir = -1..1 from the left stick X.
    void SetLookDir(f32 dir);

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

    // Debug sprite browser (LB+RB hold on menu/disclaimer): pages through
    // the pak's counter-font strips, label candidates and small sprites so
    // every UI text handle can be identified from one Xenia screenshot.
    void RenderSpriteBrowser(i32 page);
    i32  SpriteBrowserPageCount() const;

    // Sprite-strip text (the game's real counter fonts from the pak)
    void DrawStripText(const struct SpriteStrip& strip, float x, float y,
                       const char* text, u32 color, float scale = 1.0f);
    f32  MeasureStripText(const struct SpriteStrip& strip, const char* text,
                          float scale = 1.0f) const;
    void DrawStripCentered(const struct SpriteStrip& strip, float cx, float y,
                           const char* text, u32 color, float scale = 1.0f);

private:
    PakLoadedTexture* Tex(const char* name);   // cached FindTexture
    void DrawTex(const char* name, float x, float y, float w, float h, u32 color);
    void DrawFrame(int imgHandle, float x, float y, float w, float h, u32 color);
    void StaticFrame(char out[32]);            // current static frame name
    void DrawFrameFit(int imgHandle, float cx, float cy, float maxW, float maxH, u32 color);

    // Hotspot-aware draw: (ix,iy) = Clickteam instance position, i.e. where
    // the image's hotspot lands. If scene==true the office pan offset is
    // subtracted (1600x720 scene -> 1280x720 window).
    void DrawInstance(int imgHandle, float ix, float iy, u32 color, bool scene);

    // Tinted solid rectangle via the all-white pak frame (img 23).
    void DrawSolidRect(float x, float y, float w, float h, u32 color);

    SpriteBatch*      m_batch;
    TextRenderer*     m_text;
    PakLoader*        m_pak;

    f32  m_time;            // total render time
    f32  m_staticTime;      // static cycle clock
    int  m_staticIndex;     // current static frame
    f32  m_lookDir;         // left stick X (-1..1)
    f32  m_panX;            // office pan window offset 0..320

    // tiny cache: last 64 lookups
    struct TexCacheEntry { char name[64]; PakLoadedTexture* tex; };
    TexCacheEntry m_cache[64];
    int  m_cacheCount;
};

} // namespace fnaf

#endif // FNAF_GAME_RENDER_H
