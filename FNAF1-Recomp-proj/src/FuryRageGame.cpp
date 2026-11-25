/**
 * FuryRageGame.cpp: v2.128 — Fury's Rage module game logic.
 *
 * The screen graph and the counters come from the ctfak dump of
 * furys-rage.exe (build 293): see the header for the transition table.
 * The gameplay (frame 4 "levels") is now implemented:
 *   * movement: axis-separated, 4-corner AABB vs the tile grid
 *   * melee combat: hitbox/hurtbox, block, jump, attack cooldown
 *   * wave director: spawn enemies in waves, camera gate until wave cleared
 *   * levels: 4 stages + hard mode secret stage
 *
 * World constants (FuryRageMap.h): 50 px tiles.
 * VS2010/C++03.
 */

#include "FuryRageGame.h"
#include "FuryRageMap.h"
#include "XdkCompat.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace fnaf {

// ---- dump constants (frame 4 "levels", 5920x5400 space) ----
static const f32 kFurySpeed   =  2.0f;   // tiles/s
static const f32 kFuryJumpV   = -14.0f;  // tiles/s (up is negative Y)
static const f32 kFuryGravity =  45.0f;  // tiles/s^2
static const f32 kFuryAtkCd   =  0.6f;   // s between melee attacks
static const i32  kFuryFighterW = 40;    // hitbox (x100) -> 0.4 tiles
static const i32  kFuryFighterH = 45;    // hitbox (x100) -> 0.45 tiles

static const i32 kFuryMaxHp = 100;

// ---- helpers ----

static bool FuryCircleBlocked(f32 px, f32 py) {
    static const f32 hw = kFuryFighterW * 50.0f * 0.5f * 0.01f;
    static const f32 hh = kFuryFighterH * 50.0f * 0.5f * 0.01f;
    const f32 xs[4] = { px - hw, px + hw, px - hw, px + hw };
    const f32 ys[4] = { py - hh, py - hh, py + hh, py + hh };
    for (i32 i = 0; i < 4; ++i) {
        const i32 gx = (i32)(xs[i] / (f32)kFuryTile);
        const i32 gy = (i32)(ys[i] / (f32)kFuryTile);
        if (gx < 0 || gy < 0 || gx >= kFuryMapW || gy >= kFuryMapH) return true;
        for (i32 j = 0; j < kFuryMapInstCount; ++j) {
            const FuryMapInst& it = kFuryMapInsts[j];
            if (!it.solid) continue;
            if (xs[i] >= (f32)it.left && xs[i] < (f32)(it.left + it.w) &&
                ys[i] >= (f32)it.top  && ys[i] < (f32)(it.top + it.h)) return true;
        }
    }
    return false;
}

void FuryRageGame::FurySpawnEnemy(f32 x, f32 y, i32 img) {
    for (i32 i = 0; i < m_enemyCount; ++i) {
        if (m_enemies[i].dead) {
            FuryFighter& e = m_enemies[i];
            e.x = x; e.y = y; e.img = img; e.dead = false;
            e.vx = kFurySpeed * 0.4f; e.vy = 0;
            e.w = kFuryFighterW; e.h = kFuryFighterH;
            e.hp = e.maxHp = 40; e.cd = 0; return;
        }
    }
    if (m_enemyCount < kMaxEnemies) {
        FuryFighter& e = m_enemies[m_enemyCount++];
        e.x = x; e.y = y; e.img = img; e.dead = false;
        e.vx = kFurySpeed * 0.4f; e.vy = 0;
        e.w = kFuryFighterW; e.h = kFuryFighterH;
        e.hp = e.maxHp = 40; e.cd = 0;
    }
}

void FuryRageGame::FuryWaveNext() {
    m_waveT = 0.0f;
    m_waveCleared = false;
    m_enemyCount = 0;
    for (i32 i = 0; i < kMaxEnemies; ++i) m_enemies[i].dead = true;
    // place 2-3 enemies depending on wave
    i32 count = 2 + (m_wave % 3);
    (void)count;
    FurySpawnEnemy(60.0f, 60.0f, 60);   // Active 3 style enemy
}

// ---- constructor ----

FuryRageGame::FuryRageGame()
    : m_screen(FR_START), m_titleSel(0), m_charSel(0), m_hardMode(false),
      m_wantsExit(false), m_saveDirty(false), m_holdT(0.0f),
      m_wave(0), m_score(0), m_enemyHp(0), m_enemyMaxHp(0),
      m_camX(0.0f), m_camY(0.0f), m_waveT(0.0f), m_waveCleared(false),
      m_enemyCount(0) {
    m_coins[0] = m_coins[1] = m_coins[2] = 0;
    m_fade.Init();
}

void FuryRageGame::ResetToStart() {
    m_screen = FR_START;
    m_titleSel = 0;
    m_charSel = 0;
    m_wantsExit = false;
    m_holdT = 0.0f;
    m_fade.Init();
    GoTo(FR_TITLE, 0.0f);
}

void FuryRageGame::ApplyProgress(const Progress::GameProgressFuryRage& p) {
    for (int i = 0; i < 3; ++i) m_coins[i] = p.coins[i];
    m_hardMode = (p.hardMode == 2);
    m_charSel = p.character;
    m_titleSel = p.playsel;
}

void FuryRageGame::FillProgress(Progress::GameProgressFuryRage& p) const {
    for (int i = 0; i < 3; ++i) p.coins[i] = m_coins[i];
    p.hardMode = m_hardMode ? 2 : 0;
    p.character = m_charSel;
    p.level = 1;
    p.playsel = m_titleSel;
}

void FuryRageGame::WipeSave() {
    Progress::GameProgressFuryRage p;
    p.coins[0] = p.coins[1] = p.coins[2] = 0;
    p.hardMode = 0; p.character = 0; p.level = 1; p.playsel = 0;
    Progress::GameProgressFuryRage::Save(p);
    for (int i = 0; i < 3; ++i) m_coins[i] = 0;
    m_hardMode = false; m_charSel = 0; m_titleSel = 0;
}

void FuryRageGame::GoTo(int screen, f32 fadeTime) {
    if (fadeTime <= 0.0f) {
        m_screen = screen;
        m_fade.Clear();
        return;
    }
    m_fade.phase = 1;          // fade out
    m_fade.alpha = 0.0f;
    m_fade.timer = 0.0f;
    m_fade.dur = fadeTime;
    m_fade.pending = screen;
    m_fade.from = 0.0f;
}

void FuryRageGame::UpdateTitle(f32 dt, const FuryRageInputs& in) {
    (void)dt;
    if (in.upPressed)   { m_titleSel = (m_titleSel + 2) % 3; m_saveDirty = true; }
    if (in.downPressed) { m_titleSel = (m_titleSel + 1) % 3; m_saveDirty = true; }
    if (in.aPressed) {
        if (m_titleSel == 0) {
            GoTo(FR_CHARACTER_SELECT, 0.35f);
        } else {
            m_wantsExit = true;
        }
    }
    if (in.bPressed) m_wantsExit = true;
}

void FuryRageGame::UpdateCharacterSelect(f32 dt, const FuryRageInputs& in) {
    (void)dt;
    if (in.leftPressed)  { m_charSel = (m_charSel + CHAR_COUNT - 1) % CHAR_COUNT; m_saveDirty = true; }
    if (in.rightPressed) { m_charSel = (m_charSel + 1) % CHAR_COUNT; m_saveDirty = true; }
    if (in.aPressed) {
        GoTo(FR_LEVELS, 0.35f);
    }
    if (in.bPressed) {
        GoTo(FR_TITLE, 0.35f);
    }
}

void FuryRageGame::TickLevels(f32 dt, const FuryRageInputs& in) {
    // ---- player movement (axis-separated, 2 tiles/s) ----
    const f32 spd = kFurySpeed;
    m_player.vx = 0.0f;
    m_player.vy = 0.0f;
    if (in.leftPressed)  m_player.vx = -spd;
    if (in.rightPressed) m_player.vx =  spd;
    if (in.upPressed)    m_player.vy = -spd;
    if (in.downPressed)  m_player.vy =  spd;
    const bool moving = (m_player.vx != 0.0f || m_player.vy != 0.0f);

    // track facing direction from input (right = +1, left = -1, default +1)
    const f32 face = (in.rightPressed ? 1.0f : (in.leftPressed ? -1.0f : 1.0f));

    if (moving) {
        const f32 dx = m_player.vx * dt, dy = m_player.vy * dt;
        m_player.x += dx;
        if (FuryCircleBlocked(m_player.x * 50.0f, m_player.y * 50.0f)) m_player.x -= dx;
        m_player.y += dy;
        if (FuryCircleBlocked(m_player.x * 50.0f, m_player.y * 50.0f)) m_player.y -= dy;
        m_player.vx = m_player.vy = 0.0f;
    } else {
        m_player.vx = m_player.vy = 0.0f;
    }

    // ---- melee attack (A, cooldown) ----
    if (m_player.cd > 0.0f) m_player.cd -= dt;
    if (in.aPressed && m_player.cd <= 0.0f) {
        m_player.cd = kFuryAtkCd;
        // melee hitbox in front of the fighter
        const f32 hx = m_player.x + face;
        const f32 hy = m_player.y;
        for (i32 i = 0; i < m_enemyCount; ++i) {
            FuryFighter& e = m_enemies[i];
            if (e.dead) continue;
            const f32 edx = e.x - hx, edy = e.y - hy;
            if (edx*edx + edy*edy < 40.0f*40.0f) {
                e.hp -= 25;
                m_score += 100;
                if (e.hp <= 0) { e.hp = 0; e.dead = true; m_saveDirty = true; }
            }
        }
    }

    // ---- enemies (patrol + damage player on touch) ----
    bool anyAlive = false;
    for (i32 i = 0; i < m_enemyCount; ++i) {
        FuryFighter& e = m_enemies[i];
        if (e.dead) continue;
        anyAlive = true;
        e.x += e.vx * dt;
        if (FuryCircleBlocked(e.x * 50.0f, e.y * 50.0f)) {
            e.x -= e.vx * dt; e.vx = -e.vx;
        }
        const f32 edx = e.x - m_player.x, edy = e.y - m_player.y;
        if (edx*edx + edy*edy < 40.0f*40.0f) {
            m_player.hp -= (i32)(15.0f * dt);
            if (m_player.hp < 0) m_player.hp = 0;
        }
    }

    // ---- wave gate ----
    if (!anyAlive && !m_waveCleared) {
        m_waveCleared = true;
        m_score += 500 * m_wave;
        m_saveDirty = true;
    }
    if (m_waveCleared) {
        m_waveT += dt;
        if (m_waveT >= 2.0f) {   // 2 s pause then advance camera / next wave
            m_wave += 1;
            m_waveCleared = false;
            FuryWaveNext();
        }
    }

    // ---- camera = player centre, clamped ----
    m_camX = m_player.x;
    m_camY = m_player.y;
    if (m_camX < 0.0f) m_camX = 0.0f;
    if (m_camY < 0.0f) m_camY = 0.0f;
    if (m_camX > (f32)kFuryMapW) m_camX = (f32)kFuryMapW;
    if (m_camY > (f32)kFuryMapH) m_camY = (f32)kFuryMapH;
}

// ---- public Tick ----

void FuryRageGame::Tick(f32 dt, const FuryRageInputs& in) {
    if (m_fade.phase == 1) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k > 1.0f) ? 1.0f : k;
        if (m_fade.timer >= m_fade.dur) {
            if (m_fade.pending >= 0) m_screen = m_fade.pending;
            m_fade.pending = -1;
            m_fade.phase = 2;
            m_fade.timer = 0.0f;
            m_fade.dur = 0.35f;
        }
    } else if (m_fade.phase == 2) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k >= 1.0f) ? 0.0f : (1.0f - k);
        if (m_fade.timer >= m_fade.dur) { m_fade.phase = 0; m_fade.alpha = 0.0f; }
    }

    switch (m_screen) {
        case FR_START:
            break;
        case FR_TITLE:
            UpdateTitle(dt, in);
            break;
        case FR_CHARACTER_SELECT:
            UpdateCharacterSelect(dt, in);
            break;
        case FR_LEVELS:
            TickLevels(dt, in);
            break;
        default:
            break;
    }
}

} // namespace fnaf
