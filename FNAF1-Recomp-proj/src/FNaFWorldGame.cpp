/**
 * FNaFWorldGame.cpp: v2.90 — FNAF World module game logic (see FNaFWorldGame.h).
 *
 * Dump authority: ctfak full dump of fnaf-world.exe
 *   frame 0 "Frame 32"     — splash: 3 s (or Enter) -> next; fade 980 in / 880 out
 *   frame 1 "check"        — INI first-run router (the port's save replaces it)
 *   frame 2 "title screen" — 15 groups (slide BLEND: dump distances +
 *       thresholds, C-Edition durations 3/3/2 s):
 *       g4  logo   4597: slide right, shift < 800 -> left 127
 *       g3  party  4574: slide up,   shift < 416 -> top 64
 *       g6  START  4569: slide left, shift < 550 -> left 260
 *       g7  sparkle 1207 x8 every 100 ms (g8-11: wander + fade)
 *       g13 on start: sample #138 "Title screen theme - One for the road
 *           draft1" loop ch0 (the source mp3 is decoded to PCM16 at pack
 *           time, so the loop plays on the 360)
 *       g1  Escape -> End application;  g2 click START -> next frame
 *       fade-out 880 ms
 *   frame 5 "overworld"    — step 2 (world stub here: movement only)
 * World constants from the FNAF-World-C-Edition reference (logic 1:1):
 *   speed 2 tiles/s, Freddy hitbox 0.7x0.45 scale 0.95, camera = centre.
 */

#include "FNaFWorldGame.h"
#include "FNaFWorldMap.h"
#include "XdkCompat.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace fnaf {

// ---- dump constants (frame 0 "Frame 32", 800x480 space) ----
static const f32 kSplashSeconds   = 3.0f;    // g1: one-shot 3000 ms
static const f32 kSplashFadeInS   = 0.98f;   // frame 0 fade-in  980 ms
static const f32 kSplashFadeOutS  = 0.88f;   // frame 0/2 fade-out 880 ms

// ---- title slide model: the BLEND of both sources ----
//   * start/end positions + total distances — the dump (frame 2 layout lefts
//     and the g3/g4/g6 thresholds: 800 / 416 / 550 dump-px)
//   * durations — the FNAF-World-C-Edition observable feel (Title_Screen.c:
//     TITLE_SECONDS_TO_CENTRE 3, PARTY 3, START 2); the dump's raw per-frame
//     steps give 2.67/1.73/1.83 s — the recreation author timed the original
//     at 3/3/2, so the rate here is distance/duration (px/s).
static const f32 kLogoStartX     = -673.0f;   // layout left (4597)
static const f32 kLogoShiftMax   =  800.0f;   // g4 threshold
static const f32 kLogoRate       =  800.0f / 3.0f;   // 266.7 px/s
static const f32 kPartyStartTop  =  480.0f;   // layout top (4574)
static const f32 kPartyShiftMax  =  416.0f;   // g3 threshold
static const f32 kPartyRate      =  416.0f / 3.0f;   // 138.7 px/s
static const f32 kStartStartX    =  810.0f;   // layout left (4569)
static const f32 kStartShiftMax  =  550.0f;   // g6 threshold
static const f32 kStartRate      =  550.0f / 2.0f;   // 275.0 px/s

static const f32 kWorldSpeed      =  2.0f;   // World.c: 2 tiles/s
static const f32 kFreddyW         =  0.70f;  // World.c ResetWorld hitbox
static const f32 kFreddyH         =  0.45f;

// ---- v2.91 step 2: overworld (exe frame 5) -------------------------------
// Area bands (dump px). The exe stores the real regions as 'area' counter
// markers whose alterable values the ctfak JSON does not carry, so the
// quadrant layout of the dump map is used instead — labeled approximation:
//   minewalls y 2000..3850 (south) = Mysterious Mine, snow y<700 (north) =
//   Dusting Fields, lake x>=3000 (east) = Lilygear Lake, woods x<1500 (west)
//   = Choppy's Woods, centre = Fazbear Hills.
static i32 AreaAtPx(i32 px, i32 py) {
    if (py >= 2000) return 11;     // Mysterious Mine
    if (py <  700)  return 5;      // Dusting Fields
    if (px >= 3000) return 8;      // Lilygear Lake
    if (px <  1500) return 3;      // Choppy's Woods
    return 0;                      // Fazbear Hills
}

// exe frame 5 Groups 3..11: 'area' -> the looping overworld theme.
// Group 3 (area < 5): sample #93 "travel1B"; g4 area==5: #107 "Ice Dungeon";
// g5 area==8: #109 "Water theme - Emerald Waterfalls"; g6 area==11: #108
// "Dungeon theme - The tower of Majdel Shams draft1".
const char* FNaFWorldGame::AreaTheme(i32 areaId) {
    switch (areaId) {
        case 5:  return "Ice Dungeon";
        case 8:  return "Water theme - Emerald Waterfalls";
        case 11: return "Dungeon theme - The tower of Majdel Shams draft1";
        default: return "travel1B";          // the exe Hills/Woods jingle-theme
    }
}

const char* FNaFWorldGame::AreaName(i32 areaId) {
    switch (areaId) {
        case 3:  return "Choppy's Woods";
        case 5:  return "Dusting Fields";
        case 8:  return "Lilygear Lake";
        case 11: return "Mysterious Mine";
        default: return "Fazbear Hills";
    }
}

// The collision grid, decoded once from the generated header (20 % of the
// 120x80 map is blocked: trees/rocks/walls/fences by name class).
static unsigned char s_grid[kFwMapW * kFwMapH];
static bool s_gridReady = false;

// The character walk tables live in the generated header (kFwLeaderAnim /
// kFwCharFrames — exe 'character' object, one front-facing anim per char).

// 15 fps walk cycle (C-Edition CreateAnimation_V2(..., 30, 15, 60, 60))
static const f32 kWalkFps        = 15.0f;
// first playable overworld: the classic four lead (party editing is step 4)
static const u8  kLeaderFirst    = 0;      // Freddy
// chest pickup radius (dump px, centre to centre)
static const f32 kChestRadiusPx  = 45.0f;
// wild-area battle encounter window, s of continuous movement
static const f32 kEncounterMin   = 14.0f;
static const f32 kEncounterSpan  = 12.0f;  // +0..12 s (rand)

FNaFWorldGame::FNaFWorldGame() {
    preloadFrame = 0;
    ResetToTitle();
}

void FNaFWorldGame::ResetToTitle() {
    m_screen = SCR_DISCLAIMER;      // frame 0 is the boot frame
    m_fade.Init();
    m_fade.StartIn(kSplashFadeInS); // frame 0 fade-in 980
    m_time = 0.0f;
    m_cardT = 0.0f;
    m_exitRequested = false;
    m_saveDirty = false;

    m_titleT = 0.0f;
    m_titleChoice = 0;
    m_titleThemeStarted = false;
    m_xHoldT = 0.0f;

    m_logoShift  = 0.0f;
    m_partyShift = 0.0f;
    m_startShift = 0.0f;

    m_freddyX = 0.0f; m_freddyY = 0.0f;
    m_freddyVX = 0.0f; m_freddyVY = 0.0f;
    m_freddyDir = 0;
    m_camX = 0.0f; m_camY = 0.0f;
    m_zone = 0;
    m_zoneLevel = 1;
    m_fazTokens = 0;
    m_stepSndT = 0.0f;
    m_lastX = 0.0f; m_lastY = 0.0f;
    m_lastZone = 0;

    m_areaId = 0;
    m_walkT = 0.0f;
    m_encT = kEncounterMin;
    m_leaderAnim = kFwLeaderAnim[kLeaderFirst];
    m_worldInit = false;
    m_themeOn = false;
    m_battleInit = false;
    m_dmgNext = 0;
    for (int i = 0; i < 8; ++i) { m_dmgAmt[i] = 0; m_dmgBorn[i] = -99.0f; m_dmgEnemy[i] = false; }

    m_fsStage = 0; m_fsSlot = 0; m_fsRow = 0; m_fsPick = 0; m_fsSaveLive = false;

    m_party[0] = 0; m_party[1] = 12; m_party[2] = 13; m_party[3] = 14;
    m_chipsOwned = 0; m_chipsEq = 0xFFFFFFFFu; m_bytesOwned = 0;
    m_menuScreen = 0; m_menuRow = 0; m_menuCol = 0; m_menuMember = 0;
}

// ------------------------------------------------------------
// exe frame 3 "file setup" — the slot/new-game flow, three stages:
//   0: three file slots (summary "Name: X : Level: N" per slot; the exe
//      reads per-slot INI sections fnafw1/2/3 — the port's single backing
//      save lives in slot 1, slots 2/3 read as empty; labeled deviation)
//   1: "new game" / "continue" (continue only with a live save)
//   2: the YES/NO confirm
// YES -> the overworld. (The exe's mode/difficulty picks at the later
// stages stay defaults — Adventure/Normal — until the Progress keys land;
// labeled.)
// ------------------------------------------------------------
void FNaFWorldGame::TickFileSetup(f32 dt, const FNaFWorldInputs& in) {
    if (m_cardT == 0.0f) {
        m_fsStage = 0; m_fsSlot = 0; m_fsRow = 0;
        m_fsSaveLive = (m_lastZone != 0);   // a save wrote lastFrame != 0
    }
    m_cardT += dt;

    if (m_fsStage == 0) {
        if (in.upPressed)   { m_fsSlot = (m_fsSlot + 2) % 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.downPressed) { m_fsSlot = (m_fsSlot + 1) % 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.bPressed)    { RequestScreen(SCR_TITLE); return; }
        if (in.aPressed) {
            m_fsRow = 0;                    // new game preselected (the exe's order)
            m_fsStage = 1;
            Sfx("Cartoon_Pop_01", false, 3, 80);
        }
        return;
    }
    if (m_fsStage == 1) {
        if (in.upPressed || in.downPressed) {
            m_fsRow = 1 - m_fsRow;
            Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60);
        }
        if (in.bPressed)    { m_fsStage = 0; return; }
        if (in.aPressed) {
            if (m_fsRow == 1 && !m_fsSaveLive) {    // continue with no save
                Sfx("Cartoon_Pop_02", false, 3, 60);
                return;
            }
            m_fsPick = m_fsRow;             // remember new game / continue
            m_fsRow = 0;                    // YES preselected
            m_fsStage = 2;
            Sfx("Cartoon_Pop_01", false, 3, 80);
        }
        return;
    }
    // stage 2: YES / NO
    if (in.upPressed || in.downPressed) {
        m_fsRow = 1 - m_fsRow;
        Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60);
    }
    if (in.bPressed)    { m_fsStage = 1; return; }
    if (in.aPressed) {
        if (m_fsRow == 0) {                 // YES -> commit
            if (m_fsPick == 1) {            // continue: keep the save
                m_worldInit = false;        // re-place at the save position
            } else {                        // new game: fresh state
                Progress::WipeFW();
                m_zoneLevel = 1; m_fazTokens = 0;
                m_lastX = 0.0f; m_lastY = 0.0f; m_lastZone = 0;
                m_freddyX = 0.0f; m_freddyY = 0.0f;
                m_freddyDir = 0; m_walkT = 0.0f;
            }
            m_saveDirty = true;
            Sfx("Cartoon_Pop_01", false, 3, 100);
            RequestScreen(SCR_WORLD);
        } else {                            // NO -> back to the slots
            m_fsStage = 0;
        }
    }
}

// ------------------------------------------------------------
// exe frame 6 "game over": the "death" sample, 5 s (Timer 5000) ->
// frame 1 "check" (the INI router -> the title). Escape = End application;
// the port's B mirror keeps the convention (labeled deviation).
// ------------------------------------------------------------
void FNaFWorldGame::TickGameOver(f32 dt, const FNaFWorldInputs& in) {
    if (m_cardT == 0.0f) Sfx("death", true, 1, 100);
    m_cardT += dt;
    if (in.bPressed) { SfxStop("death"); m_exitRequested = true; return; }
    if (m_cardT >= 5.0f) {
        SfxStop("death");
        m_cardT = 0.0f;
        RequestScreen(SCR_TITLE);
    }
}

void FNaFWorldGame::Sfx(const char* s, bool loop, i32 ch, i32 vol) {
    if (audio.play) audio.play(s, loop, ch, vol);
}
void FNaFWorldGame::SfxStop(const char* s) {
    if (audio.stop) audio.stop(s);
}

// v2.76 model (shared with F2/SL): fade out the CURRENT screen, switch at the
// end, fade the new one in. Durations from the dump frame Transition chunks.
void FNaFWorldGame::RequestScreen(i32 target) {
    if (m_fade.Switching()) return;
    // frame fades: splash out 880; title out 880; overworld in (step 2 reads
    // frame 5's chunk — 980 placeholder, labeled).
    m_fade.StartOut(target, kSplashFadeOutS);
}

void FNaFWorldGame::Tick(f32 dt, const FNaFWorldInputs& in) {
    m_time += dt;

    // the fade drives the deferred switch (our core's shared model)
    {
        const int done = m_fade.Tick(dt);
        if (done >= 0) {
            m_screen = (Screen)done;
            m_cardT = 0.0f;
            if (m_screen == SCR_DISCLAIMER) m_fade.StartIn(kSplashFadeInS);
            else                            m_fade.StartIn(kSplashFadeInS); // labeled: frame fades TBD
        }
    }

    switch (m_screen) {
        case SCR_DISCLAIMER: TickDisclaimer(dt, in); break;
        case SCR_TITLE:      TickTitle(dt, in);      break;
        case SCR_FILESETUP:  TickFileSetup(dt, in);  break;   // exe frame 3
        case SCR_WORLD:      TickWorld(dt, in);      break;
        case SCR_BATTLE:     TickBattle(dt, in);     break;   // step-3 placeholder
        case SCR_GAMEOVER:   TickGameOver(dt, in);   break;   // exe frame 6
        default:             m_cardT += dt;  break;  // step 4 screens hold
    }
}

// ------------------------------------------------------------
// frame 0 "Frame 32": splash, 3 s or Enter (our A) -> next frame
// ------------------------------------------------------------
void FNaFWorldGame::TickDisclaimer(f32 dt, const FNaFWorldInputs& in) {
    m_cardT += dt;
    if (m_cardT >= kSplashSeconds || in.aPressed) {
        m_cardT = 0.0f;
        RequestScreen(SCR_TITLE);       // frame 0 g1/g2: next frame
    }
}

// ------------------------------------------------------------
// frame 2 "title screen"
// ------------------------------------------------------------
void FNaFWorldGame::TickTitle(f32 dt, const FNaFWorldInputs& in) {
    m_titleT += dt;

    // g13: the title theme on frame entry (loop, ch0). The pak entry is
    // PCM16 now (the source mp3 is decoded at pack time), so the loop
    // actually plays — the old "silent on 360" deviation is gone.
    if (!m_titleThemeStarted) {
        m_titleThemeStarted = true;
        Sfx("Title screen theme - One for the road draft1", true, 0, 100);
    }

    // g4/g3/g6: the slide-ins — dump distances, C-Edition durations (dt-based
    // so the feel survives any tick-rate change)
    if (m_logoShift  < kLogoShiftMax)  m_logoShift  += kLogoRate  * dt;
    if (m_partyShift < kPartyShiftMax) m_partyShift += kPartyRate * dt;
    if (m_startShift < kStartShiftMax) m_startShift += kStartRate * dt;

    // g2: click START -> next frame. The button is only REACHABLE once its
    // slide lands (the cursor cannot click offscreen objects in CF either).
    const bool startReady = (m_startShift >= kStartShiftMax);
    if (in.aPressed && startReady) {
        SfxStop("Title screen theme - One for the road draft1");
        RequestScreen(SCR_FILESETUP);   // -> frame 3 "file setup" (v2.91:
        m_saveDirty = true;             //  the exe's own slot/new-game flow)
        return;
    }

    // g1: Escape = End application (dump). Our B is the Escape mirror.
    if (in.bPressed) { m_exitRequested = true; return; }

    // port UX convention (F2/SL): title X-hold 5 s wipes the save
    if (in.xHeld) {
        m_xHoldT += dt;
        if (m_xHoldT >= 5.0f) {
            m_xHoldT = 0.0f;
            Progress::WipeFW();
            m_zoneLevel = 1; m_fazTokens = 0;
            m_lastX = 0.0f; m_lastY = 0.0f; m_lastZone = 0;
            printf("FNAFWORLD SAVE WIPED (X hold)\n");
        }
    } else {
        m_xHoldT = 0.0f;
    }
}

// ------------------------------------------------------------
// frame 5 "overworld" — v2.91 step 2: the exe overworld.
//   * movement: C-Edition World.c UpdateWorldEntity — axis-separated,
//     4-corner AABB vs the collision grid, revert the axis on a hit
//   * zones: exe 'area' bands -> theme loop (Groups 3..11)
//   * chests: exe "spawn chest" positions, A opens -> faz-tokens
//   * encounters: moving in a wild area counts down -> SCR_BATTLE (the
//     battle itself is step 3; the screen shows the pick for now)
//   * B no longer exits (the exe overworld has no Escape-back; menus come
//     with step 4) — labeled
// ------------------------------------------------------------
void FNaFWorldGame::TickWorld(f32 dt, const FNaFWorldInputs& in) {
    // the world menu rides OVER the world (movement frozen while open)
    if (m_menuScreen != 0) {
        TickMenu(dt, in);
        return;
    }

    // one-time decode of the generated collision grid
    if (!s_gridReady) {
        FwMapDecodeCollision(s_grid);
        s_gridReady = true;
    }

    // the area theme restarts on re-entry (the battle stop released it)
    if (!m_themeOn) {
        Sfx(AreaTheme(m_areaId), true, 0, 100);
        m_themeOn = true;
    }

    // one-time spawn: the save position if ApplyProgress ran, else the
    // Fazbear Hills centre (dump px ~2250,1350 -> tile 45,27) — snapped to
    // the nearest open cell so a generated solid never traps the spawn.
    if (!m_worldInit) {
        m_worldInit = true;
        if (m_freddyX == 0.0f && m_freddyY == 0.0f) {
            i32 tx = 45, ty = 27;
            for (i32 r = 0; r < 8; ++r) {           // spiral out to 8 cells
                bool found = false;
                for (i32 oy = -r; oy <= r && !found; ++oy)
                for (i32 ox = -r; ox <= r && !found; ++ox) {
                    const i32 gx = tx + ox, gy = ty + oy;
                    if (gx < 0 || gy < 0 || gx >= kFwMapW || gy >= kFwMapH) continue;
                    if (!s_grid[gy * kFwMapW + gx]) { tx = gx; ty = gy; found = true; }
                }
                if (found) break;
            }
            m_freddyX = (f32)tx + 0.5f - kFreddyW * 0.5f;
            m_freddyY = (f32)ty + 0.5f - kFreddyH * 0.5f;
        }
        m_areaId = AreaAtPx((i32)((m_freddyX + kFreddyW * 0.5f) * 50.0f),
                            (i32)((m_freddyY + kFreddyH * 0.5f) * 50.0f));
        Sfx(AreaTheme(m_areaId), true, 0, 100);
    }

    // WASD/LS movement at 2 tiles/s (1.5x on X-hold, the C-Edition fast/loud)
    const f32 spd = kWorldSpeed * (in.xHeld ? 1.5f : 1.0f);
    m_freddyVX = in.lookDir * spd;
    m_freddyVY = -in.lookDirY * spd;   // stick up = -Y (tiles grow down)

    if (m_freddyVY < -0.01f)      m_freddyDir = 1;   // up
    else if (m_freddyVY > 0.01f)  m_freddyDir = 4;   // down
    if (m_freddyVX > 0.01f)       m_freddyDir = 3;   // right
    else if (m_freddyVX < -0.01f) m_freddyDir = 2;   // left
    const bool moving = (m_freddyVX != 0.0f || m_freddyVY != 0.0f);
    if (!moving) m_freddyDir = 0;

    // axis-separated collision (World.c UpdateWorldEntity semantics):
    // move one axis, revert on any blocked corner, then the other.
    // CircleBlocked takes the HITBOX CENTRE in px; m_freddy* is the corner.
    if (moving) {
        const f32 dx = m_freddyVX * dt, dy = m_freddyVY * dt;
        const f32 cxo = kFreddyW * 0.5f * 50.0f, cyo = kFreddyH * 0.5f * 50.0f;
        m_freddyX += dx;
        if (CircleBlocked(m_freddyX * 50.0f + cxo, m_freddyY * 50.0f + cyo)) m_freddyX -= dx;
        m_freddyY += dy;
        if (CircleBlocked(m_freddyX * 50.0f + cxo, m_freddyY * 50.0f + cyo)) m_freddyY -= dy;
        m_walkT += dt * kWalkFps;      // the 15 fps walk clock
    } else {
        m_walkT = 0.0f;
    }

    // camera = Freddy centre, clamped to the map (World.c GetCameraView)
    m_camX = m_freddyX + kFreddyW * 0.5f;
    m_camY = m_freddyY + kFreddyH * 0.5f;
    if (m_camX < 0.0f) m_camX = 0.0f;
    if (m_camY < 0.0f) m_camY = 0.0f;
    if (m_camX > (f32)kFwMapW) m_camX = (f32)kFwMapW;
    if (m_camY > (f32)kFwMapH) m_camY = (f32)kFwMapH;

    // zone band -> the exe 'area' counter; switch the theme on change
    {
        const i32 area = AreaAtPx((i32)(m_camX * 50.0f), (i32)(m_camY * 50.0f));
        if (area != m_areaId) {
            SfxStop(AreaTheme(m_areaId));
            m_areaId = area;
            Sfx(AreaTheme(area), true, 0, 100);
            m_encT = kEncounterMin + (f32)(rand() % 100) * 0.01f * kEncounterSpan;
        }
    }

    // wild-area encounters (Fazbear Hills is safe — the exe start zone)
    if (m_areaId != 0 && moving) {
        m_encT -= dt;
        if (m_encT <= 0.0f) {
            m_encT = kEncounterMin + (f32)(rand() % 100) * 0.01f * kEncounterSpan;
            SfxStop(AreaTheme(m_areaId));
            m_themeOn = false;
            Sfx("battle theme - Adventure ahead -v2", false, 0, 100);
            RequestScreen(SCR_BATTLE);     // step 3 replaces the placeholder
            return;
        }
    }

    // chests: A opens the nearest one within reach
    if (in.aPressed) {
        const f32 fx = m_camX * 50.0f, fy = m_camY * 50.0f;
        i32 best = -1; f32 bestD2 = kChestRadiusPx * kChestRadiusPx;
        for (i32 i = 0; i < kFwChestCount; ++i) {
            if (kFwChests[i].opened) continue;
            const f32 ddx = (f32)kFwChests[i].x - fx;
            const f32 ddy = (f32)kFwChests[i].y - fy;
            const f32 d2 = ddx * ddx + ddy * ddy;
            if (d2 < bestD2) { bestD2 = d2; best = i; }
        }
        if (best >= 0) OpenChest(best);
    }

    // the save position is written by FillProgress from m_freddyX/Y
    // (the centre) — no duplicate truth here

    // the world menu (the exe's menu button -> frames 4/7/12): Y or START.
    // The theme keeps playing (the exe's menus ride over the world).
    if (in.yPressed || in.menuPressed) {
        m_menuScreen = 1;
        m_menuRow = 0; m_menuCol = 0; m_menuMember = 0;
        Sfx("Cartoon_Pop_01", false, 3, 80);
        return;
    }
}

// ------------------------------------------------------------
// the world menus (v2.91; exe frames 4 "character select" / 7 "chips" /
// 12 "Bytes"). Same interaction shape as the exe's grids: 4 member rows
// on the left, the item grid on the right, A acts, B goes back.
//   PARTY  — A on a grid character swaps it into the selected member
//   CHIPS  — A on an owned chip equips/unequips it on the member
//   BYTES  — the owned bytes list (the deploy model lands with the byte
//            shop mining; labeled)
// Interim grants: chest i (i < 7) also awards chip i (the C-Edition chip
// boxes; the exe's chip-box positions are a later mine — labeled).
// ------------------------------------------------------------
void FNaFWorldGame::TickMenu(f32 dt, const FNaFWorldInputs& in) {
    (void)dt;
    static const i32 kOwnedChars[8] = { 0, 12, 13, 14, 15, 16, 17, 18 }; // F..(the walk-table leaders)

    // B: one level up (or close on the root)
    if (in.bPressed || in.menuPressed) {
        if (m_menuScreen > 1) { m_menuScreen = 1; Sfx("Cartoon_Pop_02", false, 3, 60); }
        else                  { m_menuScreen = 0; Sfx("Cartoon_Pop_02", false, 3, 60); }
        return;
    }

    if (m_menuScreen == 1) {                       // root: PARTY/CHIPS/BYTES
        if (in.upPressed)   { m_menuRow = (m_menuRow + 2) % 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.downPressed) { m_menuRow = (m_menuRow + 1) % 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.aPressed) {
            m_menuScreen = 2 + m_menuRow;
            m_menuRow = 0; m_menuCol = 0; m_menuMember = 0;
            Sfx("Cartoon_Pop_01", false, 3, 80);
        }
        return;
    }

    if (m_menuScreen == 2) {                       // PARTY (frame 4)
        // left/right = the member (0..3); up/down = the owned list (8 rows);
        // A swaps the picked character into the selected member slot
        if (in.leftPressed)  { m_menuMember = (m_menuMember + 3) & 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.rightPressed) { m_menuMember = (m_menuMember + 1) & 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.upPressed)    { m_menuRow = (m_menuRow + 7) % 8; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.downPressed)  { m_menuRow = (m_menuRow + 1) % 8; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.aPressed) {
            m_party[m_menuMember] = kOwnedChars[m_menuRow];
            m_saveDirty = true;
            Sfx("Cartoon_Pop_01", false, 3, 100);
        }
        return;
    }

    if (m_menuScreen == 3) {                       // CHIPS (frame 7)
        // up/down = the member row; left/right = the chip grid (8 columns)
        if (in.upPressed)    { m_menuMember = (m_menuMember + 3) & 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.downPressed)  { m_menuMember = (m_menuMember + 1) & 3; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.leftPressed)  { m_menuCol = (m_menuCol + 7) & 7; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.rightPressed) { m_menuCol = (m_menuCol + 1) & 7; Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60); }
        if (in.aPressed) {
            const u32 bit = 1u << m_menuCol;
            const i32 cur = GetChipEq(m_menuMember);
            if (!(m_chipsOwned & bit)) {
                Sfx("Cartoon_Pop_02", false, 3, 60);        // not owned yet
            } else if (cur == m_menuCol) {
                m_chipsEq &= ~(0xFFu << (m_menuMember * 8)); // unequip
                m_chipsEq |= (0xFFu << (m_menuMember * 8));
                m_saveDirty = true;
                Sfx("Cartoon_Pop_02", false, 3, 80);
            } else {
                m_chipsEq &= ~(0xFFu << (m_menuMember * 8));
                m_chipsEq |= ((u32)(m_menuCol + 1) << (m_menuMember * 8));
                m_saveDirty = true;
                Sfx("Cartoon_Pop_01", false, 3, 100);
            }
        }
        return;
    }

    // BYTES (frame 12): the owned list; the deploy model is a later mine
    if (in.upPressed || in.downPressed) {
        m_menuRow = 1 - m_menuRow;
        Sfx("CARTOON_POP_DOUB_40013706", false, 3, 60);
    }
}

// 4-corner hitbox vs the grid (World.c CheckCollisionTilemap); map borders
// count as blocked so Freddy cannot leave the 6000x4000 world.
bool FNaFWorldGame::CircleBlocked(f32 px, f32 py) const {
    static const f32 hw = kFreddyW * 50.0f * 0.5f;
    static const f32 hh = kFreddyH * 50.0f * 0.5f;
    const f32 cx = px, cy = py;
    const f32 xs[4] = { cx - hw, cx + hw, cx - hw, cx + hw };
    const f32 ys[4] = { cy - hh, cy - hh, cy + hh, cy + hh };
    for (i32 i = 0; i < 4; ++i) {
        const i32 gx = (i32)(xs[i] / 50.0f);
        const i32 gy = (i32)(ys[i] / 50.0f);
        if (gx < 0 || gy < 0 || gx >= kFwMapW || gy >= kFwMapH) return true;
        if (s_grid[gy * kFwMapW + gx]) return true;
    }
    return false;
}

void FNaFWorldGame::EnterArea(i32 areaId) {
    SfxStop(AreaTheme(m_areaId));
    m_areaId = areaId;
    Sfx(AreaTheme(areaId), true, 0, 100);
    m_themeOn = true;
}

// ------------------------------------------------------------
// SCR_BATTLE — v2.91 step 3: the Battle_Rework.c model.
//   * every member attacks on a 5 s cooldown (ATTACK_COOLDOWN), the
//     enemy members stagger half a second apart (the NO_ATTACK_ANIMATION
//     offset in InitBattle)
//   * the player's ready member attacks on A (the real attack-menu pick
//     needs the exe event mining — labeled placeholder)
//   * victory pays faz-tokens, defeat returns to the title (the game-over
//     art screen comes with step 4)
// Enemy rosters/HP are zone-tier approximations until the battle events
// are mined (the exe battle runs inside the overworld frame's 'battle'
// counter, so its data lives in the 2128 groups there).
// ------------------------------------------------------------
void FNaFWorldGame::TickBattle(f32 dt, const FNaFWorldInputs& in) {
    static const f32 kCooldown        = 5.0f;   // ATTACK_COOLDOWN (5 s)
    static const f32 kEnemyStagger    = 0.25f;  // the i*0.5/2 s init offset
    static const f32 kResultHold      = 2.6f;

    if (!m_battleInit) {
        m_battleInit = true;
        memset(&m_battle, 0, sizeof(m_battle));
        for (int i = 0; i < 8; ++i) { m_dmgBorn[i] = -99.0f; m_dmgAmt[i] = 0; }
        m_dmgNext = 0;

        // enemy count/HP by area tier (0 hills .. 11 mine)
        static const i32 kCountByArea[4] = { 1, 2, 2, 3 };
        const int tier = (m_areaId == 0) ? 0 : (m_areaId <= 3 ? 1 : (m_areaId <= 5 ? 2 : 3));
        m_battle.enemyCount = kCountByArea[tier];
        for (int i = 0; i < m_battle.enemyCount; ++i) {
            FwBattleMember& e = m_battle.e[i];
            e.isEnemy = true; e.charId = -1;
            e.maxHp = 40 + tier * 20;
            e.hp = e.maxHp;
            e.cd = kCooldown + kEnemyStagger * i;
        }
        // the party (level-scaled health; real per-char HP comes with the
        // file-setup step)
        for (int i = 0; i < 4; ++i) {
            FwBattleMember& p = m_battle.p[i];
            p.isEnemy = false; p.charId = i;
            p.maxHp = 100 + m_zoneLevel * 20;
            p.hp = p.maxHp;
            p.cd = kCooldown + kEnemyStagger * i;
        }
        m_battle.result = 0;
        m_battle.endT = 0.0f;
        m_cardT = 0.0f;
        Sfx("battle theme - Adventure ahead -v2", true, 0, 100);
        return;
    }

    // the damage floats age out after 0.9 s
    for (int i = 0; i < 8; ++i) {
        if (m_dmgAmt[i] && m_time - m_dmgBorn[i] > 0.9f) m_dmgAmt[i] = 0;
    }

    if (m_battle.result == 0) {
        // cooldowns
        for (int i = 0; i < 4; ++i) {
            if (!m_battle.p[i].dead) m_battle.p[i].cd -= dt;
            if (i < m_battle.enemyCount && !m_battle.e[i].dead) m_battle.e[i].cd -= dt;
        }

        // A: the ready party member with the lowest cooldown strikes now
        if (in.aPressed) {
            int best = -1; f32 bestCd = 1e9f;
            for (int i = 0; i < 4; ++i) {
                if (!m_battle.p[i].dead && m_battle.p[i].cd <= 0.0f && m_battle.p[i].cd < bestCd) {
                    bestCd = m_battle.p[i].cd; best = i;
                }
            }
            if (best >= 0) {
                // first living enemy
                int tgt = -1;
                for (int i = 0; i < m_battle.enemyCount; ++i)
                    if (!m_battle.e[i].dead) { tgt = i; break; }
                if (tgt >= 0) {
                    const i32 dmg = 12 + rand() % 9;
                    m_battle.e[tgt].hp -= dmg;
                    m_battle.p[best].cd = kCooldown;
                    m_dmgX[m_dmgNext] = (f32)tgt; m_dmgY[m_dmgNext] = 0.0f;
                    m_dmgAmt[m_dmgNext] = dmg; m_dmgBorn[m_dmgNext] = m_time;
                    m_dmgEnemy[m_dmgNext] = false;
                    m_dmgNext = (m_dmgNext + 1) & 7;
                    if (m_battle.e[tgt].hp <= 0) { m_battle.e[tgt].hp = 0; m_battle.e[tgt].dead = true; }
                }
            }
        }

        // enemy AI: random living target
        for (int i = 0; i < m_battle.enemyCount; ++i) {
            FwBattleMember& e = m_battle.e[i];
            if (e.dead || e.cd > 0.0f) continue;
            int tgt = -1;
            for (int tries = 0; tries < 4 && tgt < 0; ++tries) {
                const int c = rand() % 4;
                if (!m_battle.p[c].dead) tgt = c;
            }
            if (tgt < 0) { e.cd = kCooldown; continue; }
            const i32 dmg = 8 + rand() % 7;
            m_battle.p[tgt].hp -= dmg;
            e.cd = kCooldown;
            m_dmgX[m_dmgNext] = (f32)tgt; m_dmgY[m_dmgNext] = 0.0f;
            m_dmgAmt[m_dmgNext] = dmg; m_dmgBorn[m_dmgNext] = m_time;
            m_dmgEnemy[m_dmgNext] = true;
            m_dmgNext = (m_dmgNext + 1) & 7;
            if (m_battle.p[tgt].hp <= 0) { m_battle.p[tgt].hp = 0; m_battle.p[tgt].dead = true; }
        }

        // results (GetGameOver: all player slots dead)
        bool anyEnemy = false, anyPlayer = false;
        for (int i = 0; i < m_battle.enemyCount; ++i) if (!m_battle.e[i].dead) anyEnemy = true;
        for (int i = 0; i < 4; ++i) if (!m_battle.p[i].dead) anyPlayer = true;
        if (!anyEnemy) {
            m_battle.result = 1;
            m_fazTokens = (u16)(m_fazTokens + 15 * m_battle.enemyCount);
            m_saveDirty = true;
            SfxStop("battle theme - Adventure ahead -v2");
            Sfx("Victory", false, 1, 100);
        } else if (!anyPlayer) {
            m_battle.result = 2;
            SfxStop("battle theme - Adventure ahead -v2");
        }
    } else {
        m_battle.endT += dt;
        if (m_battle.endT >= kResultHold) {
            m_battleInit = false;
            if (m_battle.result == 1) {
                RequestScreen(SCR_WORLD);      // the theme restarts in TickWorld
            } else {
                RequestScreen(SCR_GAMEOVER);   // frame 6: the death plate
            }
        }
    }
}

// the floating damage numbers (battle-space slots -> the renderer maps)
void FNaFWorldGame::GetDamage(i32 i, f32& x, f32& y, i32& amount, f32& age, bool& enemyHit) const {
    x = y = 0.0f; amount = 0; age = 0.0f; enemyHit = false;
    if (i < 0 || i > 7) return;
    x = m_dmgX[i]; y = m_dmgY[i];
    amount = m_dmgAmt[i];
    age = m_time - m_dmgBorn[i];
    enemyHit = m_dmgEnemy[i];
}

void FNaFWorldGame::OpenChest(i32 i) {
    if (i < 0 || i >= kFwChestCount) return;
    kFwChests[i].opened = 1;
    m_fazTokens = (u16)(m_fazTokens + (kFwChests[i].big ? 25 : 5));
    // v2.91 interim: chest i also awards chip i for the first seven (the
    // C-Edition chip chests; the exe's own chip-box positions are a later
    // mine — labeled)
    if (i < 7) m_chipsOwned |= (1u << i);
    m_saveDirty = true;
    Sfx("Cash_Register_Open_01", false, 1, 100);
}

i32 FNaFWorldGame::GetChestCount() const { return kFwChestCount; }

void FNaFWorldGame::GetChest(i32 i, f32& tx, f32& ty, bool& big, bool& opened) const {
    tx = ty = 0.0f; big = false; opened = true;
    if (i < 0 || i >= kFwChestCount) return;
    tx = ((f32)kFwChests[i].x + 12.0f) / 50.0f;   // +12: the sprite hotspot
    ty = ((f32)kFwChests[i].y + 13.0f) / 50.0f;
    big    = kFwChests[i].big != 0;
    opened = kFwChests[i].opened != 0;
}

// ------------------------------------------------------------
// the fnafworld save bridge
// ------------------------------------------------------------
void FNaFWorldGame::ApplyProgress(const Progress::GameProgressFW& p) {
    m_zoneLevel = p.zoneLevel;
    m_fazTokens = (u16)p.fazTokens;
    m_lastX = (f32)p.lastX;  m_lastY = (f32)p.lastY;
    m_lastZone = p.lastFrame;
    // v2.91 blob v2: the party/inventory (v1 files keep the classic four)
    for (int i = 0; i < 4; ++i) m_party[i] = p.party[i];
    m_chipsOwned = p.chipsOwned;
    m_chipsEq    = p.chipsEq;
    m_bytesOwned = p.bytesOwned;
    // World.c ResetWorld: position = Last_Location + 0.5 - size/2 (the save
    // stores the TILE the centre sits in, not the hitbox corner)
    m_freddyX = p.lastX + 0.5f - kFreddyW * 0.5f;
    m_freddyY = p.lastY + 0.5f - kFreddyH * 0.5f;
}

void FNaFWorldGame::FillProgress(Progress::GameProgressFW& p) const {
    p.zoneLevel = m_zoneLevel;
    p.fazTokens = m_fazTokens;
    // World.c SaveButtonPress/WriteSave: the save stores the FREDDY CENTRE
    // (position + size/2) as the TILE. The blob is integer, so the store
    // truncates — which makes the ApplyProgress round-trip exact (the
    // C-Edition float store drifted +0.5 tile per save; noted).
    p.lastX = (i32)(m_freddyX + kFreddyW * 0.5f);
    p.lastY = (i32)(m_freddyY + kFreddyH * 0.5f);
    p.lastFrame = (i32)m_screen;
    for (int i = 0; i < 4; ++i) p.party[i] = m_party[i];
    p.chipsOwned = m_chipsOwned;
    p.chipsEq    = m_chipsEq;
    p.bytesOwned = m_bytesOwned;
}

void FNaFWorldGame::WipeSave() {
    Progress::WipeFW();
    m_zoneLevel = 1;  m_fazTokens = 0;
    m_lastX = 0.0f;   m_lastY = 0.0f;  m_lastZone = 0;
}

} // namespace fnaf