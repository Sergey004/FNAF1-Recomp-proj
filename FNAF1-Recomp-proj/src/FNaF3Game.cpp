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
    m_cine = 0;
    m_saveDirty = false;
    m_pan = 488.0f;
    // v2.63: the persisted progression + minigame flags
    m_beat6 = false;
    m_goodend = false;
    m_fourthStar = false;
    m_bb = false; m_cake = false;
    m_k1 = false; m_k2 = false; m_k3 = false; m_k4 = false;
    m_fastNights = false; m_ventProof = false; m_hyper = false; m_noCams = false;
    m_rareId = 1;
    m_gameOverRare = 0;
    m_waitT = 0.0f;
    m_died = false;
    m_mg.Clear();
    m_cs.Clear();
    m_ex.Clear();
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
            // frame 0: 2 s / Enter / click; 1/1000 -> rare1 (slot 16), else title
            m_cardT += dt;
            if (m_cardT >= 2.0f || in.aPressed || in.bPressed) {
                if ((rand() % 1000) + 1 == 1) {
                    m_rareId = 1;
                    m_screen = SCR_RARE1;
                } else {
                    m_screen = SCR_TITLE;
                }
                m_cardT = 0.0f;
            }
            break;

        case SCR_TITLE: {
            // 4 dump rows (new game / load game / night 6 / extras). Rows 3-4
            // exist only with beatgame (the renderer hides them; A no-ops).
            if (in.upPressed)
                m_optionSelected = (m_optionSelected + 3) % 4;
            if (in.downPressed)
                m_optionSelected = (m_optionSelected + 1) % 4;
            if (in.aPressed) {
                switch (m_optionSelected) {
                    case 0:
                        // dump g23: level := 1, wipe bb/cake/k1..k5, -> ad (slot 8)
                        m_night = 1;
                        m_bb = false; m_cake = false;
                        m_k1 = false; m_k2 = false; m_k3 = false; m_k4 = false;
                        m_saveDirty = true;
                        m_screen = SCR_AD;
                        m_cardT = 0.0f;
                        break;
                    case 1: GoWhatDay(); break;                        // load: level
                    case 2: if (m_beat5) { m_night = 6; GoWhatDay(); } break;  // night 6
                    case 3:                                            // extras (slot 24)
                        m_ex.Clear();
                        m_screen = SCR_EXTRAS;
                        m_cardT = 0.0f;
                        Sfx("snd_Desolate_Underworld2", true, 1, 100);
                        break;
                }
            }
            break;
        }

        case SCR_AD:
            // dump frame 8: any key / 9 s -> what day
            m_cardT += dt;
            if (m_cardT >= 9.0f || in.aPressed || in.bPressed || in.upPressed || in.downPressed) {
                GoWhatDay();
            }
            break;

        case SCR_NIGHTSTART:
            TickWhatDay(dt, in);
            break;

        case SCR_WAIT:
            // frame 7: 100 ms black -> office (slot 0)
            m_waitT += dt;
            if (m_waitT >= 0.1f) {
                m_waitT = 0.0f;
                StartNight(m_night < 1 ? 1 : m_night);
                m_screen = SCR_OFFICE;
            }
            break;

        case SCR_OFFICE:
            TickOffice(dt, in);
            break;

        case SCR_STATIC6:
            // frame 4 = the DEATH static (the office win jumps V4 = next day
            // directly): stare loop + flash, 5 s -> gameover (slot 5)
            TickStaticDeath(dt);
            break;

        case SCR_GAMEOVER:
            TickGameOver(dt, in);
            break;

        case SCR_NEXTDAY:
            m_cardT += dt;
            if (m_cardT >= 3.5f) {
                WriteNightProgress();
            }
            break;

        case SCR_RARE1:
        case SCR_RARE2:
        case SCR_RARE3:
            TickRare(dt, in);
            break;

        case SCR_LOAD:
            TickLoad(dt);
            break;

        case SCR_CUTSCENE:
            TickCutscene(dt, in);
            break;

        case SCR_MG:
            TickMinigame(dt, in);
            break;

        case SCR_EXTRAS:
            TickExtras(dt, in);
            break;

        case SCR_ENDCHOOSER:
            // frame 17 (slot 14): the "end" sting; the cutscene counter
            // routes: != 5 -> what day (the next night), == 5 -> the ends
            // (goodend == 0 -> bad end, == 1 -> good end).
            if (m_cardT == 0.0f) {
                SfxStop("snd_tablefan"); SfxStop("snd_startday");
                SfxStop("snd_rainstorm2"); SfxStop("snd_scanner4");
                Sfx("snd_end", false, 1, 100);
            }
            m_cardT += dt;
            if (m_cardT >= 1.0f || in.aPressed) {
                if (m_cs.scene != 5) {
                    GoWhatDay();
                } else if (m_goodend) {
                    m_screen = SCR_ENDGOOD;
                } else {
                    m_screen = SCR_ENDBAD;
                }
                m_cardT = 0.0f;
            }
            break;

        case SCR_ENDBAD:
        case SCR_END2:
            // dump frames 9/11: mb2 loop + beatgame = 1, Escape/15 s -> title
            if (m_cardT == 0.0f) {
                Sfx("snd_mb2", true, 1, 100);
                m_beat5 = true;
                m_saveDirty = true;
            }
            m_cardT += dt;
            if (m_cardT >= 15.0f || in.bPressed) {
                SfxStop("snd_mb2");
                m_screen = SCR_TITLE;
                m_cardT = 0.0f;
            }
            break;

        case SCR_ENDGOOD:
            // dump frame 10: the "ending" song + beatgame = 1, Escape/59 s
            // -> title
            if (m_cardT == 0.0f) {
                Sfx("snd_ending", true, 1, 100);
                m_beat5 = true;
                m_saveDirty = true;
            }
            m_cardT += dt;
            if (m_cardT >= 59.0f || in.bPressed) {
                SfxStop("snd_ending");
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

    // ---- monitor flip (LB); frozen locks it (g594); the no-cams cheat
    // (the extras "nocams" key) keeps the monitor down ----
    if (in.lbPressed && m_frozen <= 0.0f && m_gotYou == 0 && !m_noCams) {
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
            // v2.63: the office secrets (dump groups ~8878-9115). The dump
            // double-clicks objects on the cams; the console maps them to X
            // on the right cam. The BB toy (CAM 08) and the puppet toy
            // (CAM 03) cams are dump-pinned; the arcade/cupcakes/keypad cam
            // picks are approximations (their objects aren't cam-labeled).
            if (m_night == 2 && selCam == 7 && !m_mapVent) {
                StartMinigame(MG_MANGLE, false);      // the arcade cabinet
            } else if (selCam == 8 && !m_mapVent) {
                StartMinigame(MG_BB, false);          // the BB toy
            } else if (selCam == 5 && !m_mapVent) {
                StartMinigame(MG_TOYCHICA, false);    // the cupcake run
            } else if (selCam == 1 && !m_mapVent) {
                StartMinigame(MG_GFREDDY, false);     // the 5-2-4-8 keypad
            } else if (selCam == 3 && !m_mapVent) {
                StartMinigame(MG_MARION, false);      // the puppet toy
            } else if (!ventMap) {
                PlayLure(selCam);                // audio lure (room map)
            } else if (!m_ventProof && m_sealTarget == 0 && m_youIn >= 11 && m_youIn <= 15) {
                // seal the selected vent (g: 50+Random(50) frames)
                m_sealTarget = m_youIn;
                m_sealDuration = (f32)(50 + rand() % 50) / 60.0f;
                m_sealProgress = 0.0f;
            }
        }
    } else if (m_night >= 5 && in.xPressed) {
        // the dark-room secret (dump: "dark" double-click, viewing <= 1,
        // night 5) -> RWQ (slot 22)
        StartMinigame(MG_RWQ, false);
    }

    // ---- sub-systems ----
    // the vent seal (g447-448): the progress counts to 50+Random(50) frames
    // while the seal is armed; done -> the vent closes (glitch2)
    if (m_sealTarget != 0) {
        m_sealProgress += dt;
        if (m_sealProgress >= m_sealDuration) {
            m_sealedVent = m_sealTarget;    // only one vent closed at a time
            m_sealTarget = 0;
            m_sealProgress = 0.0f;
            Sfx("snd_glitch2", false, 8, 100);
        }
    }
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
    // the "fast nights" cheat (extras): shorter hours — the factor is a
    // labeled approximation (the office's use of the flag isn't dumped)
    const f32 kHour = m_fastNights ? 20.0f : ((m_night < 2) ? 40.0f : 60.0f);
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
            // the office win jumps V4 = next day directly (frame 4 "static"
            // is the DEATH screen, not the 6 AM transition)
            m_screen = SCR_NEXTDAY;
            m_cardT = 0.0f;
            SfxStop("snd_tablefan");
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
    // (the "hyper" cheat adds +2 to the effective AI — labeled approximation)
    m_moveCounter += dt;
    const i32 aiEff = m_ai + (m_hyper ? 2 : 0);
    const i32 threshold = (10 - aiEff - (m_aggressive ? 1 : 0)) + (rand() % 15) - m_totalTurns;
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
                        else if (m_viewing >= 2) { dest = R3_GY2; m_bigScare = true; m_gotYouT = 0.0f; }
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
            case R3_V14: m_stRoom = R3_GY2; m_bigScare = false; m_gotYouT = 0.0f; break;
            case R3_V15: m_stRoom = R3_GY2; m_bigScare = false; m_gotYouT = 0.0f; break;
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
        if (m_night >= 2 && m_phBB == 0 && m_viewing <= 1 && (rand() % 10) + 1 <= m_ai) {
            m_phBB = 1; m_phBBStare = 0.0f;      // fresh stare on re-arm
        }
        if (m_night >= 2 && m_phMangle == 0 && m_youIn != 4 && (rand() % 7) + 1 <= m_ai) {
            m_phMangle = 2; m_phMangleStare = 0.0f;
        }
        if (m_night >= 4 && m_phPuppet == 0 && m_youIn != 8 && (rand() % 10) + 1 <= m_ai) {
            m_phPuppet = 2; m_phPuppetStare = 0.0f;
        }
        if (m_night >= 3 && m_phChica == 0 && m_youIn != 7 && (rand() % 10) + 1 <= m_ai) {
            m_phChica = 2; m_phChicaStare = 0.0f;
        }
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
    // Groups 510/512: he is at GOT YOU 2 (the window) — while the monitor is
    // up, after a 1 s beat, Random(2) fires the scare; the vent-error
    // blackout (alpha > 250) fires it at once even without the monitor
    if (m_gotYou == 0 && m_stRoom == R3_GY2) {
        m_gotYouT += dt;
        if ((m_viewing >= 2 && m_gotYouT >= 1.0f && (rand() % 2) == 0) ||
            (m_blackoutAlpha > 250.0f && m_viewing <= 1))
            m_gotYou = 2;
    }

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
            // the office death jumps V3 = static (frame 4: stare + flash,
            // 5 s) -> gameover (frame 6) -> title. The "in-place restart"
            // reading of Group 606 was the bogus file-index name of V3.
            m_died = true;
            m_gotYou = 0;
            m_scareT = 0.0f;
            m_screen = SCR_STATIC6;
            m_cardT = 0.0f;
            SfxStop("snd_tablefan");
            Sfx("snd_stare", true, 1, 100);
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

// ---- v2.62: the freddy3 save bridge (v2.63: the full dump key set) ----

void FNaF3Game::ApplyProgressF3(const Progress::GameProgressF3& p) {
    m_lastNight = p.level < 1 ? 1 : (p.level > 6 ? 6 : p.level);
    m_cine = p.cine;
    // beatgame is the nightmare unlock: the dump stores no separate flag —
    // level 6+ implies night 5 was beaten (the title's nightmare row no-ops
    // while !m_beat5; FNAF3's title rows are always drawn)
    m_beat5 = (p.level > 5);
    m_beat6 = p.beat6;
    m_goodend = p.goodend;
    m_fourthStar = p.fourthStar;
    m_bb = p.bb; m_cake = p.cake;
    m_k1 = p.k1; m_k2 = p.k2; m_k3 = p.k3; m_k4 = p.k4;
    m_fastNights = p.fastNights;
    m_ventProof = p.ventProof;
    m_hyper = p.hyper;
    m_noCams = p.noCams;
}

void FNaF3Game::FillProgressF3(Progress::GameProgressF3& p) const {
    p.level = m_night;      // the next-day screen already incremented it
    p.cine = m_cine;
    p.beat6 = m_beat6;
    p.goodend = m_goodend;
    p.fourthStar = m_fourthStar;
    p.bb = m_bb; p.cake = m_cake;
    p.k1 = m_k1; p.k2 = m_k2; p.k3 = m_k3; p.k4 = m_k4;
    p.fastNights = m_fastNights;
    p.ventProof = m_ventProof;
    p.hyper = m_hyper;
    p.noCams = m_noCams;
}

// ============================================================
// v2.63 — the dump's outer flow + cutscenes + minigames + extras.
// Jump values are STORYBOARD SLOTS (frameHandles[value]); the decoder's
// printed frame names were file-indexed and wrong — resolved through the
// chunk-8747 handle list: 0 office, 1 title, 2 what day, 3 static,
// 4 next day, 5 gameover, 6 wait, 8 ad, 9 good end, 10 the end 2,
// 11 bad end, 12 cutscenes, 13 load, 14 end chooser, 15 BB, 16 rare1,
// 17 Mangle, 18 rare3, 19 rare2, 20 Toy Chica, 21 GFreddy, 22 RWQ,
// 23 Marion, 24 extra, 26 demo end.
// ============================================================

// ---- the night card ("what day", frame 2, slot 2) ----

void FNaF3Game::GoWhatDay() {
    m_night = m_night < 1 ? 1 : (m_night > 6 ? 6 : m_night);
    m_lastNight = m_night;
    m_screen = SCR_NIGHTSTART;
    m_cardT = 0.0f;
    SfxStop("snd_tablefan");
    Sfx("snd_startday", false, 2, 100);
}

void FNaF3Game::TickWhatDay(f32 dt, const FNaF3Inputs& in) {
    // g10-12: the card holds 160 frames (~2.7 s) -> wait (100 ms) -> office;
    // 1/1000 -> rare3 (slot 18). "blip flash" pops over the card.
    (void)in;
    m_cardT += dt;
    if (m_cardT >= 2.7f) {
        m_cardT = 0.0f;
        if ((rand() % 1000) + 1 == 1) {
            m_rareId = 3;
            m_screen = SCR_RARE3;
        } else {
            m_screen = SCR_WAIT;
            m_waitT = 0.0f;
        }
    }
}

// ---- the death static (frame 4, slot 3): stare loop + flash, 5 s ----

void FNaF3Game::TickStaticDeath(f32 dt) {
    m_cardT += dt;
    if (m_cardT >= 5.0f) {
        SfxStop("snd_stare");
        m_screen = SCR_GAMEOVER;
        m_cardT = 0.0f;
    }
}

// ---- gameover (frame 6, slot 5): 5 s / click -> title; 1/1000 -> rare2 ----

void FNaF3Game::TickGameOver(f32 dt, const FNaF3Inputs& in) {
    if (m_cardT == 0.0f) {
        SfxStop("snd_stare");
        m_gameOverRare = (rand() % 1000) + 1;    // g2: rare random
        m_screen = SCR_GAMEOVER;                 // (cardT stays 0 below)
    }
    m_cardT += dt;
    if (m_cardT >= 5.0f || in.aPressed || in.bPressed) {
        m_cardT = 0.0f;
        if (m_gameOverRare == 1) {
            m_rareId = 2;
            m_screen = SCR_RARE2;
        } else {
            m_screen = SCR_TITLE;
            m_died = false;
        }
    }
}

// ---- the rare screens (frames 12/13/14): garble loop, 5 s out ----

void FNaF3Game::TickRare(f32 dt, const FNaF3Inputs& in) {
    if (m_cardT == 0.0f) {
        SfxStop("snd_tablefan"); SfxStop("snd_startday"); SfxStop("snd_stare");
        Sfx("snd_crazy garble", true, 1, 100);
    }
    m_cardT += dt;
    if (m_cardT >= 5.0f || in.bPressed) {
        SfxStop("snd_crazy garble");
        m_cardT = 0.0f;
        if (m_rareId == 3) GoWhatDay();
        else {
            m_screen = SCR_TITLE;
            m_died = false;
        }
    }
}

// ---- the between-night glitch (frame 18 "load", slot 13) ----

void FNaF3Game::TickLoad(f32 dt) {
    if (m_cardT == 0.0f) {
        // g2: cutscene += 1 — the scene number for the coming retro scene
        m_cine += 1;
        m_saveDirty = true;
        SfxStop("snd_tablefan"); SfxStop("snd_startday");
        Sfx("snd_long glitched2", true, 1, 100);
    }
    m_cardT += dt;
    if (m_cardT >= 5.0f) {
        SfxStop("snd_long glitched2");
        m_cardT = 0.0f;
        m_cs.Clear();
        m_cs.scene = m_cine < 1 ? 1 : (m_cine > 5 ? 5 : m_cine);
        m_cs.px = 500.0f; m_cs.py = 300.0f;   // g62: (500,272) rel backdrop
        m_screen = SCR_CUTSCENE;
        // g33: the scene ambience (the rain + the scanner loops)
        Sfx("snd_rainstorm2", true, 2, 100);
        Sfx("snd_scanner4", true, 3, 100);
    }
}

// ---- next day (frame 5, slot 4): the night advance + INI writes ----

void FNaF3Game::WriteNightProgress() {
    // g1: night number += 1; the INI keys (level = the new night)
    m_night += 1;
    m_lastNight = m_night > 6 ? 6 : m_night;
    m_saveDirty = true;
    if (m_night >= 7) {
        // g10/g11/g12: night 6 beaten -> the end 2 (slot 10) + beat6;
        // 4thstar only with every cheat off
        m_night = 6;
        m_beat6 = true;
        m_fourthStar = !(m_fastNights || m_ventProof || m_hyper || m_noCams);
        m_screen = SCR_END2;
        m_cardT = 0.0f;
        return;
    }
    if (m_night == 6) m_beat5 = true;           // g24: beatgame
    // g5-9: nights 2..6 -> load (slot 13) -> the retro cutscene. The dump
    // parks the night counter at 5 before the finale chain (g9).
    if (m_night == 6) m_night = 5;
    m_screen = SCR_LOAD;
    m_cardT = 0.0f;
}

// ============================================================
// the six Atari minigames (frames 19-24). Shared platformer skeleton:
// 100 ms ticks (fall 10 px, walk 15/20 px), the 60 ms rise tick (20 px,
// jump counters 7/9/10/2), sensors vs obstacle rects, balloon bounce
// floors, a 1024x768 viewport over the 3072x2304 world (scroll at the
// 384/512 mid-lines in 768/1024 steps). Dump deviations: pixel-perfect
// backdrop collision -> rect kit; item positions marked ~ are placed from
// the room kit where the layout rows were not captured.
// ============================================================

namespace {

struct Mg3Rect { f32 x, y, w, h; };

// the room kit both engines share: world borders (imgs 722/723)
static const Mg3Rect kMgBorder[4] = {
    {    0.0f,   -6.0f,  32.0f, 2316.0f },   // left wall
    { 3044.0f,   -2.0f,  32.0f, 2316.0f },   // right wall
    {   -2.0f,  -10.0f, 3054.0f,   30.0f },  // ceiling
    {    0.0f, 2272.0f, 3054.0f,   30.0f }   // ground
};

// per-game wall/platform kits (x, y, w, h tops). The floor pieces are the
// dump's 762x30 / 164x30 platform images at their instance positions.
static const Mg3Rect kMgWallsBB[] = {
    {  106.0f,  668.0f, 762.0f, 30.0f },
    {  106.0f,  132.0f, 762.0f, 30.0f },
    {   36.0f,   28.0f, 543.0f, 42.0f },
    { 1828.0f, 1160.0f, 762.0f, 30.0f },
    { 2180.0f, 1652.0f, 762.0f, 30.0f },
    { 2180.0f, 2198.0f, 762.0f, 30.0f },
    {  610.0f,  554.0f, 164.0f, 30.0f }, {  346.0f, 488.0f, 164.0f, 30.0f },
    {  134.0f,  400.0f, 164.0f, 30.0f }, {  434.0f, 320.0f, 164.0f, 30.0f },
    {  700.0f,  294.0f, 164.0f, 30.0f }, { 2334.0f, 2096.0f, 164.0f, 30.0f },
    { 2536.0f, 1992.0f, 164.0f, 30.0f }, { 2758.0f, 1904.0f, 164.0f, 30.0f },
    { 2466.0f, 1828.0f, 164.0f, 30.0f },
    {  839.0f,  134.0f,  32.0f, 562.0f },    // img 725 wall
    { 2910.0f, 1652.0f,  32.0f, 572.0f }     // img 733 wall
};
static const Mg3Rect kMgWallsMangle[] = {
    {  106.0f,  668.0f, 2736.0f, 30.0f },    // img 874 long band
    {  106.0f,  132.0f, 2736.0f, 30.0f },
    {  148.0f,  810.0f,  890.0f, 30.0f },    // img 904 block top
    { 1132.0f,  196.0f, 358.0f, 92.0f },  { 1382.0f, 196.0f, 358.0f, 92.0f },
    { 1698.0f,  432.0f, 358.0f, 92.0f },  { 2184.0f, 182.0f, 358.0f, 92.0f },
    {  610.0f,  554.0f, 164.0f, 30.0f },  { 1392.0f, 342.0f, 164.0f, 30.0f },
    { 1694.0f,  374.0f, 164.0f, 30.0f },  { 1334.0f, 552.0f, 164.0f, 30.0f },
    { 1106.0f,  422.0f, 164.0f, 30.0f },  { 2678.0f, 412.0f, 164.0f, 30.0f },
    { 2096.0f,  456.0f, 164.0f, 30.0f },  { 2346.0f, 550.0f, 164.0f, 30.0f },
    { 2386.0f,  398.0f, 164.0f, 30.0f },  { 2838.0f, 340.0f, 32.0f, 358.0f }
};
static const Mg3Rect kMgWallsToyChica[] = {
    {  106.0f,  670.0f, 762.0f, 30.0f },
    {  106.0f,  132.0f, 762.0f, 30.0f },
    { 1660.0f,  670.0f, 762.0f, 30.0f },
    { 2180.0f, 1652.0f, 762.0f, 30.0f },
    { 2180.0f, 2198.0f, 762.0f, 30.0f },
    {  346.0f, 1358.0f, 762.0f, 30.0f },  {  904.0f, 1358.0f, 762.0f, 30.0f },
    { 1669.0f, 1357.0f, 762.0f, 30.0f },
    { 1206.0f,  560.0f, 164.0f, 30.0f },  { 1476.0f, 412.0f, 164.0f, 30.0f },
    { 1632.0f,  412.0f, 164.0f, 30.0f },  { 1882.0f, 280.0f, 164.0f, 30.0f },
    { 1522.0f,  752.0f, 164.0f, 30.0f },  { 1328.0f, 852.0f, 164.0f, 30.0f },
    { 1074.0f,  948.0f, 164.0f, 30.0f },  { 1300.0f, 1062.0f, 164.0f, 30.0f },
    { 2334.0f, 2096.0f, 164.0f, 30.0f },  { 2536.0f, 1992.0f, 164.0f, 30.0f },
    { 2758.0f, 1904.0f, 164.0f, 30.0f },  { 2466.0f, 1828.0f, 164.0f, 30.0f }
};
// GFreddy and RWQ share the four-column stacked-room kit
static const Mg3Rect kMgWallsGF[] = {
    {  106.0f,  668.0f, 762.0f, 30.0f },  {  106.0f,  132.0f, 762.0f, 30.0f },
    { 1146.0f,  654.0f, 762.0f, 30.0f },  { 1146.0f,  118.0f, 762.0f, 30.0f },
    { 2194.0f,  650.0f, 762.0f, 30.0f },  { 2194.0f,  114.0f, 762.0f, 30.0f },
    {  108.0f,  884.0f, 762.0f, 30.0f },  {  108.0f, 1420.0f, 762.0f, 30.0f },
    { 1154.0f,  882.0f, 762.0f, 30.0f },  { 1154.0f, 1418.0f, 762.0f, 30.0f },
    {  134.0f,  580.0f, 164.0f, 30.0f },  {  114.0f, 1330.0f, 164.0f, 30.0f },
    { 1160.0f, 1328.0f, 164.0f, 30.0f },  { 1152.0f,  564.0f, 164.0f, 30.0f },
    {  178.0f,  210.0f, 226.0f, 128.0f }, {  566.0f,  352.0f, 226.0f, 128.0f },
    { 1226.0f,  960.0f, 226.0f, 128.0f }, { 1614.0f, 1102.0f, 226.0f, 128.0f },
    { 1218.0f,  196.0f, 226.0f, 128.0f }, { 1606.0f,  338.0f, 226.0f, 128.0f },
    {  839.0f,  134.0f,  32.0f, 562.0f }
};
static const Mg3Rect kMgWallsRWQ[] = {
    {  106.0f,  668.0f, 762.0f, 30.0f },  {  106.0f,  132.0f, 762.0f, 30.0f },
    { 2194.0f,  648.0f, 762.0f, 30.0f },  { 2194.0f,  112.0f, 762.0f, 30.0f },
    {  134.0f,  580.0f, 164.0f, 30.0f },
    {  714.0f, 1150.0f, 164.0f, 30.0f },  {  132.0f, 1194.0f, 164.0f, 30.0f },
    {  382.0f, 1288.0f, 164.0f, 30.0f },  {  422.0f, 1136.0f, 164.0f, 30.0f },
    { 2698.0f,  534.0f, 164.0f, 30.0f },  { 2434.0f,  468.0f, 164.0f, 30.0f },
    { 2222.0f,  380.0f, 164.0f, 30.0f },  { 2522.0f,  300.0f, 164.0f, 30.0f },
    { 2788.0f,  274.0f, 164.0f, 30.0f },
    {  873.0f,  890.0f, 762.0f, 30.0f },  {  102.0f,  896.0f, 762.0f, 30.0f },
    { 220.0f,  920.0f, 358.0f, 92.0f },
    {  839.0f,  134.0f,  32.0f, 562.0f }, { 2927.0f,  114.0f, 32.0f, 562.0f }
};
static const Mg3Rect kMgWallsMarion[] = {
    {  106.0f,  668.0f, 762.0f, 30.0f },  {  106.0f,  132.0f, 762.0f, 30.0f },
    { 2180.0f, 1652.0f, 762.0f, 30.0f },  { 2180.0f, 2198.0f, 762.0f, 30.0f },
    {  598.0f,  562.0f, 164.0f, 30.0f },  { 1215.0f,  562.0f, 164.0f, 30.0f },
    { 1722.0f,  561.0f, 164.0f, 30.0f },  { 2554.0f,  563.0f, 164.0f, 30.0f },
    { 2334.0f, 2096.0f, 164.0f, 30.0f },  { 2536.0f, 1992.0f, 164.0f, 30.0f },
    { 2758.0f, 1904.0f, 164.0f, 30.0f },  { 2466.0f, 1828.0f, 164.0f, 30.0f }
};

static const Mg3Rect* MgWallTable(i32 game, i32& count) {
    switch (game) {
        case FNaF3Game::MG_BB:        count = (i32)(sizeof(kMgWallsBB)/sizeof(kMgWallsBB[0]));        return kMgWallsBB;
        case FNaF3Game::MG_MANGLE:    count = (i32)(sizeof(kMgWallsMangle)/sizeof(kMgWallsMangle[0]));return kMgWallsMangle;
        case FNaF3Game::MG_TOYCHICA:  count = (i32)(sizeof(kMgWallsToyChica)/sizeof(kMgWallsToyChica[0])); return kMgWallsToyChica;
        case FNaF3Game::MG_GFREDDY:   count = (i32)(sizeof(kMgWallsGF)/sizeof(kMgWallsGF[0]));        return kMgWallsGF;
        case FNaF3Game::MG_RWQ:       count = (i32)(sizeof(kMgWallsRWQ)/sizeof(kMgWallsRWQ[0]));      return kMgWallsRWQ;
        case FNaF3Game::MG_MARION:    count = (i32)(sizeof(kMgWallsMarion)/sizeof(kMgWallsMarion[0]));return kMgWallsMarion;
    }
    count = 0;
    return 0;
}

// the balloon bounce strips (BB layout, img 854) — shared by every game
// (they appear once the BB minigame's balloon is taken)
static const Mg3Rect kMgBalloons[8] = {
    {    0.0f, 1073.0f, 130.0f, 114.0f }, {  275.0f, 1183.0f, 130.0f, 114.0f },
    {  591.0f, 1271.0f, 130.0f, 114.0f }, {  795.0f, 1213.0f, 130.0f, 114.0f },
    {  985.0f, 1165.0f, 130.0f, 114.0f }, { 1177.0f, 1111.0f, 130.0f, 114.0f },
    { 1399.0f, 1109.0f, 130.0f, 114.0f }, { 1633.0f, 1109.0f, 130.0f, 114.0f }
};

// the pickup/exit tables (world rects). BB and Marion are dump-exact; the
// other games place their items on the kit platforms (~ labeled).
struct Mg3Item { f32 x, y, w, h; i32 kind; };
// kind: 0 collect, 1 exit, 2 kid/crying, 3 cake pickup, 4 big cake
static const Mg3Item kMgItemsBB[] = {
    { 612.0f,  425.0f, 68.0f, 114.0f, 0 }, { 708.0f, 427.0f, 68.0f, 114.0f, 0 },
    { 354.0f,  359.0f, 68.0f, 114.0f, 0 }, { 154.0f, 275.0f, 68.0f, 114.0f, 0 },
    { 440.0f,  197.0f, 68.0f, 114.0f, 0 }, { 514.0f, 195.0f, 68.0f, 114.0f, 0 },
    { 752.0f,  173.0f, 68.0f, 114.0f, 0 },
    { 151.0f,  544.0f, 103.0f, 124.0f, 1 },   // exit
    { 2506.0f, 1697.0f, 68.0f, 114.0f, 3 },   // the balloon pickup ("get this")
    { 2247.0f,  992.0f, 159.0f, 168.0f, 4 }   // big cake (feeds the kid, k1)
};
static const Mg3Item kMgItemsMangle[] = {
    { 844.0f, 1248.0f, 65.0f, 57.0f, 0 },  { 1255.0f, 2093.0f, 65.0f, 57.0f, 0 },
    { 1069.0f, 1949.0f, 65.0f, 57.0f, 0 }, { 1353.0f, 1875.0f, 65.0f, 57.0f, 0 },
    { 1651.0f, 1805.0f, 65.0f, 57.0f, 0 }, { 1887.0f, 1669.0f, 65.0f, 57.0f, 0 },
    { 1673.0f, 1567.0f, 65.0f, 57.0f, 0 },
    { 2675.0f, 543.0f, 103.0f, 124.0f, 1 }, // exit
    { 2659.0f, 502.0f, 168.0f, 168.0f, 2 }, // the crying kid (layout-exact)
    { 2550.0f, 1800.0f, 65.0f, 57.0f, 3 }   // the cake pickup
};
static const Mg3Item kMgItemsToyChica[] = {
    { 1242.0f, 475.0f, 41.0f, 38.0f, 0 }, { 1931.0f, 202.0f, 41.0f, 38.0f, 0 },
    { 2293.0f, 1202.0f, 41.0f, 38.0f, 0 }, { 2618.0f, 1200.0f, 41.0f, 38.0f, 0 },
    { 844.0f, 33.0f, 65.0f, 57.0f, 0 },   { 461.0f, 505.0f, 65.0f, 57.0f, 0 },
    { 720.0f, 1203.0f, 65.0f, 57.0f, 0 }, { 547.0f, 1127.0f, 65.0f, 57.0f, 0 },
    { 1827.0f, 1232.0f, 103.0f, 124.0f, 1 },  // exit
    { 81.0f, 2149.0f, 103.0f, 124.0f, 1 },    // exit 2
    { 500.0f, 480.0f, 159.0f, 168.0f, 4 }     // big cake (feeds the kid, k2)
};
static const Mg3Item kMgItemsGF[] = {
    {  544.0f, -98.0f, 68.0f, 114.0f, 3 },  { -116.0f, 295.0f, 68.0f, 114.0f, 3 },
    { 2949.0f, 2150.0f, 103.0f, 124.0f, 1 },  // exit (dump: hit)
    { 2653.0f,  482.0f, 159.0f, 168.0f, 4 }    // big cake (the kid's donut, k3)
};
static const Mg3Item kMgItemsRWQ[] = {
    { 714.0f, -212.0f, 65.0f, 57.0f, 0 }, { 1370.0f, 525.0f, 65.0f, 57.0f, 0 },
    { 2700.0f, 405.0f, 65.0f, 57.0f, 0 }, { 2796.0f, 407.0f, 65.0f, 57.0f, 0 },
    { 725.0f,  170.0f, 103.0f, 124.0f, 1 },  // exit (dump: dropdown)
    { 1131.0f, 1356.0f, 159.0f, 168.0f, 4 }   // big cake (the RWQ kid, k4)
};
static const Mg3Item kMgItemsMarion[] = {
    {  468.0f,  425.0f, 68.0f, 114.0f, 0 }, {  908.0f, 427.0f, 68.0f, 114.0f, 0 },
    { 1258.0f,  394.0f, 159.0f, 168.0f, 0 },  // the three party cakes
    { 1761.0f,  394.0f, 159.0f, 168.0f, 0 },
    {  151.0f,  544.0f, 103.0f, 124.0f, 1 },  // exit
    { 2609.0f,  396.0f, 159.0f, 168.0f, 4 }   // big cake + trigger (goodend)
};
static const Mg3Item* MgItemTable(i32 game, i32& count) {
    switch (game) {
        case FNaF3Game::MG_BB:        count = (i32)(sizeof(kMgItemsBB)/sizeof(kMgItemsBB[0]));        return kMgItemsBB;
        case FNaF3Game::MG_MANGLE:    count = (i32)(sizeof(kMgItemsMangle)/sizeof(kMgItemsMangle[0]));return kMgItemsMangle;
        case FNaF3Game::MG_TOYCHICA:  count = (i32)(sizeof(kMgItemsToyChica)/sizeof(kMgItemsToyChica[0])); return kMgItemsToyChica;
        case FNaF3Game::MG_GFREDDY:   count = (i32)(sizeof(kMgItemsGF)/sizeof(kMgItemsGF[0]));        return kMgItemsGF;
        case FNaF3Game::MG_RWQ:       count = (i32)(sizeof(kMgItemsRWQ)/sizeof(kMgItemsRWQ[0]));      return kMgItemsRWQ;
        case FNaF3Game::MG_MARION:    count = (i32)(sizeof(kMgItemsMarion)/sizeof(kMgItemsMarion[0]));return kMgItemsMarion;
    }
    count = 0;
    return 0;
}

// per-game audio + jump table (dump samples)
struct Mg3Cfg { i32 game; const char* music; const char* jumpSfx; i32 jumpCnt; i32 balloonJump; i32 walkStep; bool hasJump; };
static const Mg3Cfg kMgCfg[6] = {
    { FNaF3Game::MG_BB,        "snd_mb4b", "snd_jump",  7,  7, 15, true  },
    { FNaF3Game::MG_MANGLE,    "snd_mb5",  "snd_jump2", 7, 10, 15, true  },
    { FNaF3Game::MG_TOYCHICA,  "snd_mb8",  "snd_jump3", 9,  9, 15, true  },
    { FNaF3Game::MG_GFREDDY,   "snd_mb9",  "snd_jump4", 7,  7, 15, true  },
    { FNaF3Game::MG_RWQ,       "snd_mb1",  0,           2,  7, 15, true  },
    { FNaF3Game::MG_MARION,    "snd_mb2",  0,           0,  0, 20, false }
};
static const Mg3Cfg& MgCfg(i32 game) {
    for (i32 i = 0; i < 6; ++i)
        if (kMgCfg[i].game == game) return kMgCfg[i];
    return kMgCfg[0];
}

// the RWQ S-teleport spots (watch-screen centers, dump g41-45)
static const f32 kRwqView[5][2] = {
    { 512.0f,  384.0f }, { 1536.0f, 384.0f }, { 2560.0f, 384.0f },
    { 512.0f, 1152.0f }, { 1536.0f, 1152.0f }
};

static bool RectHit(const Mg3Rect& r, f32 x, f32 y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

} // file-local tables

bool FNaF3Game::MgObstacle(f32 x, f32 y) const {
    i32 n = 0;
    const Mg3Rect* t = MgWallTable(m_mg.game, n);
    for (i32 i = 0; i < 4; ++i)
        if (RectHit(kMgBorder[i], x, y)) return true;
    for (i32 i = 0; i < n; ++i)
        if (RectHit(t[i], x, y)) return true;
    return false;
}

bool FNaF3Game::MgBalloonAt(f32 x, f32 y) const {
    if (!m_bb) return false;                       // g28/29: balloons after BB
    for (i32 i = 0; i < 8; ++i)
        if (RectHit(kMgBalloons[i], x, y)) return true;
    return false;
}

void FNaF3Game::StartMinigame(i32 game, bool fromExtras) {
    m_mg.Clear();
    m_mg.game = game;
    m_mg.fromExtras = fromExtras;
    m_mg.px = 309.0f; m_mg.py = 589.0f;            // the dump spawn (hit box)
    if (game == MG_MANGLE) { m_mg.px = 445.0f; m_mg.py = 595.0f; }
    m_mg.camX = m_mg.px - 512.0f; m_mg.camY = m_mg.py - 384.0f;
    if (m_mg.camX < 0.0f) m_mg.camX = 0.0f;
    if (m_mg.camY < 0.0f) m_mg.camY = 0.0f;
    m_screen = SCR_MG;
    m_cardT = 0.0f;
    // the office / cutscene loops stop on entry
    SfxStop("snd_tablefan"); SfxStop("snd_rainstorm2"); SfxStop("snd_scanner4");
    SfxStop("snd_crazy garble"); SfxStop("snd_long glitched2");
    SfxStop("snd_Desolate_Underworld2");
    Sfx(MgCfg(game).music, true, 1, 100);          // g2: the game's own loop
}

void FNaF3Game::ExitMinigameToExtras() {
    SfxStop(MgCfg(m_mg.game).music);
    m_ex.Clear();
    m_screen = SCR_EXTRAS;
    m_cardT = 0.0f;
    Sfx("snd_Desolate_Underworld2", true, 1, 100);
}

void FNaF3Game::MgCheckPickups() {
    i32 n = 0;
    const Mg3Item* t = MgItemTable(m_mg.game, n);
    const f32 px = m_mg.px, py = m_mg.py;
    for (i32 i = 0; i < n; ++i) {
        if (m_mg.taken[i]) continue;
        const Mg3Item& it = t[i];
        if (px < it.x - 23.0f || px > it.x + it.w + 23.0f ||
            py < it.y - 23.0f || py > it.y + it.h + 23.0f)
            continue;
        switch (it.kind) {
            case 0:   // a collect (Counter+1; the "collect"/"get" sfx)
                m_mg.taken[i] = 1;
                m_mg.collects += 1;
                Sfx(m_mg.game == MG_TOYCHICA ? "snd_get2" :
                    (m_mg.game == MG_MANGLE ? "snd_get" : "snd_collect"),
                    false, 2, 100);
                if (m_mg.game == MG_MANGLE) {
                    // the four kids join the walk (g39-42)
                    if (m_mg.collects <= 4) m_mg.kidFollow[m_mg.collects - 1] = true;
                }
                break;
            case 5:   // Toy Chica: feed a guest ("feed" + the exit counter)
                m_mg.taken[i] = 1;
                m_mg.fed += 1;
                Sfx("snd_feed", false, 2, 100);
                break;
            case 1:   // the exit -> win (g31/45; Mangle's shows after a scroll)
                if (m_mg.game == MG_TOYCHICA && m_mg.fed < 4) break;
                if (m_mg.game == MG_MANGLE && !m_mg.scrolled) break;
                m_mg.won = true;
                SfxStop(MgCfg(m_mg.game).music);
                break;
            case 3:   // the balloon pickup ("get this") -> bb = 1 + win
                m_mg.taken[i] = 1;
                m_bb = true;
                m_saveDirty = true;
                m_mg.won = true;
                SfxStop(MgCfg(m_mg.game).music);
                break;
            case 4:   // the big cake: feed the kid (needs the Mangle cake)
                if (!m_cake || m_mg.feeding) break;
                if (m_mg.game == MG_MARION) {
                    // Marion: needs all four kids (k1..k4) — dump g26
                    if (!(m_k1 && m_k2 && m_k3 && m_k4)) break;
                    m_goodend = true;              // the "goodend" INI write
                    m_saveDirty = true;
                    m_mg.party = 1;                // the float finale
                    m_mg.partyT = 0.0f;
                } else {
                    if (m_mg.game == MG_BB) { m_k1 = true; Sfx("snd_feed", false, 2, 100); }
                    if (m_mg.game == MG_TOYCHICA) { m_k2 = true; Sfx("snd_feed", false, 2, 100); }
                    if (m_mg.game == MG_GFREDDY) { m_k3 = true; Sfx("snd_feed", false, 2, 100); }
                    if (m_mg.game == MG_RWQ) { m_k4 = true; Sfx("snd_feed", false, 2, 100); }
                    m_saveDirty = true;
                    m_mg.feeding = true;           // the freeze cutscene
                    m_mg.cakeT = 0.0f;
                }
                m_mg.taken[i] = 1;
                break;
        }
    }
}

void FNaF3Game::MgStep(f32 dt, const FNaF3Inputs& in) {
    const Mg3Cfg& cfg = MgCfg(m_mg.game);
    const bool frozen = m_mg.feeding || m_mg.party > 0 || m_mg.won;

    // ---- the 100 ms walk tick (dump g6/g7): blocked by the walls ----
    m_mg.walkT += dt;
    if (!frozen && m_mg.walkT >= 0.1f) {
        m_mg.walkT -= 0.1f;
        if (m_mg.game == MG_RWQ) {
            // the glitch re-roll (dump g46: one chance in 2 per 100 ms)
            if ((rand() % 2) == 0) m_mg.view = 1 + rand() % 5;
        }
        f32 nx = m_mg.px;
        if (in.rightPressed && !in.leftPressed) {
            nx = m_mg.px + (f32)cfg.walkStep;
            m_mg.facing = 0;
        } else if (in.leftPressed && !in.rightPressed) {
            nx = m_mg.px - (f32)cfg.walkStep;
            m_mg.facing = 1;
        }
        if (nx != m_mg.px) {
            if (!MgObstacle(nx, m_mg.py)) m_mg.px = nx;
        }
    }

    // ---- jump start (g8/g9: W + grounded, or on the balloons) ----
    if (!frozen && cfg.hasJump && in.aPressed == false && in.upPressed &&
        m_mg.jumpCnt == 0) {
        const f32 feet = m_mg.py + 23.0f;
        if (MgObstacle(m_mg.px, feet + 4.0f)) {
            m_mg.jumpCnt = cfg.jumpCnt;
            if (cfg.jumpSfx) Sfx(cfg.jumpSfx, false, 2, 100);
        } else if (MgBalloonAt(m_mg.px, feet + 4.0f)) {
            m_mg.jumpCnt = cfg.balloonJump;        // g9: the balloon boost
            if (cfg.jumpSfx) Sfx(cfg.jumpSfx, false, 2, 100);
        }
    }

    // ---- the 60 ms rise tick (g10) ----
    if (m_mg.jumpCnt > 0) {
        m_mg.riseT += dt;
        while (m_mg.riseT >= 0.06f && m_mg.jumpCnt > 0) {
            m_mg.riseT -= 0.06f;
            m_mg.jumpCnt -= 1;
            if (!MgObstacle(m_mg.px, m_mg.py - 23.0f - 20.0f))
                m_mg.py -= 20.0f;
            else
                m_mg.jumpCnt = 0;                  // g11: the head bonk
        }
    } else {
        // ---- the 100 ms fall tick (g4/g5) ----
        m_mg.fallT += dt;
        while (m_mg.fallT >= 0.1f) {
            m_mg.fallT -= 0.1f;
            const f32 feet = m_mg.py + 23.0f;
            if (MgObstacle(m_mg.px, feet + 10.0f) ||
                MgBalloonAt(m_mg.px, feet + 10.0f))
                break;                             // landed
            m_mg.py += 10.0f;
        }
    }

    // ---- the viewport (g13-18: 1024x768 pages over the 3072x2304 world,
    // scroll when the hit box crosses the 384/512 mid-lines) ----
    if (m_mg.game == MG_RWQ) {
        // RWQ: the S-teleport views (down = cycle) + the glitch re-roll
        if (in.downPressed && !frozen) {
            m_mg.view = m_mg.view % 5 + 1;
        }
        m_mg.camX = kRwqView[m_mg.view - 1][0] - 512.0f;
        m_mg.camY = kRwqView[m_mg.view - 1][1] - 384.0f;
        m_mg.scrolled = true;
    } else {
        const f32 midX = m_mg.camX + 512.0f;
        const f32 midY = m_mg.camY + 384.0f;
        if (m_mg.px > midX + 512.0f) { m_mg.camX += 1024.0f; m_mg.scrolled = true; }
        if (m_mg.px < midX - 512.0f) { m_mg.camX -= 1024.0f; m_mg.scrolled = true; }
        if (m_mg.py > midY + 384.0f) { m_mg.camY += 768.0f; m_mg.scrolled = true; }
        if (m_mg.py < midY - 384.0f) { m_mg.camY -= 768.0f; m_mg.scrolled = true; }
        if (m_mg.camX < 0.0f) m_mg.camX = 0.0f;
        if (m_mg.camY < 0.0f) m_mg.camY = 0.0f;
        if (m_mg.camX > 2048.0f) m_mg.camX = 2048.0f;
        if (m_mg.camY > 1536.0f) m_mg.camY = 1536.0f;
    }

    // ---- pickups / exits / cakes ----
    if (!frozen) MgCheckPickups();

    // ---- the feed cutscene (the big cakes): 200 frames then win ----
    if (m_mg.feeding) {
        m_mg.cakeT += dt * 60.0f;
        if (m_mg.cakeT > 200.0f) {
            m_mg.feeding = false;
            m_mg.won = true;
            SfxStop(MgCfg(m_mg.game).music);
        }
    }

    // ---- the Marion float finale: >1300 frames -> win ----
    if (m_mg.party > 0) {
        m_mg.partyT += dt * 60.0f;
        if (m_mg.partyT > 1300.0f) {
            m_mg.party = 0;
            m_mg.won = true;
            SfxStop(MgCfg(m_mg.game).music);
        }
    }

    // ---- the win counter (g24-26: 200 frames) then the exit route ----
    if (m_mg.won) {
        m_mg.winT += dt * 60.0f;
        if (m_mg.winT >= 200.0f) {
            SfxStop(MgCfg(m_mg.game).music);
            if (m_mg.fromExtras) {
                ExitMinigameToExtras();
            } else {
                m_mg.Clear();
                GoWhatDay();                       // slot 2: back to the card
            }
        }
    }
}

void FNaF3Game::TickMinigame(f32 dt, const FNaF3Inputs& in) {
    if (in.bPressed) {
        // the dump's Escape quits the application; the console abandons the
        // game back to its entry point instead (labeled deviation)
        SfxStop(MgCfg(m_mg.game).music);
        if (m_mg.fromExtras) { ExitMinigameToExtras(); }
        else { m_mg.Clear(); GoWhatDay(); }
        return;
    }
    MgStep(dt, in);
}

// ============================================================
// the cutscenes frame (16): the retro pizzeria, 5x5 rooms, scene 1-4 =
// Shadow Freddy leads you to the back room where the Purple Guy takes the
// animatronic you play apart; scene 5 = the back room opens and he hides
// in the Springtrap suit (the finale).
// ============================================================

namespace {

// open directions per room (bit 1=up 2=down 4=left 8=right) — the barrier
// groups 10-25 of frame 16 (a shown barrier = a closed direction)
static const u8 kCsOpen[6][6] = {
    { 0,  0,  0,  0,  0,  0 },
    { 0,  0,  0,  0,  0,  2 },              // v1: only (1,5) exists
    { 0,  8, 14, 14, 14,  6 },              // v2: (2,5) up only at scene 5
    { 0,  0, 11, 13,  7,  1 },              // v3
    { 0,  0,  3,  0,  3,  0 },              // v4
    { 0,  0,  9, 12,  5,  0 }               // v5
};

// where the Shadow Freddy figure waits (cells, scene < 5) and which way it
// drifts (dump g96-105): 0 right, 1 up, 2 down
struct CsShadow { i32 v, h, dir; };
static const CsShadow kCsShadows[10] = {
    { 2, 4, 0 }, { 3, 3, 0 }, { 3, 2, 0 }, { 2, 5, 1 }, { 3, 4, 1 },
    { 4, 4, 1 }, { 5, 4, 1 }, { 5, 3, 0 }, { 5, 2, 0 }, { 4, 2, 2 }
};

} // file-local

void FNaF3Game::TickCutscene(f32 dt, const FNaF3Inputs& in) {
    CutsceneState& cs = m_cs;
    cs.t += dt;

    if (in.bPressed) {
        // the dump's Escape quits; the console returns to the title
        SfxStop("snd_rainstorm2"); SfxStop("snd_scanner4"); SfxStop("snd_crazy garble");
        m_screen = SCR_TITLE;
        return;
    }

    // ---- the 250 ms step gate (g5-8): 30 px, blocked by the room walls ----
    cs.moveT += dt;
    const bool killing = cs.manStage == 2 || cs.finaleOn;
    if (!killing && cs.moveT >= 0.25f) {
        cs.moveT -= 0.25f;
        i32 nv = cs.v, nh = cs.h;
        f32 nx = cs.px, ny = cs.py;
        bool move = false;
        if (in.upPressed && !in.downPressed)    { ny -= 30.0f; move = true; }
        if (in.downPressed && !in.upPressed)    { ny += 30.0f; move = true; }
        if (in.leftPressed && !in.rightPressed) { nx -= 30.0f; move = true; }
        if (in.rightPressed && !in.leftPressed) { nx += 30.0f; move = true; }
        if (move) {
            if (nx < 60.0f)  nx = 60.0f;       // the wall bands (imgs 722/723)
            if (nx > 964.0f) nx = 964.0f;
            if (ny < 90.0f)  ny = 90.0f;
            if (ny > 700.0f) ny = 700.0f;
            // the leave zones (g26-29): crossing the open edge wraps the room
            const u8 open = kCsOpen[cs.v][cs.h];
            if (nx >= 964.0f && cs.px < 964.0f) {
                if (open & 8) { nh += 1; nx = 60.0f; }
                else nx = cs.px;
            } else if (nx <= 60.0f && cs.px > 60.0f) {
                if (open & 4) { nh -= 1; nx = 964.0f; }
                else nx = cs.px;
            } else if (ny >= 700.0f && cs.py < 700.0f) {
                if (open & 2) { nv += 1; ny = 90.0f; }
                else ny = cs.py;
            } else if (ny <= 90.0f && cs.py > 90.0f) {
                const bool upOpen = (cs.v == 2 && cs.h == 5) ? (cs.scene >= 5)
                                                             : ((open & 1) != 0);
                if (upOpen) { nv -= 1; ny = 700.0f; }
                else {
                    ny = cs.py;
                    if (cs.v == 2 && cs.h == 5 && cs.scene < 5 && !cs.errShown) {
                        cs.errShown = true;    // g76: the ERR + the trigger arms
                        cs.errT = 3.3f;
                        cs.manStage = 1;       // the Purple Guy is coming
                        cs.manX = 100.0f; cs.manY = 600.0f;
                        Sfx("snd_crazy garble", true, 4, 100);
                    }
                }
            }
            cs.px = nx; cs.py = ny;
            if (nv != cs.v || nh != cs.h) {
                cs.v = nv; cs.h = nh;
                for (i32 i = 0; i < 3; ++i) cs.shadowOn[i] = false;
                // the shadow figure of this room (scene < 5)
                if (cs.scene < 5) {
                    for (i32 i = 0; i < 10; ++i) {
                        if (kCsShadows[i].v == cs.v && kCsShadows[i].h == cs.h) {
                            cs.shadowOn[kCsShadows[i].dir] = true;
                            cs.shadowX[kCsShadows[i].dir] = 0.0f;
                        }
                    }
                }
                if (cs.v == 1 && cs.h == 5) {
                    // the finale room (g157): the Purple Guy patrol + the suit
                    cs.finaleOn = true;
                    cs.patrolX = 100.0f; cs.patrolState = 0; cs.trips = 0;
                    SfxStop("snd_crazy garble");
                }
            }
        }
    }

    // ---- the shadows drift out (g89-95: 30 px per 290 ms) ----
    for (i32 i = 0; i < 3; ++i) {
        if (!cs.shadowOn[i]) continue;
        cs.shadowX[i] += 30.0f * (dt / 0.29f);
        if (cs.shadowX[i] < -100.0f) cs.shadowOn[i] = false;
    }

    // ---- the ERR display ----
    if (cs.errT > 0.0f) cs.errT -= dt;

    // ---- the Purple Guy homing (g111-114: 30 px per 100 ms) ----
    if (cs.manStage == 1) {
        cs.manT += dt;
        while (cs.manT >= 0.1f) {
            cs.manT -= 0.1f;
            if (cs.manY < cs.py - 20.0f) cs.manY += 30.0f;
            else if (cs.manY > cs.py + 20.0f) cs.manY -= 30.0f;
            else if (cs.manX < cs.px - 20.0f) cs.manX += 30.0f;
            else if (cs.manX > cs.px + 20.0f) cs.manX -= 30.0f;
        }
        if (cs.manX > cs.px - 50.0f && cs.manX < cs.px + 50.0f &&
            cs.manY > cs.py - 60.0f && cs.manY < cs.py + 60.0f) {
            // g115-118: the take-apart (the parts sprite per scene)
            cs.manStage = 2;
            cs.killT = 0.0f;
            SfxStop("snd_crazy garble");
            Sfx("snd_scare", false, 6, 100);
        }
    } else if (cs.manStage == 2) {
        cs.killT += dt * 60.0f;
        if (cs.killT >= 100.0f) {
            // g120: Next frame -> file 17, the end chooser
            SfxStop("snd_rainstorm2"); SfxStop("snd_scanner4");
            m_screen = SCR_ENDCHOOSER;
            m_cardT = 0.0f;
        }
    }

    // ---- the scene-5 finale (room 1,5): the patrol, the suit, the crush ----
    if (cs.finaleOn && cs.suitStage == 0) {
        // the Purple Guy patrols; each pass plays "run", the scare bumps
        // the trip counter; 4 trips -> he bolts for the suit (g161-165)
        cs.patrolT += dt;
        while (cs.patrolT >= 0.1f) {
            cs.patrolT -= 0.1f;
            cs.patrolX += 50.0f;
            if (cs.patrolX > 900.0f) {
                cs.patrolX = 100.0f;
                cs.patrolState += 1;
                cs.trips += 1;
                Sfx("snd_run", false, 5, 100);
                if (cs.trips % 2 == 1) Sfx("snd_scare", false, 6, 100);
                if (cs.trips >= 4) { cs.suitStage = 1; cs.suitT = 0.0f; }
            }
        }
    } else if (cs.finaleOn && cs.suitStage >= 1) {
        // the man run + the suit stages (g166-178): 100/100/200/300/200/300
        cs.suitT += dt * 60.0f;
        if (cs.suitStage == 1 && cs.suitT >= 100.0f) {
            cs.suitStage = 2; cs.suitT = 0.0f;
            Sfx("snd_insuit", false, 5, 100);
        } else if (cs.suitStage == 2 && cs.suitT >= 100.0f) {
            cs.suitStage = 3; cs.suitT = 0.0f;
        } else if (cs.suitStage == 3 && cs.suitT >= 200.0f) {
            cs.suitStage = 4; cs.suitT = 0.0f;
            Sfx("snd_laugh", false, 5, 100);
        } else if (cs.suitStage == 4 && cs.suitT >= 300.0f) {
            cs.suitStage = 5; cs.suitT = 0.0f;
            Sfx("snd_crush", false, 5, 100);
        } else if (cs.suitStage == 5 && cs.suitT >= 200.0f) {
            cs.suitStage = 6; cs.suitT = 0.0f;
            Sfx("snd_insuit", false, 5, 100);
        } else if (cs.suitStage == 6 && cs.suitT >= 300.0f) {
            // g178: Next frame -> the chooser (scene 5 -> the endings)
            SfxStop("snd_rainstorm2"); SfxStop("snd_scanner4");
            m_screen = SCR_ENDCHOOSER;
            m_cardT = 0.0f;
        }
    }

    // ---- ambience: the rain spawner (g70-72), the rat (g81-85) ----
    cs.rainT += dt;
    if (cs.rainT >= 1.0f) {
        cs.rainT -= 1.0f;
        if (cs.rainN < 12 && (rand() % 3) == 0) {
            cs.dropX[cs.rainN] = (f32)(60 + rand() % 900);
            cs.dropY[cs.rainN] = 0.0f;
            cs.rainN += 1;
        }
    }
    for (i32 i = cs.rainN - 1; i >= 0; --i) {
        cs.dropY[i] += 30.0f * (dt / 0.2f);
        if (cs.dropY[i] > 768.0f) {
            cs.dropY[i] = cs.dropY[cs.rainN - 1];
            cs.dropX[i] = cs.dropX[cs.rainN - 1];
            cs.rainN -= 1;
        }
    }
    cs.ratT += dt;
    if (cs.ratT >= 8.0f) { cs.ratT = 0.0f; cs.ratX = -60.0f; }
    if (cs.ratX < 1100.0f) cs.ratX += 50.0f * dt;
    if (cs.hintT > 0.0f) cs.hintT -= dt;
}

// ============================================================
// the extras menu (frame 25)
// ============================================================

void FNaF3Game::TickExtras(f32 dt, const FNaF3Inputs& in) {
    ExtrasState& ex = m_ex;
    if (ex.cooldown > 0.0f) ex.cooldown -= dt;

    if (in.upPressed)   { ex.row = (ex.row + 4) % 5; Sfx("snd_select", false, 2, 80); }
    if (in.downPressed) { ex.row = (ex.row + 1) % 5; Sfx("snd_select", false, 2, 80); }
    if (in.leftPressed || in.rightPressed) {
        const i32 d = in.rightPressed ? 1 : -1;
        if (ex.row == 0) ex.viewer = (ex.viewer + 7 + d) % 7;
        if (ex.row == 1 && m_goodend) ex.mgPick = (ex.mgPick + 5 + d) % 5;
        if (ex.row == 2 && m_beat6)   ex.jsPick = (ex.jsPick + 6 + d) % 6;
        if (ex.row == 3 && m_goodend && m_beat6) ex.viewer = 0;   // (unused pick)
        Sfx("snd_select", false, 2, 80);
    }

    if (in.bPressed) {                              // Escape -> title
        SfxStop("snd_Desolate_Underworld2");
        SfxStop("snd_scream3");
        m_screen = SCR_TITLE;
        return;
    }

    if (in.aPressed && ex.cooldown <= 0.0f) {
        switch (ex.row) {
            case 4:                                  // exit -> title
                SfxStop("snd_Desolate_Underworld2");
                m_screen = SCR_TITLE;
                break;
            case 1:                                  // replay a minigame
                if (m_goodend) {
                    static const i32 kReplay[5] = {
                        MG_BB, MG_MANGLE, MG_TOYCHICA, MG_GFREDDY, MG_RWQ
                    };
                    StartMinigame(kReplay[ex.mgPick], true);   // extras game? = 1
                }
                break;
            case 2:                                  // a jumpscare viewer
                if (m_beat6) {
                    ex.jsT = 1.5f;
                    Sfx("snd_scream3", false, 3, 100);
                }
                break;
            case 3:                                  // the cheats (g93-101)
                if (m_goodend && m_beat6) {
                    // A toggles the highlighted cheat (the dump uses four
                    // buttons; the console cycles them with left/right and
                    // toggles with A — viewer doubles as the cheat pick)
                    const i32 pick = ex.viewer % 4;
                    if (pick == 0) m_fastNights = !m_fastNights;
                    if (pick == 1) m_ventProof = !m_ventProof;
                    if (pick == 2) m_hyper = !m_hyper;
                    if (pick == 3) m_noCams = !m_noCams;
                    m_saveDirty = true;
                    ex.cooldown = 0.25f;
                    Sfx("snd_select", false, 3, 80);
                }
                break;
            default:
                break;
        }
    }

    if (ex.jsT > 0.0f) ex.jsT -= dt;
}


} // namespace fnaf
