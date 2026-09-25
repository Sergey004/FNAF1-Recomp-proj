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
#include "CfAnimTimer.h"

namespace fnaf {

class SpriteBatch;
class TextRenderer;
class PakLoader;
struct PakLoadedTexture;   // defined as struct in PakLoader.h (C4099)
class MenuSystem;
class Game;
class AudioSystem;
class Achievements;

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

    // v2.7.13: "HELP WANTED" newspaper (frame "ad", img_574 full screen).
    // New Game intro; any button skips, ~8 s timeout in main.
    void RenderIntroAd();

    // Full title menu, drawn with the original button art and static overlay
    void RenderTitle(const MenuSystem& menu, bool hasSave, i32 stars);

    // "Night N / 12 AM" title card (frame "wait" reconstruction)
    void RenderNightStart(i32 night);

    // Office view: background, doors, button panels, mute-call, HUD
    void RenderOffice(const Game& game, bool phonePlaying);

    // Camera monitor: screen frame, room feed, static, map, cam label
    void RenderCamera(const Game& game, bool phonePlaying);

    // Power out: dark office + flickering Freddy sequence
    void RenderPowerOut(const Game& game);

    // Full-screen jump scare sequence per animatronic (real anim frames)
    void RenderJumpscare(AnimatronicId anim, f32 elapsed);

    // 6 AM win screen (elapsed >= 0 enables the data "6"-roll animation
    // for nights 1-4; elapsed < 0 keeps the static v2.7.12 look)
    void RenderNightComplete(i32 night, f32 elapsed = -1.0f);

    // Static + GAME OVER
    void RenderGameOver();

    // v2.46: Night 7 setup screen (frame "customize"): four AI counters
    // 0-20 with +/- arrows, START; levels[4] = freddy/bonnie/chica/foxy,
    // sel = the console cursor row (0..3, -1 = none).
    void RenderCustomize(const i32 levels[4], i32 sel);

    // Draw the fullscreen static overlay on top of anything
    void DrawStaticOverlay(float alpha);

    // Full-screen black fade overlay (0..1 opacity), for frame transitions.
    void DrawFade(float alpha);

    // v2.14: in-game achievements — full list screen (called from the title
    // menu) and the transient "Achievement Unlocked" toast overlay.
    void RenderAchievements(const Achievements& a);
    void DrawAchievementToast(const char* name, int gamerscore);

    // v2.17: DEBUG/DEV menu (god mode, night jump, 6AM/power-out/jumpscare
    // triggers, sound test, achievement tools, console toggle, 17-frame list).
    void RenderDevMenu(int sel, int night, const char* animName,
                       const char* soundLabel, bool god, bool console, bool analogDoor, bool holdLights);
    // v2.17: full-screen scare flash (Golden Freddy / door window poses)
    void RenderScareFlash(int imgHandle, float elapsed);
    // v2.17: full-screen "IT'S ME" hallucination flicker (obj "Active 21")
    void RenderItsmeFlash(float elapsed);

    // v2.22: Golden Freddy ("yellow bear") support
    int  GetGoldenRoll() const;            // this session's 1/100 poster roll (-1 unrolled)
    void SetGoldenFreddyInOffice(bool on); // slumped Golden Freddy (img 573) in the office
    // v2.53 (dump g43): while the ARM is live, CAM 2B always shows the
    // Golden poster. Fed by main's golden state machine.
    void SetGoldenPosterArmed(bool on)    { m_goldenPosterArmed = on; }

    // Debug sprite browser (LB+RB hold on menu/disclaimer): pages through
    // the pak's counter-font strips, label candidates and small sprites so
    // every UI text handle can be identified from one Xenia screenshot.
    void RenderSpriteBrowser(i32 page);
    i32  SpriteBrowserPageCount() const;

    // --- v2.8 PERSPECTIVE TUNER --------------------------------------
    // Live-adjusts the clean-room PANORAMA parabola shader (RPanorama.fx).
    // Knobs: 0 = ZOOM (edge vertical squeeze px; + bulge / - pincushion /
    // 0 flat), 1 = CENTER_Y (vertical pivot of the bend), 2 = CURVE (the
    // parabola coefficient that replaces the serialized literal 4.0; 0 flat,
    // >4 sharper edges). Defaults are the exact serialized values
    // zoom=300/center=355/curve=4.0, so the game renders the original look
    // until L3+R3 in game touches them. On exit main.cpp prints "PERSP
    // FINAL" to the log + debug console for baking.
    enum { PERSP_TUNER_KNOBS = 3 };
    void PerspTunerAdjust(i32 knob, i32 dir, bool fast);
    void PerspTunerReset(i32 knob);          // knob < 0 resets all three
    f32  PerspTunerValue(i32 knob) const;
    f32  PerspTunerDefault(i32 knob) const;
    void RenderPerspTuner(i32 sel);          // HUD overlay (top-left)

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

    // v2.8 clean-room Perspective: the SCENE (layer 0) is captured into a
    // 1280x720 render target and re-projected through the parabola panorama
    // shader (SpriteBatch::BeginSceneCapture/EndSceneCapture/DrawPerspective),
    // mirroring the original HWA extension's readFrameToTexture + shader.
    // (frameX/frameY = Clickteam instance position, hotspot applied here.)

    // Tinted solid rectangle via the all-white pak frame (img 23).
    void DrawSolidRect(float x, float y, float w, float h, u32 color);

    // v2.7.9: layer-3 HUD shared by the office and the monitor
    // (clock, night, power, usage, mute call -- the monitor toggle
    // groups 81/82 never hide it in the frame data).
    void DrawSharedHud(const Game& game, bool phonePlaying);

    SpriteBatch*      m_batch;
    TextRenderer*     m_text;
    PakLoader*        m_pak;

    f32  m_time;            // total render time
    CfAnimTimer m_static;   // static noise cycle (loop, 8 frames)
    f32  m_lookDir;         // left stick X (-1..1)
    f32  m_panX;            // office pan window offset 0..320

    // tiny cache: last 64 lookups
    struct TexCacheEntry { char name[64]; PakLoadedTexture* tex; };
    TexCacheEntry m_cache[64];
    int  m_cacheCount;

    // v2.7.8 office FX state: door slide anims (obj 59/60 anims a12/a14),
    // tablet-open white flash (obj 46), tablet-close dark wipe (obj 73),
    // monitor/door edge detection. Evidence: docs/OFFICE_FX.md.
    bool m_prevMonitor;
    CfAnimTimer m_flash;    // tablet-open white flash (obj 46, 9 frames)
    CfAnimTimer m_wipe;     // tablet-close dark wipe (obj 73, 11 frames)
    CfAnimTimer m_raise;    // v2.7.9: tablet raise (obj 68, 11 frames)
    int  m_prevCam;       // v2.7.9: cam of the last settled monitor frame (-1 none)
    int  m_goldenRoll;    // v2.17: "random for pic" rolled on each monitor drop
    bool m_goldFredInOffice; // v2.22: Golden Freddy slumped in the office
    bool m_goldenPosterArmed; // v2.53: kill arm live -> 2B always shows him
    f32  m_lastT;};

} // namespace fnaf

#endif // FNAF_GAME_RENDER_H
