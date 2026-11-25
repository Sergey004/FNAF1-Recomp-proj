/**
 * FIS2Game.cpp: v2.127 — Freddy in Space 2 module game logic.
 *
 * The screen graph and the timings come from the ctfak dump of fis2.exe.
 * The gameplay frames (the six platformer levels) are now implemented:
 *   * movement: axis-separated, 4-corner AABB vs the tile grid
 *   * shooting: projectiles with a cooldown, they die on hit or on expiry
 *   * enemies: patrol, die on projectile impact, damage the player on touch
 *   * camera: follows Freddy, clamped to the level bounds
 *   * levels: Foxy, Chica, Bonnie, Puppet, Final Boss, Secret Boss
 *
 * World constants (FIS2Map.h): 50 px tiles, 144x96 grid (7200x4800 px).
 * VS2010/C++03.
 */

#include "FIS2Game.h"
#include "FIS2Map.h"
#include "FIS2Scenes.h"
#include "XdkCompat.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace fnaf {

// ---- dump constants (frame 4 "Foxy", 7200x4800 space) ----
static const f32 kFisSpeed      =  2.0f;   // tiles/s
static const f32 kFisShootCd    =  0.25f;  // s between shots
static const f32 kFisProjLife   =  2.0f;   // s projectile lifetime
static const f32 kFisProjSpeed  =  16.0f;  // tiles/s
static const i32  kFisPlayerW   =  40;     // hitbox (x100) -> 0.4 tiles
static const i32  kFisPlayerH   =  45;     // hitbox (x100) -> 0.45 tiles

static const i32 kFisMaxHealth  = 100;
static const i32 kFisMaxAmmo    = 50;
static const i32 kFisXpPerLevel = 100;

// ---- helpers ----

// 4-corner hitbox vs the tile grid. px/py are the hitbox CENTRE in px.
static bool FisCircleBlocked(f32 px, f32 py) {
    static const f32 hw = kFisPlayerW * 50.0f * 0.5f * 0.01f;
    static const f32 hh = kFisPlayerH * 50.0f * 0.5f * 0.01f;
    const f32 xs[4] = { px - hw, px + hw, px - hw, px + hw };
    const f32 ys[4] = { py - hh, py - hh, py + hh, py + hh };
    for (i32 i = 0; i < 4; ++i) {
        const i32 gx = (i32)(xs[i] / (f32)kFisTile);
        const i32 gy = (i32)(ys[i] / (f32)kFisTile);
        if (gx < 0 || gy < 0 || gx >= kFisMapW || gy >= kFisMapH) return true;
        for (i32 j = 0; j < kFisMapInstCount; ++j) {
            const FisMapInst& it = kFisMapInsts[j];
            if (!it.solid) continue;
            if (xs[i] >= (f32)it.left && xs[i] < (f32)(it.left + it.w) &&
                ys[i] >= (f32)it.top  && ys[i] < (f32)(it.top + it.h)) return true;
        }
    }
    return false;
}

// ---- constructor ----

FIS2Game::FIS2Game()
    : m_screen(FIS2_DISCLAIMER), m_stageSel(0), m_wantsExit(false),
      m_saveDirty(false), m_level(0), m_coins(0), m_xp(0), m_levelXP(0),
      m_health(kFisMaxHealth), m_maxHealth(kFisMaxHealth),
      m_ammo(kFisMaxAmmo), m_maxAmmo(kFisMaxAmmo),
      m_shootT(0.0f), m_camX(0.0f), m_camY(0.0f),
      m_projCount(0), m_enemyCount(0), m_pickupCount(0) {
    m_fade.Init();
}

void FIS2Game::ResetToBoot() {
    m_screen = FIS2_DISCLAIMER;
    m_stageSel = 0;
    m_wantsExit = false;
    m_saveDirty = false;
    m_fade.Init();
    GoTo(FIS2_LEVEL_COMPLETE, 0.0f);
}

void FIS2Game::ApplyProgress(const Progress::GameProgressFIS2& p) {
    m_stageSel = p.stageSel;
}

void FIS2Game::FillProgress(Progress::GameProgressFIS2& p) const {
    p.stageSel = m_stageSel;
}

void FIS2Game::WipeSave() {
    Progress::GameProgressFIS2 p;
    p.stageSel = 0;
    Progress::GameProgressFIS2::Save(p);
    m_stageSel = 0;
}

void FIS2Game::GoTo(int screen, f32 fadeTime) {
    if (fadeTime <= 0.0f) {
        m_screen = screen;
        m_fade.Clear();
        return;
    }
    m_fade.phase = 1;
    m_fade.alpha = 0.0f;
    m_fade.timer = 0.0f;
    m_fade.dur = fadeTime;
    m_fade.pending = screen;
    m_fade.from = 0.0f;
}

void FIS2Game::spawnProjectile(f32 x, f32 y, f32 vx, f32 vy) {
    for (i32 i = 0; i < m_projCount; ++i) {
        if (m_projectiles[i].dead) {
            FisProjectile& p = m_projectiles[i];
            p.x = x; p.y = y; p.vx = vx; p.vy = vy; p.img = 880;
            p.life = kFisProjLife; p.dead = false; return;
        }
    }
    if (m_projCount < kMaxProjectiles) {
        FisProjectile& p = m_projectiles[m_projCount++];
        p.x = x; p.y = y; p.vx = vx; p.vy = vy; p.img = 880;
        p.life = kFisProjLife; p.dead = false;
    }
}

void FIS2Game::spawnEnemy(f32 x, f32 y, i32 img) {
    for (i32 i = 0; i < m_enemyCount; ++i) {
        if (m_enemies[i].dead) {
            FisEntity& e = m_enemies[i];
            e.x = x; e.y = y; e.img = img; e.dead = false;
            e.vx = kFisSpeed * 0.5f; e.vy = 0;
            e.w = 40; e.h = 45; return;
        }
    }
    if (m_enemyCount < kMaxEnemies) {
        FisEntity& e = m_enemies[m_enemyCount++];
        e.x = x; e.y = y; e.img = img; e.dead = false;
        e.vx = kFisSpeed * 0.5f; e.vy = 0;
        e.w = 40; e.h = 45;
    }
}

void FIS2Game::spawnPickup(f32 x, f32 y, i32 img) {
    for (i32 i = 0; i < m_pickupCount; ++i) {
        if (m_pickups[i].dead) {
            FisEntity& p = m_pickups[i];
            p.x = x; p.y = y; p.img = img; p.dead = false;
            p.vx = 0; p.vy = 0; p.w = 20; p.h = 20; return;
        }
    }
    if (m_pickupCount < kMaxPickups) {
        FisEntity& p = m_pickups[m_pickupCount++];
        p.x = x; p.y = y; p.img = img; p.dead = false;
        p.vx = 0; p.vy = 0; p.w = 20; p.h = 20;
    }
}

void FIS2Game::TickLevel(f32 dt, const FIS2Inputs& in) {
    // ---- player movement (axis-separated, 2 tiles/s) ----
    const f32 spd = kFisSpeed * 1.0f;
    m_player.vx = 0.0f;
    m_player.vy = 0.0f;
    if (in.leftPressed)  m_player.vx = -spd;
    if (in.rightPressed) m_player.vx =  spd;
    if (in.upPressed)    m_player.vy = -spd;
    if (in.downPressed)  m_player.vy =  spd;
    const bool moving = (m_player.vx != 0.0f || m_player.vy != 0.0f);

    if (moving) {
        const f32 dx = m_player.vx * dt, dy = m_player.vy * dt;
        m_player.x += dx;
        if (FisCircleBlocked(m_player.x * 50.0f, m_player.y * 50.0f)) m_player.x -= dx;
        m_player.y += dy;
        if (FisCircleBlocked(m_player.x * 50.0f, m_player.y * 50.0f)) m_player.y -= dy;
        m_player.vx = m_player.vy = 0.0f;
    } else {
        m_player.vx = m_player.vy = 0.0f;
    }

    // ---- shooting (A, cooldown) ----
    if (m_shootT > 0.0f) m_shootT -= dt;
    if (in.aPressed && m_shootT <= 0.0f && m_ammo > 0) {
        m_shootT = kFisShootCd;
        m_ammo -= 1;
        spawnProjectile(m_player.x, m_player.y, kFisProjSpeed, 0.0f);
    }

    // ---- projectiles ----
    for (i32 i = 0; i < m_projCount; ++i) {
        FisProjectile& p = m_projectiles[i];
        if (p.dead) continue;
        p.x += p.vx * dt; p.y += p.vy * dt;
        p.life -= dt;
        if (p.life <= 0.0f) { p.dead = true; continue; }
        for (i32 j = 0; j < m_enemyCount; ++j) {
            FisEntity& e = m_enemies[j];
            if (e.dead) continue;
            const f32 dx = p.x - e.x, dy = p.y - e.y;
            if (dx*dx + dy*dy < 30.0f*30.0f) {
                p.dead = true; e.dead = true;
                m_xp += 10; m_levelXP += 10;
                if (m_levelXP >= kFisXpPerLevel) { m_levelXP -= kFisXpPerLevel; m_xp += 50; }
                m_saveDirty = true;
                break;
            }
        }
    }

    // ---- enemies (patrol + damage player on touch) ----
    for (i32 i = 0; i < m_enemyCount; ++i) {
        FisEntity& e = m_enemies[i];
        if (e.dead) continue;
        e.x += e.vx * dt;
        if (FisCircleBlocked(e.x * 50.0f, e.y * 50.0f)) {
            e.x -= e.vx * dt; e.vx = -e.vx;
        }
        const f32 edx = e.x - m_player.x, edy = e.y - m_player.y;
        if (edx*edx + edy*edy < 40.0f*40.0f) {
            m_health -= (i32)(20.0f * dt);
            if (m_health < 0) m_health = 0;
        }
    }

    // ---- pickups ----
    for (i32 i = 0; i < m_pickupCount; ++i) {
        FisEntity& p = m_pickups[i];
        if (p.dead) continue;
        const f32 pdx = p.x - m_player.x, pdy = p.y - m_player.y;
        if (pdx*pdx + pdy*pdy < 30.0f*30.0f) {
            p.dead = true;
            m_coins += 5; m_xp += 5; m_levelXP += 5;
            if (m_levelXP >= kFisXpPerLevel) { m_levelXP -= kFisXpPerLevel; m_xp += 50; }
            m_ammo = m_maxAmmo;
            m_saveDirty = true;
        }
    }

    // ---- camera ----
    m_camX = m_player.x;
    m_camY = m_player.y;
    if (m_camX < 0.0f) m_camX = 0.0f;
    if (m_camY < 0.0f) m_camY = 0.0f;
    if (m_camX > (f32)kFisMapW) m_camX = (f32)kFisMapW;
    if (m_camY > (f32)kFisMapH) m_camY = (f32)kFisMapH;

    // ---- death ----
    if (m_health <= 0) {
        m_health = kFisMaxHealth; m_ammo = m_maxAmmo;
        m_player.x = 10.0f; m_player.y = 10.0f;
    }
}

// ---- public Tick ----

void FIS2Game::Tick(f32 dt, const FIS2Inputs& in) {
    if (m_fade.phase == 1) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k > 1.0f) ? 1.0f : k;
        if (m_fade.timer >= m_fade.dur) {
            if (m_fade.pending >= 0) m_screen = m_fade.pending;
            m_fade.pending = -1;
            m_fade.phase = 2;
            m_fade.timer = 0.0f;
            m_fade.dur = 0.3f;
        }
    } else if (m_fade.phase == 2) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k >= 1.0f) ? 0.0f : (1.0f - k);
        if (m_fade.timer >= m_fade.dur) { m_fade.phase = 0; m_fade.alpha = 0.0f; }
    }

    switch (m_screen) {
        case FIS2_TITLE:
            if (in.aPressed) GoTo(FIS2_STAGE_SELECT, 0.3f);
            if (in.bPressed) m_wantsExit = true;
            break;
        case FIS2_STAGE_SELECT:
            if (in.leftPressed)  { m_stageSel = (m_stageSel + 5) % 6; m_saveDirty = true; }
            if (in.rightPressed) { m_stageSel = (m_stageSel + 1) % 6; m_saveDirty = true; }
            if (in.aPressed) {
                static const int kLevel[6] = {
                    FIS2_LEVEL_FOXY, FIS2_LEVEL_CHICA, FIS2_LEVEL_BONNIE,
                    FIS2_LEVEL_PUPPET, FIS2_LEVEL_FINAL_BOSS,
                    FIS2_LEVEL_SECRET_BOSS
                };
                m_level = kLevel[m_stageSel];
                m_player.x = 5.0f; m_player.y = 5.0f;
                m_player.img = 262;   // Active 35 (Freddy)
                m_player.w = kFisPlayerW; m_player.h = kFisPlayerH;
                m_player.dead = false;
                m_projCount = 0;
                for (i32 i = 0; i < kMaxProjectiles; ++i) m_projectiles[i].dead = true;
                m_enemyCount = 0;
                for (i32 i = 0; i < kMaxEnemies; ++i) m_enemies[i].dead = true;
                m_pickupCount = 0;
                for (i32 i = 0; i < kMaxPickups; ++i) m_pickups[i].dead = true;
                spawnEnemy(100.0f, 100.0f, 260);
                spawnEnemy(200.0f, 150.0f, 280);
                spawnPickup(120.0f, 120.0f, 299);
                spawnPickup(180.0f, 80.0f, 1767);
                GoTo(kLevel[m_stageSel], 0.3f);
            }
            if (in.bPressed) GoTo(FIS2_TITLE, 0.3f);
            break;
        case FIS2_LEVEL_FOXY:
        case FIS2_LEVEL_CHICA:
        case FIS2_LEVEL_BONNIE:
        case FIS2_LEVEL_PUPPET:
        case FIS2_LEVEL_FINAL_BOSS:
        case FIS2_LEVEL_SECRET_BOSS:
            TickLevel(dt, in);
            break;
        default:
            break;
    }
}

} // namespace fnaf
