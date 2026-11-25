/**
 * FIS2Game.h: v2.108 — Freddy in Space 2 module game logic.
 *
 * Clean-room port of the original Freddy in Space 2 flow (the ctfak dump of
 * fis2.exe, build 292). The screen graph is the dump's TRANSITIONS.txt:
 *
 *   frame 14 "disclaimer"   -> "level complete" (the boot order)
 *   frame 1  "intro"        -> "title screen"
 *   frame 2  "title screen" -> "stage select"
 *   frame 3  "stage select" -> the six platformer levels (Foxy, Chica,
 *                               Bonnie, Puppet, Final Boss, Secret Boss),
 *                               "total", "dedicated" or back to the title
 *   each level              -> the next level or the stage select
 *   frame 10 "level complete" / 12 "THE END" / 13 "credits"
 *
 * The original window is 720x480 (3:2); the port renders 1280x720, so the
 * game is pillarboxed (1080x720, 100 px bars).
 *
 * VS2010/C++03: no nullptr, no lambdas, no std::string.
 */

#ifndef FNAF_FIS2_GAME_H
#define FNAF_FIS2_GAME_H

#include "Types.h"
#include "Progress.h"
#include "ScreenFade.h"

namespace fnaf {

struct FIS2Inputs {
    bool aPressed;        // A edge  — confirm / jump
    bool bPressed;        // B edge  — back
    bool upPressed, downPressed, leftPressed, rightPressed;
    bool startPressed;
    FIS2Inputs() : aPressed(false), bPressed(false), upPressed(false),
                   downPressed(false), leftPressed(false), rightPressed(false),
                   startPressed(false) {}
};

struct FIS2AudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    FIS2AudioHooks() : play(0), stop(0), channelVolume(0) {}
};

// the dump's frames (fis2.exe)
enum FIS2Screen {
    FIS2_DEDICATED = 0,
    FIS2_INTRO = 1,
    FIS2_TITLE = 2,
    FIS2_STAGE_SELECT = 3,
    FIS2_LEVEL_FOXY = 4,
    FIS2_LEVEL_CHICA = 5,
    FIS2_LEVEL_BONNIE = 6,
    FIS2_LEVEL_PUPPET = 7,
    FIS2_LEVEL_FINAL_BOSS = 8,
    FIS2_LEVEL_SECRET_BOSS = 9,
    FIS2_LEVEL_COMPLETE = 10,
    FIS2_TOTAL = 11,
    FIS2_THE_END = 12,
    FIS2_CREDITS = 13,
    FIS2_DISCLAIMER = 14,
    FIS2_SCREEN_COUNT
};

// v2.127: gameplay state (side-scrolling shooter/platformer).
struct FisEntity {
    f32  x, y;          // tile coords (centre of hitbox)
    f32  vx, vy;        // tile coords / s
    i32  w, h;          // hitbox size in tiles (x100 for sub-tile precision)
    i32  img;           // pak image handle
    bool dead;
    bool isEnemy;
    FisEntity() : x(0), y(0), vx(0), vy(0), w(0), h(0), img(-1), dead(false), isEnemy(false) {}
};

struct FisProjectile {
    f32  x, y, vx, vy;  // tile coords / s
    i32  img;
    f32  life;          // seconds remaining
    bool dead;
    FisProjectile() : x(0), y(0), vx(0), vy(0), img(-1), life(0), dead(false) {}
};

class FIS2Game {
public:
    FIS2Game();

    FIS2AudioHooks audio;

    void ResetToBoot();

    void Tick(f32 dt, const FIS2Inputs& in);

    int  GetScreen() const { return m_screen; }
    int  GetStageSel() const { return m_stageSel; }
    f32  GetFadeAlpha() const { return m_fade.alpha; }
    bool WantsExit() const { return m_wantsExit; }

    bool IsSaveDirty() const { return m_saveDirty; }
    void ClearSaveDirty() { m_saveDirty = false; }
    void ApplyProgress(const Progress::GameProgressFIS2& p);
    void FillProgress(Progress::GameProgressFIS2& p) const;   // v2.127
    void WipeSave();                                           // v2.127

    // ---- world state ----
    i32  GetLevel() const { return m_level; }      // 0..5 (Foxy..Secret Boss)
    i32  GetCoins() const { return m_coins; }
    i32  GetXP() const { return m_xp; }
    i32  GetLevelXP() const { return m_levelXP; }  // xp within current level
    i32  GetHealth() const { return m_health; }
    i32  GetMaxHealth() const { return m_maxHealth; }
    i32  GetAmmo() const { return m_ammo; }
    i32  GetMaxAmmo() const { return m_maxAmmo; }

    // ---- camera (tile coords, centre of view) ----
    f32  GetCamX() const { return m_camX; }
    f32  GetCamY() const { return m_camY; }
    f32  GetCamZoom() const { return 1.0f; }       // 1:1, level frame 7200x4800

    // ---- entity access for the renderer ----
    const FisEntity& GetPlayer() const { return m_player; }
    i32  GetProjectileCount() const { return m_projCount; }
    const FisProjectile& GetProjectile(i32 i) const { return m_projectiles[i]; }
    i32  GetEnemyCount() const { return m_enemyCount; }
    const FisEntity& GetEnemy(i32 i) const { return m_enemies[i]; }
    i32  GetPickupCount() const { return m_pickupCount; }
    const FisEntity& GetPickup(i32 i) const { return m_pickups[i]; }

private:
    void GoTo(int screen, f32 fadeTime);
    void TickLevel(f32 dt, const FIS2Inputs& in);   // v2.127: gameplay
    void spawnProjectile(f32 x, f32 y, f32 vx, f32 vy);
    void spawnEnemy(f32 x, f32 y, i32 img);
    void spawnPickup(f32 x, f32 y, i32 img);

    int   m_screen;
    int   m_stageSel;     // the stage-select cursor (the dump's counter)
    bool  m_wantsExit;
    bool  m_saveDirty;
    ScreenFade m_fade;

    // ---- gameplay (v2.127) ----
    i32   m_level;           // current level index (0..5)
    i32   m_coins;           // collected coins
    i32   m_xp;              // total xp
    i32   m_levelXP;         // xp within current level (0..99)
    i32   m_health, m_maxHealth;
    i32   m_ammo, m_maxAmmo;
    f32   m_shootT;          // shoot cooldown clock
    f32   m_camX, m_camY;    // camera centre, tile coords

    FisEntity  m_player;     // the player (Freddy)
    static const int kMaxProjectiles = 32;
    FisProjectile m_projectiles[kMaxProjectiles];
    i32   m_projCount;
    static const int kMaxEnemies = 16;
    FisEntity   m_enemies[kMaxEnemies];
    i32   m_enemyCount;
    static const int kMaxPickups = 32;
    FisEntity   m_pickups[kMaxPickups];
    i32   m_pickupCount;
    static const i32 kFisXpPerLevel = 100;
};

} // namespace fnaf

#endif // FNAF_FIS2_GAME_H
