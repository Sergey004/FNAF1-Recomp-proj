/**
 * FNaF3Game.cpp: v2.61 — FNAF3 night loop (see FNaF3Game.h).
 * Dump authority: Dumps/Five Nights at Freddys 3/Events/frame_3_Frame 1_events.txt
 * (773 groups). Timer convention of this project: the dumped "timer N"
 * values are RAW MILLISECONDS (the "~Ns" labels divide by 50 and read 20x
 * long) — e.g. the hour timer 40000/60000 is 40/60 s, the Springtrap move
 * tick 1000 is 1 s. Deviations are labeled inline.
 */

#include "FNaF3Game.h"
#include "XdkCompat.h"   // Snprintf — XDK CRT predates C99 snprintf
#include <stdlib.h>

namespace fnaf {

// ------------------------------------------------------------
// construction / flow
// ------------------------------------------------------------

FNaF3Game::FNaF3Game() {
    ResetToTitle();
}

void FNaF3Game::ResetToTitle() {
    m_screen = SCR_DISCLAIMER;
    m_night = 1;
    m_timeOfNight = 0;
    m_hourClock = 0.0f;
    m_cardT = 0.0f;
    m_time = 0.0f;
    m_optionSelected = 0;
    m_lastNight = 1;
    m_beat5 = false;
    m_pan = 488.0f;
    // a fresh boot still has nothing behind the title until the FNAF3 INI
    // save system lands (same session-unlock terms as FNAF2 v2.58)
    m_lastNight = 1;
    m_beat5 = false;
    m_stRoom = R3_OFFMAP;
    m_viewing = 0;
    m_gotYou = 0;
    m_scareT = 0.0f;
    m_frozen = 0.0f;
    m_scareCooldown = 0.0f;
}

void FNaF3Game::StartNight(i32 night) {
    m_night = night;
    InitNightState();
    m_screen = SCR_NIGHTSTART;
    m_cardT = 0.0f;
}

void FNaF3Game::InitNightState() {
    // hour 12 (g500-style), meters (g624), AI (g503-508), stare limit
    // (g631-636), Springtrap start (g139-144 + the night-1 off-map park g623)
    m_timeOfNight = 0;
    m_hourClock = 0.0f;
    m_ai = (m_night < 2) ? 0 : (m_night < 6 ? m_night : 7);
    static const i32 kLimit[6] = { 100, 90, 80, 70, 60, 50 };
    m_timeLimit = kLimit[m_night < 1 ? 0 : (m_night > 6 ? 5 : m_night - 1)];
    m_aggressive = false;
    m_aggrT = 0.0f;
    m_aggrRollT = 0.0f;

    m_audioMeter  = -3 - (rand() % 4);
    m_cameraMeter = -3 - (rand() % 4);
    m_ventMeter   = -2 - (rand() % 4);
    m_camUseAcc = 0.0f;
    m_ventErrT = 0.0f;
    m_ignoreAcc = 0.0f;
    m_boAcc = 0.0f;
    m_blackoutAlpha = 0.0f;
    m_hallucinating = false;
    m_moveStatic = 0.0f;

    m_stRoom = R3_OFFMAP;
    if (m_night >= 2) {
        static const i32 kStart[5] = { R3_10, R3_09, R3_08, R3_07, R3_06 };
        m_stRoom = kStart[rand() % 5];          // g139-144: Random(5)+1 rooms
    }
    m_moveCounter = 0.0f;
    m_totalTurns = 0;
    m_action = 0;
    m_queued = false;
    m_playEffect = 0;
    m_runPastT = 0.0f;
    m_bigScare = false;
    m_picRandom = rand() % 2;
    m_picT = 0.0f;

    m_viewing = 0;
    m_youIn = 1;
    m_mapVent = false;
    m_selIdx = 0;
    m_sealedVent = 0;
    m_sealTarget = 0;
    m_sealProgress = 0.0f;
    m_sealDuration = 0.0f;
    m_playCounter = 7;
    m_playT = 0.0f;
    m_lureT = -1.0f;
    m_lureCountdown = -1;
    m_lureTarget = R3_OFFMAP;

    m_panelOpen = false;
    m_panelCursor = 1;
    m_rebooting = 0;
    m_rebootProgress = 0.0f;
    m_rebootT = 0.0f;

    m_phBB = 0; m_phBBStare = 0.0f;
    m_phFoxy = 0; m_foxyScareT = 0.0f;
    m_phMangle = 0; m_phMangleStare = 0.0f;
    m_phPuppet = 0; m_phPuppetStare = 0.0f; m_phPuppetRushT = 0.0f;
    m_phChica = 0; m_phChicaStare = 0.0f;
    m_phGF = 0; m_phGFT = 0.0f; m_phGFStepT = 0.0f; m_phGFWalkT = 0.0f;
    m_phGFStare = 0.0f;
    m_shadowFreddy = false;
    m_phantomRollT = 0.0f;
    m_scareCooldown = 0.0f;
    m_frozen = 0.0f;
    m_whiteFlash = 0.0f;

    m_gotYou = 0;
    m_gotYouT = 0.0f;
    m_scareT = 0.0f;
    m_scareImg = 0;
    m_pan = 488.0f;
}

// ------------------------------------------------------------
// helpers
// ------------------------------------------------------------

void FNaF3Game::Sfx(const char* s, bool loop, i32 ch, i32 vol) {
    if (audio.play) audio.play(s, loop, ch, vol);
}
void FNaF3Game::SfxStop(const char* s) {
    if (audio.stop) audio.stop(s);
}

// dump "camera screen" act #17 feed table: empty / Springtrap per cam
i32 FNaF3Game::FeedImg(i32 cam, bool springtrap) {
    static const i32 kEmpty[16] = { 0, 17, 14, 15, 16, 21, 12, 13, 23, 24, 25, 45, 46, 47, 48, 49 };
    static const i32 kSt[16]    = { 0, 30, 43, 36, 37, 44, 41, 42, 38, 39, 40, 50, 51, 52, 53, 54 };
    if (cam < 1 || cam > 15) return 0;
    return springtrap ? kSt[cam] : kEmpty[cam];
}

static bool RoomIsVent(i32 r) {
    return r >= FNaF3Game::R3_V11 && r <= FNaF3Game::R3_V15;
}

// Springtrap's current cam id for the feed (0 = not on a camera)
static i32 RoomToCam(i32 r) {
    if (r >= FNaF3Game::R3_01 && r <= FNaF3Game::R3_10) return r;
    if (RoomIsVent(r)) return r - FNaF3Game::R3_V11 + 11;
    return 0;
}

// ------------------------------------------------------------
// tick
// ------------------------------------------------------------

void FNaF3Game::Tick(f32 dt, const FNaF3Inputs& in) {
    m_time += dt;

    switch (m_screen) {
        case SCR_DISCLAIMER:
            m_cardT += dt;
            if (m_cardT >= 3.5f || in.aPressed || in.bPressed) {
                m_screen = SCR_TITLE; m_cardT = 0.0f;
            }
            break;

        case SCR_TITLE: {
            // 4 dump rows (new game / load game / nightmare / extra)
            if (in.upPressed)
                m_optionSelected = (m_optionSelected + 3) % 4;
            if (in.downPressed)
                m_optionSelected = (m_optionSelected + 1) % 4;
            if (in.aPressed) {
                switch (m_optionSelected) {
                    case 0: StartNight(1); break;                       // new game
                    case 1: StartNight(m_lastNight < 1 ? 1 : m_lastNight); break; // load
                    case 2: if (m_beat5) StartNight(6); break;          // nightmare
                    case 3: /* extras menu — later wave */ break;
                }
            }
            break;
        }

        case SCR_NIGHTSTART:
            m_cardT += dt;
            if (m_cardT >= 2.5f) { m_screen = SCR_OFFICE; m_cardT = 0.0f; }
            break;

        case SCR_OFFICE:
            TickOffice(dt, in);
            break;

        case SCR_STATIC6:
            m_cardT += dt;
            if (m_cardT >= 5.0f) {
                SfxStop("snd_stare");
                m_screen = SCR_NEXTDAY;
                m_cardT = 0.0f;
            }
            break;

        case SCR_NEXTDAY:
            m_cardT += dt;
            if (m_cardT >= 3.5f) {
                // "next day" (g): night number += 1, level saved (the INI
                // save is a later wave — session memory only)
                m_night += 1;
                m_lastNight = m_night > 6 ? 6 : m_night;
                if (m_night - 1 == 5) m_beat5 = true;   // nightmare unlocked
                m_screen = SCR_TITLE;
                m_cardT = 0.0f;
            }
            break;
    }
}

// ------------------------------------------------------------
// office
// ------------------------------------------------------------

void FNaF3Game::TickOffice(f32 dt, const FNaF3Inputs& in) {
    // ---- office pan (dump scroll moves +-8/frame = 480 px/s) ----
    if (m_viewing == 0 && m_gotYou == 0)
        m_pan += in.lookDir * 480.0f * dt;
    if (m_pan < 0.0f)   m_pan = 0.0f;
    if (m_pan > 976.0f) m_pan = 976.0f;

    // ---- monitor flip (LB); frozen locks it (g594) ----
    if (in.lbPressed && m_frozen <= 0.0f && m_gotYou == 0) {
        if (m_viewing == 0) {
            m_viewing = 2;
            // Phantom Foxy rolls on every monitor open (g722-725; night 2
            // has no roll in the dump)
            bool foxyRoll = false;
            if (m_night == 1)      foxyRoll = (rand() % 1000) == 0;
            else if (m_night == 3) foxyRoll = (rand() % 50) == 0;
            else if (m_night == 4) foxyRoll = (rand() % 25) == 0;
            else if (m_night >= 5) foxyRoll = (rand() % 10) == 0;
            if (foxyRoll) { m_phFoxy = 1; m_foxyScareT = 0.0f; }
        } else if (!m_panelOpen) {
            m_viewing = 0;
            // leaving the monitor cancels the seal (g450)
            m_sealTarget = 0;
        }
    }
    // B also drops the monitor
    if (in.bPressed && m_viewing != 0 && !m_panelOpen) {
        m_viewing = 0;
        m_sealTarget = 0;
    }

    // ---- map toggle (RB, monitor only) ----
    if (m_viewing != 0 && in.rbPressed && !m_panelOpen) {
        m_mapVent = !m_mapVent;
        if (!m_mapVent) m_sealTarget = 0;   // g449: toggling off cancels
    }

    // ---- maintenance panel (Y on the monitor) ----
    if (m_viewing != 0 && in.yPressed) {
        m_panelOpen = !m_panelOpen;
        if (m_panelOpen) { m_panelCursor = 1; m_rebooting = 0; }
        else             { m_rebooting = 0; SfxStop("snd_wait"); }
    }
    if (m_panelOpen) {
        if (in.upPressed)   { m_panelCursor = m_panelCursor > 1 ? m_panelCursor - 1 : 5; Sfx("snd_select", false, 8, 60); }
        if (in.downPressed) { m_panelCursor = m_panelCursor < 5 ? m_panelCursor + 1 : 1; Sfx("snd_select", false, 8, 60); }
        if (in.aPressed && m_rebooting == 0) {
            if (m_panelCursor == 5) { m_panelOpen = false; }
            else {
                m_rebooting = m_panelCursor;         // 1..4
                m_rebootProgress = 0.0f;
                m_rebootT = 0.0f;
                Sfx("snd_wait", true, 8, 60);
            }
        }
    } else if (m_viewing != 0) {
        // ---- map selector + camera select + actions ----
        if (in.upPressed)    m_selIdx = (m_selIdx + 15) % 16;
        if (in.downPressed)  m_selIdx = (m_selIdx + 1) % 16;
        if (in.leftPressed)  m_selIdx = (m_selIdx + 15) % 16;
        if (in.rightPressed) m_selIdx = (m_selIdx + 1) % 16;
        const bool ventMap = m_mapVent;
        // selector rows: on the room map indices 0..9 = cams 1..10 (+15 =
        // toggle); on the vent map indices 0..4 = cams 11..15 (+15 = toggle)
        const i32 selCam = ventMap ? (m_selIdx < 5 ? m_selIdx + 11 : 0)
                                   : (m_selIdx < 10 ? m_selIdx + 1 : 0);
        if (in.aPressed) {
            if (m_selIdx == 15) {
                m_mapVent = !m_mapVent;          // the map toggle button
                if (!m_mapVent) m_sealTarget = 0;
            } else if (selCam != 0 && selCam != m_youIn) {
                m_youIn = selCam;                // g: you in := C/D
                Sfx("snd_feed", false, 10, 80);
            }
        }
        if (in.xPressed && selCam != 0) {
            if (!ventMap) {
                PlayLure(selCam);                // audio lure (room map)
            } else if (m_sealTarget == 0 && m_youIn >= 11 && m_youIn <= 15) {
                // seal the selected vent (g: 50+Random(50) frames)
                m_sealTarget = m_youIn;
                m_sealDuration = (f32)(50 + rand() % 50) / 60.0f;
                m_sealProgress = 0.0f;
            }
        }
    }

    // ---- sub-systems ----
    TickClock(dt);
    TickSpringtrap(dt);
    TickLure(dt);
    TickPanel(dt);
    TickPhantoms(dt);
    TickGotYou(dt);

    // shared decays
    if (m_scareCooldown > 0.0f) m_scareCooldown -= dt;
    if (m_frozen > 0.0f)        m_frozen -= dt;
    if (m_runPastT > 0.0f)      m_runPastT -= dt;
    if (m_whiteFlash > 0.0f)    m_whiteFlash -= dt * 3.0f;
    if (m_moveStatic > 0.0f)    m_moveStatic -= dt * 2.0f;
    if (m_foxyScareT >= 0.0f && m_phFoxy == 0) m_foxyScareT = -1.0f;
}

// ------------------------------------------------------------
// clock (g496-502): raw ms -> 40 s night 1, 60 s nights 2+
// ------------------------------------------------------------

void FNaF3Game::TickClock(f32 dt) {
    const f32 kHour = (m_night < 2) ? 40.0f : 60.0f;
    m_hourClock += dt;
    if (m_hourClock >= kHour) {
        m_hourClock -= kHour;
        m_timeOfNight += 1;
        m_aggressive = false;               // g145/g758 reset on the roll
        if (m_timeOfNight >= 4) m_aggressive = true;   // g744
        // forced phantom windows (g523/531/524-528)
        if (m_timeOfNight == 3 && m_night >= 2 && m_phBB == 0) m_phBB = 1;
        if (m_timeOfNight == 4 && m_night >= 3 && m_phGF == 0) m_phGF = 1;
        if (m_timeOfNight == 5) {
            if (m_night >= 2 && m_phMangle == 0) m_phMangle = 2;
            if (m_night >= 4 && m_phPuppet == 0) m_phPuppet = 2;
            if (m_night >= 3 && m_phChica  == 0) m_phChica  = 2;
        }
        if (m_timeOfNight >= 6) {
            m_screen = SCR_STATIC6;         // g502 -> frame 4 "static"
            m_cardT = 0.0f;
            SfxStop("snd_tablefan");
            Sfx("snd_stare", true, 9, 100);
        }
    }
}

// ------------------------------------------------------------
// Springtrap (g146-219): 1 s move ticks, threshold roll, room graph
// ------------------------------------------------------------

void FNaF3Game::TickSpringtrap(f32 dt) {
    if (m_gotYou != 0 || m_scareT > 0.0f) return;

    // pic random re-roll (g341-342): every 10 s
    m_picT += dt;
    if (m_picT >= 10.0f) { m_picT = 0.0f; m_picRandom = rand() % 2; }

    // aggressive drivers: vent broken (g299), frozen (g607), the 5 s roll
    // (g516), the 15 s reset (g145)
    m_aggrT += dt;
    if (m_aggrT >= 15.0f) { m_aggrT = 0.0f; m_aggressive = false; }
    if (m_ventMeter <= -10 || m_frozen > 0.0f) m_aggressive = true;
    m_aggrRollT += dt;
    if (m_aggrRollT >= 5.0f) {
        m_aggrRollT = 0.0f;
        if ((rand() % 5) < m_ai) m_aggressive = true;
    }

    if (m_stRoom == R3_OFFMAP) return;      // night 1: he does not play

    // move tick (g146/148): every 1 s; threshold
    //   move counter > (10 - AI - aggressive) + Random(15) - total turns
    m_moveCounter += dt;
    const i32 threshold = (10 - m_ai - (m_aggressive ? 1 : 0)) + (rand() % 15) - m_totalTurns;
    if ((i32)m_moveCounter > threshold) {
        m_moveCounter = 0.0f;
        // g149: action := Random(3) + aggressive + 1  (1..4, 2..5 aggr)
        m_action = 1 + rand() % 3 + (m_aggressive ? 1 : 0);
        if (m_action == 1) {                // g150: stall — the next window
            m_totalTurns += 1;              // comes sooner
            m_action = 0;
        } else {
            ExecuteAction(m_action);
            m_action = 0;
        }
    }

    // ST1 -> ST2 needs a screen open (g200: queued + viewing a screen)
    if (m_queued && (m_viewing >= 2 || m_panelOpen)) {
        m_queued = false;
        m_stRoom = R3_ST2;
    }
}

void FNaF3Game::ExecuteAction(i32 act) {
    const bool screen = (m_viewing >= 2 || m_panelOpen);
    const bool blackout = m_blackoutAlpha > 250.0f;
    i32 dest = m_stRoom;
    bool ventSound = false;

    switch (m_stRoom) {
        case R3_10: dest = (act == 4) ? R3_V14 : R3_09; ventSound = (act == 4); break;
        case R3_09: dest = (act == 2) ? R3_10 : (act == 3) ? R3_08 : R3_V11;
                    ventSound = (act == 4); break;
        case R3_08: dest = (act == 2) ? R3_09 : (act == 3) ? R3_07 : R3_05; break;
        case R3_07: dest = (act == 2) ? R3_08 : (act == 3) ? R3_06 : R3_V12;
                    ventSound = (act == 4); break;
        case R3_06: dest = (act == 2) ? R3_07 : R3_05; break;
        case R3_05: dest = (act == 2) ? R3_06 : (act == 3) ? R3_02 :
                     (m_picRandom == 0 ? R3_04 : R3_V13);
                    ventSound = (act == 4 && m_picRandom != 0); break;
        case R3_04: dest = (act == 2) ? R3_02 : R3_03; break;
        case R3_03: dest = (act == 2) ? R3_04 : R3_ST1; break;
        case R3_02: dest = (act == 2) ? R3_05 : (act == 3) ? R3_04 :
                     (m_picRandom == 0 ? R3_ST1 : R3_V15);
                    ventSound = (act == 4 && m_picRandom != 0); break;
        case R3_ST1: if (act > 2) { m_queued = true; return; } dest = m_stRoom; break;
        case R3_ST2: if (act > 2) {
                        m_stRoom = R3_ST3;
                        m_runPastT = 0.6f;      // g197: the hall dash sprite
                        Sfx("snd_run", false, 5, 100);
                        return;
                     }
                     dest = m_stRoom; break;
        case R3_ST3: if (blackout) { dest = R3_ST4; }
                     else if (act == 2 && screen) dest = R3_01;
                     else if (act > 2 && screen) dest = R3_ST4;
                     else dest = m_stRoom; break;
        case R3_01:  if (blackout) { dest = R3_ST4; }
                     else if (act == 2 && screen) dest = R3_ST3;
                     else if (act > 2 && screen) dest = R3_ST4;
                     else dest = m_stRoom; break;
        case R3_ST4: if (act > 1) {
                        if (blackout)          dest = R3_GY;
                        else if (m_panelOpen)  dest = R3_GY;     // g513/515
                        else if (m_viewing >= 2) { dest = R3_GY2; m_bigScare = true; }
                        else dest = m_stRoom;
                     } else dest = m_stRoom; break;
        default: dest = m_stRoom; break;
    }

    if (dest == m_stRoom) return;
    if (RoomIsVent(dest)) { EnterVent(dest); return; }

    m_stRoom = dest;
    m_totalTurns = 0;
    // walk sounds only from night 2 (g151)
    if (m_night > 1) {
        char walk[16];
        Snprintf(walk, sizeof(walk), "snd_walk%d", 1 + rand() % 7);
        Sfx(walk, false, 5, 100);
    }
}

void FNaF3Game::EnterVent(i32 vent) {
    // g469-478: sealed -> bounce back with the quiet crawl, open -> resolve
    const bool sealed = (m_sealedVent == vent);
    m_playEffect = sealed ? (1 + rand() % 2) : (3 + rand() % 2);
    switch (m_playEffect) {
        case 1: Sfx("snd_vent_quiet1",  false, 7, 100); break;
        case 2: Sfx("snd_vent_quiet2",  false, 7, 100); break;
        case 3: Sfx("snd_vent_closer1", false, 7, 100); break;
        case 4: Sfx("snd_vent_louder2", false, 7, 100); break;
    }
    if (sealed) {
        switch (vent) {
            case R3_V11: m_stRoom = R3_09; break;
            case R3_V12: m_stRoom = R3_07; break;
            case R3_V13: m_stRoom = R3_05; break;
            case R3_V14: m_stRoom = R3_10; break;
            case R3_V15: m_stRoom = R3_02; break;
        }
    } else {
        switch (vent) {
            case R3_V11: m_stRoom = R3_ST3; break;
            case R3_V12: m_stRoom = R3_ST3; break;
            case R3_V13: m_stRoom = R3_ST1; break;
            case R3_V14: m_stRoom = R3_GY2; m_bigScare = false; break;
            case R3_V15: m_stRoom = R3_GY2; m_bigScare = false; break;
        }
    }
    m_totalTurns = 0;
}

// ------------------------------------------------------------
// audio lure (g224-267)
// ------------------------------------------------------------

void FNaF3Game::PlayLure(i32 cam) {
    if (m_playCounter < 7) return;             // g224/225: charge == 7
    if (m_audioMeter <= -10) return;           // audio system broken
    m_playCounter = 0;
    m_audioMeter -= m_ai;                      // audio damage per play
    switch (rand() % 3) {                      // the echo sample roll
        case 0: Sfx("snd_echo1",  false, 7, 100); break;
        case 1: Sfx("snd_echo3b", false, 7, 100); break;
        case 2: Sfx("snd_echo4b", false, 7, 100); break;
    }
    // DEVIATION (labeled): the dump creates "lure" without a position — the
    // overlap conditions are against the cam buttons, so the lure plays in
    // the SELECTED cam room.
    if ((rand() % 7) + 1 == 1) return;         // g231: 1-in-7 fizzle
    static const i32 kAdj[11][5] = {
        { 0, 0, 0, 0, 0 },
        { R3_ST4, 0, 0, 0, 0 },                          // 01
        { R3_ST1, R3_03, R3_04, R3_05, 0 },              // 02
        { R3_02, R3_04, 0, 0, 0 },                       // 03
        { R3_02, R3_03, 0, 0, 0 },                       // 04
        { R3_02, R3_06, R3_07, R3_08, 0 },               // 05
        { R3_05, R3_07, 0, 0, 0 },                       // 06
        { R3_06, R3_08, 0, 0, 0 },                       // 07
        { R3_05, R3_07, R3_09, 0, 0 },                   // 08
        { R3_08, R3_10, 0, 0, 0 },                       // 09
        { R3_09, 0, 0, 0, 0 }                            // 10
    };
    for (i32 i = 0; i < 5 && kAdj[cam][i] != 0; ++i) {
        if (m_stRoom == kAdj[cam][i]) {
            m_lureT = 2.0f;                              // g267: 2 s life
            m_lureCountdown = rand() % 100;              // g: completion frames
            m_lureTarget = cam;                          // move TO the cam room
            return;
        }
    }
}

void FNaF3Game::TickLure(f32 dt) {
    if (m_playCounter < 7) {
        m_playT += dt;
        if (m_playT >= 1.5f) { m_playT = 0.0f; m_playCounter += 1; }   // g224
    }
    if (m_lureT > 0.0f) {
        m_lureT -= dt;
        if (m_lureCountdown >= 0) {
            m_lureCountdown -= 1;                        // 1/frame (60 fps)
            if (m_lureCountdown < 0 && m_lureT > 0.0f && m_stRoom != R3_OFFMAP) {
                m_stRoom = m_lureTarget;                 // g256-266
                m_moveCounter = 0.0f;
                m_totalTurns = 0;
                m_moveStatic = 1.0f;                     // smallstatic flare
            }
        }
        if (m_lureT <= 0.0f) m_lureCountdown = -1;
    }
}

// ------------------------------------------------------------
// maintenance panel + meters (g274-369, 624-641, 746-752)
// ------------------------------------------------------------

void FNaF3Game::TickPanel(f32 dt) {
    // camera degradation: 12 s of monitor use -> camera meter -= AI (g637)
    if (m_viewing >= 2) {
        m_camUseAcc += dt;
        if (m_camUseAcc >= 12.0f) {
            m_camUseAcc = 0.0f;
            m_cameraMeter -= m_ai;
        }
    }

    // vent decay per AI (g330-334): 12/10/9/8/6 s
    if (m_ai >= 2) {
        static const f32 kVent[6] = { 12.0f, 12.0f, 10.0f, 9.0f, 8.0f, 6.0f };
        const i32 vi = m_ai > 5 ? 5 : m_ai;
        m_ventErrT += dt;
        if (m_ventErrT >= kVent[vi]) {
            m_ventErrT = 0.0f;
            m_ventMeter -= 1;
        }
    }

    // panel ignored: after 10 s unattended the vent rots 1/s + aggressive
    // (g746-749); night 1 is spared
    if (m_viewing <= 1 && !m_panelOpen) {
        m_ignoreAcc += dt;
        if (m_ignoreAcc > 10.0f && m_night != 1) {
            m_ventMeter -= 1;
            m_aggressive = true;
            m_ignoreAcc -= 1.0f;
        }
    } else {
        m_ignoreAcc = 0.0f;
    }

    // reboot progress (g303/310): +1..2 per second to 10
    if (m_rebooting != 0) {
        m_rebootT += dt;
        if (m_rebootT >= 1.0f) {
            m_rebootT = 0.0f;
            m_rebootProgress += (f32)(1 + rand() % 2);
            if (m_rebootProgress >= 10.0f) {
                if (m_rebooting == 1 || m_rebooting == 4) m_audioMeter  = 0;
                if (m_rebooting == 2 || m_rebooting == 4) m_cameraMeter = 0;
                if (m_rebooting == 3 || m_rebooting == 4) m_ventMeter   = 0;
                m_rebooting = 0;
                SfxStop("snd_wait");
                Sfx("snd_done", false, 8, 80);
            }
        }
    }

    // the vent-error blackout pipeline (g344-362): frames at 60 fps
    const f32 kBoThreshold = (f32)(2000 - m_ai * 200);
    if (m_ventMeter <= -10) {
        m_boAcc += dt * 60.0f;
        if (m_boAcc > kBoThreshold && m_blackoutAlpha < 255.0f)
            m_blackoutAlpha += dt * 100.0f;
        else if (m_boAcc <= kBoThreshold && m_blackoutAlpha > 0.0f)
            m_blackoutAlpha -= dt * 100.0f;
        // hallucinations (g345-353)
        const bool hallu = m_boAcc > (f32)(1000 - m_ai * 100);
        if (hallu && !m_hallucinating) Sfx("snd_alarm", false, 4, 100);
        m_hallucinating = hallu;
    } else {
        m_boAcc = 0.0f;
        if (m_blackoutAlpha > 0.0f) m_blackoutAlpha -= dt * 100.0f;
        m_hallucinating = false;
    }
    if (m_blackoutAlpha < 0.0f) m_blackoutAlpha = 0.0f;
    if (m_blackoutAlpha > 255.0f) m_blackoutAlpha = 255.0f;
}

// ------------------------------------------------------------
// phantoms (g516-622)
// ------------------------------------------------------------

void FNaF3Game::TickPhantoms(f32 dt) {
    if (m_gotYou != 0) return;

    // shared 20 s roll window (g522-530 use timer 20000/60000)
    m_phantomRollT += dt;
    if (m_phantomRollT >= 20.0f) {
        m_phantomRollT = 0.0f;
        if (m_night >= 2 && m_phBB == 0 && m_viewing <= 1 && (rand() % 10) + 1 <= m_ai)
            m_phBB = 1;
        if (m_night >= 2 && m_phMangle == 0 && m_youIn != 4 && (rand() % 7) + 1 <= m_ai)
            m_phMangle = 2;
        if (m_night >= 4 && m_phPuppet == 0 && m_youIn != 8 && (rand() % 10) + 1 <= m_ai)
            m_phPuppet = 2;
        if (m_night >= 3 && m_phChica == 0 && m_youIn != 7 && (rand() % 10) + 1 <= m_ai)
            m_phChica = 2;
        if (m_night >= 3 && m_phGF == 0 && (rand() % 12) + 1 <= m_ai)
            m_phGF = 1;
        if (!m_shadowFreddy && m_viewing <= 1 && (rand() % 10000) == 0)
            m_shadowFreddy = true;                 // g621
    }

    // ---- Phantom BB: shows while you watch cams 01/07/09/10 (g532-537) ----
    if (m_phBB == 1 && m_viewing >= 2 &&
        (m_youIn == 1 || m_youIn == 7 || m_youIn == 9 || m_youIn == 10))
        m_phBB = 2;
    if (m_phBB == 2) {
        if (m_viewing >= 2 &&
            (m_youIn == 1 || m_youIn == 7 || m_youIn == 9 || m_youIn == 10)) {
            m_phBBStare += dt * 60.0f;
            if (m_phBBStare > (f32)m_timeLimit) {
                PhantomScare(1);                   // scream3 + drop monitor
                m_phBB = 0;                        // disabled for the night
                m_phBBStare = 0.0f;
            }
        }
    }

    // ---- Phantom Foxy: office overlay; scare when panned onto him ----
    if (m_phFoxy == 1) {
        if (m_viewing == 0 && m_scareCooldown <= 0.0f &&
            m_pan > 580.0f && m_pan < 820.0f) {    // DEVIATION: fixed spot
            m_foxyScareT = 0.6f;
            PhantomScare(2);
            m_phFoxy = 0;
        }
    }
    if (m_foxyScareT > 0.0f) m_foxyScareT -= dt;

    // ---- Phantom Mangle: cam 04 feed; stare kills the audio (g571-578) ----
    if (m_phMangle == 2) {
        if (m_viewing >= 2 && m_youIn == 4) {
            m_phMangleStare += dt * 60.0f;
            if (m_phMangleStare > (f32)m_timeLimit) {
                m_audioMeter = -10;                // audio broken for good
                Sfx("snd_garble1", false, 6, 100);
                m_viewing = 0;
                m_phMangle = 0;
            }
        }
    }

    // ---- Phantom Puppet: cam 08 feed; stare -> rush overlay (g580-593) ----
    if (m_phPuppet == 2) {
        if (m_viewing >= 2 && m_youIn == 8) {
            m_phPuppetStare += dt * 60.0f;
            if (m_phPuppetStare > (f32)m_timeLimit) {
                m_phPuppetRushT = 1.0f;            // img 320 sequence
                Sfx("snd_mask", false, 12, 100);
                PhantomScare(3);
                m_phPuppet = 0;
            }
        }
    }
    if (m_phPuppetRushT > 0.0f) m_phPuppetRushT -= dt;

    // ---- Phantom Chica: cam 07 feed -> the office window (g581-604) ----
    if (m_phChica == 2) {
        if (m_viewing >= 2 && m_youIn == 7) {
            m_phChicaStare += dt * 60.0f;
            if (m_phChicaStare > (f32)m_timeLimit) m_phChica = 3;
        }
    } else if (m_phChica == 3) {
        if (m_viewing == 0 && m_pan < 600.0f) {    // g596: scroll < 600
            m_pan -= 180.0f;                       // the view jerk (g604)
            if (m_pan < 0.0f) m_pan = 0.0f;
            PhantomScare(4);
            m_phChica = 0;
        }
    }

    // ---- Golden Freddy: arms, walks the office (g549-557) ----
    if (m_phGF == 1) {
        m_phGFStepT += dt;
        if (m_phGFStepT >= 1.0f) {                 // g552: every 1 s, 50/50
            m_phGFStepT = 0.0f;
            if ((rand() % 2) == 0) { m_phGF = 2; m_phGFWalkT = 0.0f; m_phGFStare = 0.0f; }
        }
    } else if (m_phGF == 2) {
        m_phGFWalkT += dt;
        if (m_viewing <= 1) {
            m_phGFStare += dt * 60.0f;
            if (m_phGFStare > (f32)m_timeLimit * 3.0f) {
                PhantomScare(5);
                m_phGF = 0;
            }
        }
        if (m_phGFWalkT > 3.0f) m_phGF = 0;        // walked through, gone
    }
}

void FNaF3Game::PhantomScare(i32 who) {
    // g539/726/558: scream3, monitor drop, frozen lockout, cooldown, and the
    // forced vent error (g558)
    switch (who) {
        case 1: case 2: case 4: case 5: Sfx("snd_scream3", false, 6, 100); break;
        case 3: /* the puppet rush already played "mask" */ break;
    }
    m_frozen = 2.0f;
    m_scareCooldown = 10.0f;
    m_ventMeter = -10;
    m_whiteFlash = 1.0f;
    if (who != 2) m_viewing = 0;                   // monitor drop
}

// ------------------------------------------------------------
// got you (g204-219, 509-519, 602-606)
// ------------------------------------------------------------

void FNaF3Game::TickGotYou(f32 dt) {
    // Group 219: stage 4 + office view + panned onto his window
    if (m_gotYou == 0 && m_stRoom == R3_ST4 && m_viewing <= 1 &&
        m_pan >= 588.0f && m_pan <= 688.0f)
        m_gotYou = 1;
    // Group 204: he is at GOT YOU and the view is not past him
    if (m_gotYou == 0 && m_stRoom == R3_GY && (m_pan + 512.0f) <= 1300.0f)
        m_gotYou = 1;

    if (m_gotYou == 1) {
        // the view yanks back (g205: -20/frame), 20 frames of it (g209/210),
        // then the scream + the office jumpscare (g206)
        m_pan -= 1200.0f * dt;
        if (m_pan < 0.0f) m_pan = 0.0f;
        m_gotYouT += dt * 60.0f;
        if (m_gotYouT > 20.0f && m_scareT <= 0.0f) {
            Sfx("snd_scream3", false, 6, 100);
            m_scareImg = 778;
            m_scareT = 0.001f;
        }
    } else if (m_gotYou == 2) {
        // GOT YOU 2 (the window scare / the monitor scare): straight to the
        // second jumpscare (g509)
        if (m_scareT <= 0.0f) {
            Sfx("snd_scream3", false, 6, 100);
            m_scareImg = 792;
            m_scareT = 0.001f;
        }
    }

    if (m_scareT > 0.0f) {
        m_scareT += dt;
        if (m_scareT >= 1.6f) {
            // g602-606: the frame reloads itself — an in-place night restart
            ResetNightInPlace();
        }
    }
}

void FNaF3Game::ResetNightInPlace() {
    InitNightState();
    m_screen = SCR_OFFICE;      // straight back into the office (g606)
    Sfx("snd_startday", false, 2, 100);
}

// feed query for the renderer: is Springtrap visible on this cam?
bool FNaF3Game::SpringtrapOnCam(i32 cam) const {
    return RoomToCam(m_stRoom) == cam;
}

} // namespace fnaf
