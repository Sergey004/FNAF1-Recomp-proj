/**
 * FuryRageGame.cpp: v2.130 — Fury's Rage module game logic.
 *
 * The screen graph and the counters come from the ctfak dump of
 * furys-rage.exe (build 293): see the header for the transition table.
 * The gameplay (frame 4 "levels") is now implemented:
 *   * movement: axis-separated, 4-corner AABB vs the tile grid
 *   * melee combat: hitbox/hurtbox, block, jump, attack cooldown
 *   * wave director: spawn enemies in waves, camera gate until wave cleared
 *   * levels: 4 stages + hard mode secret stage
 *
 * v2.130: the fighter is the dump's own: the walk cycle of the player
 * object "Active" (10..17 to the right, 59.. to the left), the chosen
 * fighter's picture on the child object "Active 6" (206 Freddy / 365 Monty
 * / 364 Roxanne / 625 Chica), the "move box" as the hitbox, the dump's
 * start position (540, 829) and the per-character step of the dump
 * (groups 98..103). The enemies come from the dump's own table (14
 * types with their picture, hp and speed).
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

// ---- the dump's level data (frame 4 "levels") ----

// The player object "Active": animation 0 is the walk cycle, direction 0
// faces right and direction 16 faces left.
static const i32 kFuryWalkRight[8] = { 10, 11, 12, 13, 14, 15, 16, 17 };
static const i32 kFuryWalkLeft[8]  = { 59, 84, 18, 19, 20, 22, 23, 56 };

// The child object "Active 6" carries the chosen fighter's picture: its
// animation follows the "character" counter (Freddy, Monty, Roxanne,
// Chica) - the dump's groups 316..319.
static const i32 kFuryCharArt[4] = { 206, 365, 364, 625 };

// The "move box" step of the dump (groups 98..103): Freddy and Roxanne
// move 5 px per tick, Monty 4 px and Chica 6 px (30 ticks/s) -> tiles/s.
static const f32 kFuryCharSpeed[4] = { 3.0f, 2.4f, 3.0f, 3.6f };

// The enemy object "Active 5": 14 types, each with its own picture, hp and
// speed (the dump's spawn groups 161.. and the type -> animation table).
struct FuryEnemyDef { i32 img; i32 hp; i32 speed; };
static const FuryEnemyDef kFuryEnemies[14] = {
    {  72, 25, 25 },   //  1 PoisonFoxy
    { 107, 20, 30 },   //  2 Thug Chica
    { 129, 35, 20 },   //  3 Thuglife Chipper
    { 182, 60, 15 },   //  4 Punk Mangle
    { 300, 80, 15 },   //  5 H sisters
    { 341, 80, 15 },   //  6 H sisters
    { 390, 50, 30 },   //  7 Clown
    { 439, 30, 40 },   //  8 mech
    { 528, 40, 90 },   //  9 Vanny
    { 582, 65, 20 },   // 10 fire girl
    { 554, 25, 15 },   // 11 trap
    { 456, 80, 12 },   // 12 Marionette
    { 571, 20, 15 },   // 13 IceFreddy
    { 985, 30, 10 },   // 14 Mechionette
};

// The dump's "move box" (the player's collision box) is 82x62 px and the
// player starts at (540, 829) - the feet on the floor strip.
static const f32 kFuryStartX = 511.0f / 50.0f;   // the box's centre
static const f32 kFuryStartY = 791.0f / 50.0f;
static const i32 kFuryBoxW = 164;                // 82 px (x100 of a tile)
static const i32 kFuryBoxH = 124;                // 62 px

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

void FuryRageGame::FurySpawnEnemy(f32 x, f32 y, i32 img, i32 hp, f32 speed) {
    for (i32 i = 0; i < m_enemyCount; ++i) {
        FuryFighter& e = m_enemies[i];
        if (e.dead) {
            e.x = x; e.y = y; e.img = img; e.dead = false;
            e.vx = -speed; e.vy = 0;
            e.w = kFuryFighterW; e.h = kFuryFighterH;
            e.hp = e.maxHp = hp; e.cd = 0; e.animT = 0; return;
        }
    }
    if (m_enemyCount < kMaxEnemies) {
        FuryFighter& e = m_enemies[m_enemyCount++];
        e.x = x; e.y = y; e.img = img; e.dead = false;
        e.vx = -speed; e.vy = 0;
        e.w = kFuryFighterW; e.h = kFuryFighterH;
        e.hp = e.maxHp = hp; e.cd = 0; e.animT = 0;
    }
}

void FuryRageGame::FuryWaveNext() {
    m_waveT = 0.0f;
    m_waveCleared = false;
    m_enemyCount = 0;
    for (i32 i = 0; i < kMaxEnemies; ++i) m_enemies[i].dead = true;
    // The wave grows with the wave number; the enemies come from the
    // dump's own table (types 1..14) in order.
    const i32 count = 2 + (m_wave % 3);
    for (i32 i = 0; i < count; ++i) {
        const i32 type = (m_wave * 2 + i) % 14;
        const FuryEnemyDef& d = kFuryEnemies[type];
        const f32 x = m_player.x + 8.0f + (f32)i * 3.0f;
        const f32 y = m_player.y;
        // the dump's enemy speed is a step per tick; the port reads it as
        // tiles per second (25 -> 1 tile/s)
        FurySpawnEnemy(x, y, d.img, d.hp, (f32)d.speed * 0.04f);
    }
}

// v2.130: the level starts with the dump's own fighter: the walk cycle of
// "Active", the chosen fighter's picture on the child object "Active 6",
// the "move box" as the hitbox and the dump's start position.
void FuryRageGame::FuryStartLevel() {
    m_wave = 0;
    m_score = 0;
    m_enemyHp = 0;
    m_enemyMaxHp = 0;
    m_player.charId = m_charSel;
    m_player.img = kFuryWalkRight[0];
    m_player.overlay = kFuryCharArt[m_charSel];
    m_player.hp = m_player.maxHp = kFuryMaxHp;
    m_player.w = kFuryBoxW;
    m_player.h = kFuryBoxH;
    m_player.x = kFuryStartX;
    m_player.y = kFuryStartY;
    m_player.vx = m_player.vy = 0;
    m_player.cd = 0;
    m_player.animT = 0;
    m_player.dmgAcc = 0;
    m_player.dead = false;
    m_player.isEnemy = false;
    m_camX = m_player.x;
    m_camY = m_player.y;
    m_waveCleared = false;
    m_waveT = 0;
    m_levelStarted = true;
    FuryWaveNext();
}

// ---- constructor ----

FuryRageGame::FuryRageGame()
    : m_screen(FR_START), m_titleSel(0), m_charSel(0), m_hardMode(false),
      m_wantsExit(false), m_saveDirty(false), m_holdT(0.0f),
      m_levelStarted(false),
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
    m_levelStarted = false;
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
    // leaving the level resets its setup (the next entry starts over)
    if (screen != FR_LEVELS) m_levelStarted = false;
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
    // ---- player movement (axis-separated; the dump's per-character step) ----
    const i32 ci = (m_charSel >= 0 && m_charSel < CHAR_COUNT) ? m_charSel : 0;
    const f32 spd = kFuryCharSpeed[ci];
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

    // ---- the walk cycle of "Active" (animation 0, direction 0 / 16) ----
    if (moving) {
        m_player.animT += dt;
        if (m_player.animT >= 0.12f) {
            m_player.animT -= 0.12f;
            m_player.walk = (m_player.walk + 1) & 7;
        }
        const i32* cyc = (face < 0.0f) ? kFuryWalkLeft : kFuryWalkRight;
        m_player.img = cyc[m_player.walk];
    } else {
        m_player.animT = 0.0f;
        m_player.img = (face < 0.0f) ? kFuryWalkLeft[0] : kFuryWalkRight[0];
    }

    // ---- melee attack (A, cooldown) ----
    if (m_player.cd > 0.0f) m_player.cd -= dt;
    if (in.aPressed && m_player.cd <= 0.0f) {
        m_player.cd = kFuryAtkCd;
        // the dump's "punch box" is 83 px -> 1.66 tiles in front of the
        // fighter
        const f32 hx = m_player.x + face * 1.2f;
        const f32 hy = m_player.y;
        for (i32 i = 0; i < m_enemyCount; ++i) {
            FuryFighter& e = m_enemies[i];
            if (e.dead) continue;
            const f32 edx = e.x - hx, edy = e.y - hy;
            if (edx*edx + edy*edy < 1.7f*1.7f) {
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
        if (edx*edx + edy*edy < 1.7f*1.7f) {
            // 15 hp per second while the enemy is on top of the player
            m_player.dmgAcc += 15.0f * dt;
            if (m_player.dmgAcc >= 1.0f) {
                const i32 d = (i32)m_player.dmgAcc;
                m_player.dmgAcc -= (f32)d;
                m_player.hp -= d;
                if (m_player.hp < 0) m_player.hp = 0;
            }
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

    // ---- camera = the player in the middle of the 1920x1080 view ----
    const f32 halfW = 1920.0f / 2.0f / 50.0f;
    const f32 halfH = 1080.0f / 2.0f / 50.0f;
    m_camX = m_player.x - halfW;
    m_camY = m_player.y - halfH;
    if (m_camX < 0.0f) m_camX = 0.0f;
    if (m_camY < 0.0f) m_camY = 0.0f;
    if (m_camX > (f32)kFuryMapW - 2.0f * halfW) m_camX = (f32)kFuryMapW - 2.0f * halfW;
    if (m_camY > (f32)kFuryMapH - 2.0f * halfH) m_camY = (f32)kFuryMapH - 2.0f * halfH;
    if (m_camX < 0.0f) m_camX = 0.0f;
    if (m_camY < 0.0f) m_camY = 0.0f;
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
            if (!m_levelStarted) FuryStartLevel();   // v2.130: the dump's start
            TickLevels(dt, in);
            break;
        default:
            break;
    }
}

} // namespace fnaf
