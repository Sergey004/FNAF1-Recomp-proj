/**
 * FNaF4Game.cpp: v2.61 — FNAF4 night loop (see FNaF4Game.h).
 * Dump authority: Dumps/Five Nights at Freddys 4/Events/frame_3_level_events.txt
 * (480 groups). Timers are RAW MILLISECONDS (60 s hours, 5 s lane rolls,
 * 3 s shut-door visits, 10 s retreats, 4 s bed ticks). Labeled deviations:
 * the walk-through-the-hall free movement is collapsed to five positions
 * (console pad, no cursor); night 5's Bonnie/Chica/Freddy/Foxy AI carry
 * night 4's table (the dump only pins Fredbear 12).
 */

#include "FNaF4Game.h"
#include <stdlib.h>

namespace fnaf {

FNaF4Game::FNaF4Game() {
    ResetToTitle();
}

void FNaF4Game::ResetToTitle() {
    m_screen = SCR_DISCLAIMER;
    m_night = 1;
    m_hour = 0;
    m_hourClock = 0.0f;
    m_cardT = 0.0f;
    m_time = 0.0f;
    m_optionSelected = 0;
    m_lastNight = 1;
    m_beat5 = false;
    m_pos = P_CENTER;
    m_walkT = 0.0f;
    m_peeking = false;
    m_leftShut = m_rightShut = false;
    m_listening = 0;
    m_gameover = false;
    m_attackImg = 0;
    m_attackT = 0.0f;
    m_biteT = 0.0f;
}

void FNaF4Game::StartNight(i32 night) {
    m_night = night;
    InitNightState();
    m_screen = SCR_NIGHTSTART;
    m_cardT = 0.0f;
}

void FNaF4Game::InitNightState() {
    m_hour = 0;
    m_hourClock = 0.0f;
    m_pos = P_CENTER;
    m_walkT = 0.0f;
    m_walkDir = 0;
    m_peeking = false;
    m_leftShut = m_rightShut = false;
    m_prevLeftShut = m_prevRightShut = false;
    m_listening = 0;

    m_bonniePos = m_chicaPos = 0;
    m_laneRollT = 0.0f;
    m_bonnieIgnoreT = m_chicaIgnoreT = 0.0f;
    m_bonnieNearT = m_chicaNearT = 0.0f;
    m_bonnieBed = m_chicaBed = false;
    m_bonnieVisitT = m_chicaVisitT = 0.0f;

    m_foxyIn = false;
    m_closetCounter = 0;
    m_foxyGotYou = 0;
    m_foxyRollT = 0.0f;
    m_closetTickT = 0.0f;
    m_fredbearSndT = 0.0f;

    m_freddyCounter = 0;
    m_miniArt = rand() % 4;
    m_freddyT = 0.0f;
    m_bedTickT = 0.0f;
    m_biteT = 0.0f;

    m_totalDanger = 0;
    m_dangerT = 0.0f;
    m_bfTimer = 5.0f;
    m_blackFlashA = 0.0f;
    m_bfHold = 0.0f;

    m_attackImg = 0;
    m_attackT = 0.0f;
    m_gameover = false;

    ApplyNightAI();
}

// ------------------------------------------------------------
// helpers
// ------------------------------------------------------------

void FNaF4Game::Sfx(const char* s, bool loop, i32 ch, i32 vol) {
    if (audio.play) audio.play(s, loop, ch, vol);
}
void FNaF4Game::ChVol(i32 ch, i32 vol) {
    if (audio.channelVolume) audio.channelVolume(ch, vol);
}

// the per-night/hour AI tables (dump groups 387-409 + 111/444/405)
void FNaF4Game::ApplyNightAI() {
    switch (m_night) {
        case 1:
            m_bonnieAI = m_chicaAI = m_freddyAI = 0; m_foxyAI = 0;
            if (m_hour >= 2) { m_bonnieAI = 1; m_chicaAI = 1; m_freddyAI = 1; }
            if (m_hour >= 3) { m_bonnieAI = 3; m_chicaAI = 2; m_freddyAI = 2; }
            break;
        case 2:
            m_bonnieAI = 5; m_chicaAI = 5; m_freddyAI = 2; m_foxyAI = 1;
            if (m_hour >= 3) { m_bonnieAI = 7; m_chicaAI = 7; m_freddyAI = 3; m_foxyAI = 4; }
            break;
        case 3:
            m_bonnieAI = 7; m_chicaAI = 7; m_freddyAI = 3; m_foxyAI = 10;
            if (m_hour >= 3) { m_bonnieAI = 10; m_chicaAI = 10; }
            break;
        case 4:
            m_bonnieAI = 10; m_chicaAI = 10; m_freddyAI = 4; m_foxyAI = 5;
            if (m_hour >= 3) { m_bonnieAI = 12; m_chicaAI = 12; m_foxyAI = 10; }
            break;
        case 5:
            // DEVIATION: the dump pins only Fredbear 12 for night 5; the
            // rest carries night 4's table
            m_bonnieAI = 10; m_chicaAI = 10; m_freddyAI = 4; m_foxyAI = 5;
            m_fredbearAI = 12;
            if (m_hour >= 3) { m_bonnieAI = 12; m_chicaAI = 12; m_foxyAI = 10; }
            break;
        default:   // night 6+
            m_bonnieAI = 12; m_chicaAI = 12; m_freddyAI = 5; m_foxyAI = 10;
            m_fredbearAI = 0;
            if (m_hour >= 4) {   // g405: Fredbear takes the whole house
                m_bonnieAI = 0; m_chicaAI = 0; m_freddyAI = 0; m_foxyAI = 0;
                m_fredbearAI = 15;
            }
            break;
    }
    if (m_night < 5) m_fredbearAI = 0;
    if (m_fredbearAI > 0) m_freddyCounter = 0;   // g433
}

void FNaF4Game::TriggerJumpscare(i32 overlayImg) {
    if (m_gameover) return;
    m_attackImg = overlayImg;
    m_attackT = 0.001f;
    m_gameover = true;
    Sfx("snd_scream2", false, 16, 100);
}

// ------------------------------------------------------------
// tick
// ------------------------------------------------------------

void FNaF4Game::Tick(f32 dt, const FNaF4Inputs& in) {
    m_time += dt;

    switch (m_screen) {
        case SCR_DISCLAIMER:
            m_cardT += dt;
            if (m_cardT >= 3.5f || in.aHeld || in.bPressed) {
                m_screen = SCR_TITLE; m_cardT = 0.0f;
            }
            break;

        case SCR_TITLE:
            // 4 dump rows (new game / continue / 6th night / extra)
            if (in.upPressed)    m_optionSelected = (m_optionSelected + 3) % 4;
            if (in.downPressed)  m_optionSelected = (m_optionSelected + 1) % 4;
            if (in.aPressed) {
                switch (m_optionSelected) {
                    case 0: StartNight(1); break;                       // new game
                    case 1: StartNight(m_lastNight < 1 ? 1 : m_lastNight); break; // continue
                    case 2: if (m_beat5) StartNight(6); break;          // 6th night
                    case 3: /* extras — later wave */ break;
                }
            }
            break;

        case SCR_NIGHTSTART:
            m_cardT += dt;
            if (m_cardT >= 2.5f) { m_screen = SCR_BEDROOM; m_cardT = 0.0f; }
            break;

        case SCR_BEDROOM:
            TickBedroom(dt, in);
            break;

        case SCR_NIGHTWIN:
            m_cardT += dt;
            if (m_cardT >= 5.0f) {
                m_night += 1;
                m_lastNight = m_night > 6 ? 6 : m_night;
                if (m_night - 1 == 5) m_beat5 = true;   // the 6th night row
                m_screen = SCR_TITLE;
                m_cardT = 0.0f;
            }
            break;
    }

    // the jumpscare anim runs over any screen; it ends on the title
    if (m_attackT > 0.0f) {
        m_attackT += dt;
        if (m_attackT >= 1.5f) {                        // g204-207: -> title
            m_attackT = 0.0f;
            m_attackImg = 0;
            m_gameover = false;
            m_screen = SCR_TITLE;
            m_cardT = 0.0f;
        }
    }
    // the cosmetic bite overlay (g255) just fades
    if (m_biteT > 0.0f) m_biteT -= dt;
}

// ------------------------------------------------------------
// bedroom
// ------------------------------------------------------------

void FNaF4Game::TickBedroom(f32 dt, const FNaF4Inputs& in) {
    if (m_gameover) return;

    // ---- walking (collapsed to the 5-position graph) ----
    if (m_walkT > 0.0f) {
        m_walkT -= dt;
        if (m_walkT <= 0.0f) { m_walkT = 0.0f; m_pos = m_walkTo; }
    } else {
        bool walk = false;
        Pos to = m_pos;
        i32 dir = 0;
        if (m_foxyGotYou) {
            // the forced flashlight: walking away or sleeping is fatal
            // (g44-46/257)
            if (in.leftPressed || in.rightPressed || in.upPressed || in.bPressed)
                TriggerJumpscare(642);
        } else if (in.leftPressed) {
            if (m_pos == P_CENTER)      { to = P_LEFT;   dir = 45;  walk = true; }
            else if (m_pos == P_CLOSET) { to = P_CENTER; dir = 45;  walk = true; }
            else if (m_pos == P_RIGHT)  { to = P_CLOSET; dir = 45;  walk = true; }
        } else if (in.rightPressed) {
            if (m_pos == P_CENTER)      { to = P_CLOSET; dir = 160; walk = true; }
            else if (m_pos == P_CLOSET) { to = P_RIGHT;  dir = 160; walk = true; }
            else if (m_pos == P_LEFT)   { to = P_CENTER; dir = 160; walk = true; }
        } else if (in.upPressed && m_pos == P_CENTER) {
            to = P_BED; dir = 57; walk = true;          // to the bed
        } else if ((in.downPressed || in.bPressed) && m_pos == P_BED) {
            to = P_CENTER; dir = 57; walk = true;       // out of the bed
        }
        if (walk) {
            m_walkTo = to;
            m_walkDir = dir;
            m_walkT = 0.45f;
            m_peeking = false;
            Sfx("snd_carpetrun1b", false, 5, 100);
        }
    }

    // ---- doors (X hold at the doors; creaks per g79-86/193-196) ----
    m_prevLeftShut = m_leftShut;  m_prevRightShut = m_rightShut;
    m_leftShut  = (m_pos == P_LEFT  && in.xHeld && m_walkT <= 0.0f);
    m_rightShut = (m_pos == P_RIGHT && in.xHeld && m_walkT <= 0.0f);
    if (m_leftShut && !m_prevLeftShut)
        Sfx((m_miniArt % 2) ? "snd_doorcreak1" : "snd_doorcreak3", false, 7, 100);
    if (!m_leftShut && m_prevLeftShut)
        Sfx((m_miniArt % 2) ? "snd_door2a" : "snd_door2b", false, 7, 100);
    if (m_rightShut && !m_prevRightShut)
        Sfx((m_miniArt % 2) ? "snd_doorcreak2" : "snd_doorcreak4", false, 7, 100);
    if (!m_rightShut && m_prevRightShut)
        Sfx("snd_door2c", false, 7, 100);

    // ---- the flashlight peek (A hold; the click per g129-132) ----
    const bool wasPeeking = m_peeking;
    m_peeking = (in.aHeld && m_walkT <= 0.0f &&
                 (m_pos == P_LEFT || m_pos == P_RIGHT || m_pos == P_CLOSET || m_pos == P_BED));
    if (m_peeking && !wasPeeking)
        Sfx("snd_FLASHLIG_7_GEN-HDF11244", false, 10, 100);

    // ---- listening (standing at a door holding nothing; g185-192) ----
    m_listening = 0;
    if (m_walkT <= 0.0f && !m_peeking && !in.xHeld) {
        if (m_pos == P_LEFT)  m_listening = 1;
        if (m_pos == P_RIGHT) m_listening = 2;
    }
    ChVol(20, (m_listening == 1 && m_bonniePos == 1) ||
              (m_listening == 2 && m_chicaPos  == 1) ? 100 : 0);
    // Chica's kitchen (g306-308): audible through the right-door listen
    ChVol(19, m_listening == 2 ? (m_chicaPos == 0 ? 100 : 40) : 0);

    // ---- the lanes ----
    TickLanes(dt);
    TickFoxy(dt, m_peeking && m_pos == P_CLOSET);
    TickFreddy(dt, m_peeking && m_pos == P_BED);
    TickParanoia(dt);

    // Fredbear's closet breathing while watched (g97/98)
    if (m_fredbearAI > 0 && m_foxyIn)
        ChVol(27, (m_peeking && m_pos == P_CLOSET) ? 100 : 0);
    else
        ChVol(27, 0);

    // ---- the door-flash attacks (g201/202; DEVIATION: a shut door
    // blocks the flash-scare — the peek shows the closed door) ----
    if (m_peeking && m_pos == P_LEFT  && m_bonniePos == 1 && !m_leftShut)
        TriggerJumpscare(487);
    if (m_peeking && m_pos == P_RIGHT && m_chicaPos  == 1 && !m_rightShut)
        TriggerJumpscare(450);

    // ---- the bedroom attacks (g216/217: lie down while they linger) ----
    if (m_pos == P_BED && m_walkT <= 0.0f) {
        if (m_bonnieBed) { m_bonnieBed = false; TriggerJumpscare(592); }
        else if (m_chicaBed) { m_chicaBed = false; TriggerJumpscare(358); }
    }

    TickClock(dt);
}

// ------------------------------------------------------------
// clock (g380-385): raw ms -> 60 s per hour
// ------------------------------------------------------------

void FNaF4Game::TickClock(f32 dt) {
    m_hourClock += dt;
    if (m_hourClock >= 60.0f) {
        m_hourClock -= 60.0f;
        m_hour += 1;
        Sfx("snd_clockchime7", false, 3, 60);
        ApplyNightAI();
        if (m_hour >= 6) {                      // g385 -> the 5->6 AM screen
            m_screen = SCR_NIGHTWIN;
            m_cardT = 0.0f;
            Sfx("snd_alarmclock4", false, 2, 100);
        }
    }
}

// ------------------------------------------------------------
// Bonnie/Chica lanes (g150-167, 193-217, 298-308, 396/397)
// ------------------------------------------------------------

void FNaF4Game::TickLanes(f32 dt) {
    // the shared 5 s opportunity roll (g150/151)
    m_laneRollT += dt;
    if (m_laneRollT >= 5.0f) {
        m_laneRollT = 0.0f;
        if (m_listening != 1 && !m_leftShut && (rand() % 20) + 1 <= m_bonnieAI && m_bonniePos == 0) {
            m_bonniePos = 1;
            Sfx((m_miniArt % 2) ? "snd_walk1b" : "snd_walk2b", false, 5, 100);
        }
        if (m_listening != 2 && !m_rightShut && (rand() % 20) + 1 <= m_chicaAI && m_chicaPos == 0) {
            m_chicaPos = 1;
            Sfx((m_miniArt % 2) ? "snd_walk3b" : "snd_walk4b", false, 5, 100);
        }
    }

    // ignored at the door: he retreats after 10 s (g161/167)
    if (m_bonniePos == 1) {
        m_bonnieIgnoreT += dt;
        if (m_bonnieIgnoreT >= 10.0f && m_listening != 1 && !m_leftShut) {
            m_bonniePos = 0;
            m_bonnieIgnoreT = 0.0f;
            m_bonnieNearT = 0.0f;
            Sfx("snd_walk2b", false, 5, 100);
        }
    } else m_bonnieIgnoreT = 0.0f;
    if (m_chicaPos == 1) {
        m_chicaIgnoreT += dt;
        if (m_chicaIgnoreT >= 10.0f && m_listening != 2 && !m_rightShut) {
            m_chicaPos = 0;
            m_chicaIgnoreT = 0.0f;
            m_chicaNearT = 0.0f;
            Sfx("snd_walk4b", false, 5, 100);
        }
    } else m_chicaIgnoreT = 0.0f;

    // the shut-door visit: he comes, then leaves after 3 s (g197-200)
    if (m_leftShut && m_bonniePos == 1) {
        m_bonnieVisitT += dt;
        if (m_bonnieVisitT >= 3.0f) {
            m_bonnieVisitT = 0.0f;
            m_bonniePos = 0;
            Sfx("snd_walk1b", false, 5, 100);
        }
    } else m_bonnieVisitT = 0.0f;
    if (m_rightShut && m_chicaPos == 1) {
        m_chicaVisitT += dt;
        if (m_chicaVisitT >= 3.0f) {
            m_chicaVisitT = 0.0f;
            m_chicaPos = 0;
            Sfx("snd_walk3b", false, 5, 100);
        }
    } else m_chicaVisitT = 0.0f;

    // the linger timers: +1 per second at near, spent by lying down
    // (g298/303, alterable[6] > 20 + Night*4)
    const i32 kLinger = 20 + m_night * 4;
    if (m_bonniePos == 1) {
        m_bonnieNearT += dt;
        if (m_peeking && m_pos == P_LEFT) m_bonnieNearT = 0.0f;   // g303
        if (m_bonnieNearT > (f32)kLinger) m_bonnieBed = true;
    }
    if (m_chicaPos == 1) {
        m_chicaNearT += dt;
        if (m_peeking && m_pos == P_RIGHT) m_chicaNearT = 0.0f;   // g298-300
        if (m_chicaNearT > (f32)kLinger) m_chicaBed = true;
    }
}

// ------------------------------------------------------------
// the closet lane (g92-148, 255-258; Fredbear per g97/111/433/444)
// ------------------------------------------------------------

void FNaF4Game::TickFoxy(f32 dt, bool viewingCloset) {
    if (m_gameover) return;

    // the entry roll (g119): every 5 s, Random(10)+1 <= AI; Fredbear takes
    // the lane when his AI is up (g111/444)
    if (!m_foxyIn) {
        const i32 ai = m_fredbearAI > 0 ? m_fredbearAI : m_foxyAI;
        if (ai > 0) {
            m_foxyRollT += dt;
            if (m_foxyRollT >= 5.0f) {
                m_foxyRollT = 0.0f;
                if ((rand() % 10) + 1 <= ai) {
                    m_foxyIn = true;
                    m_closetCounter = 3 + rand() % 5;      // g146
                    Sfx("snd_foxyrun1", false, 5, 100);
                    Sfx("snd_runandenter", false, 5, 100);
                }
            }
        }
        return;
    }

    // the stage counter: -1/s watched, +1/s unwatched (g136-140), 0..10
    m_closetTickT += dt;
    if (m_closetTickT >= 1.0f) {
        m_closetTickT = 0.0f;
        if (viewingCloset) {
            if (m_closetCounter > 0) m_closetCounter -= 1;
        } else {
            if (m_closetCounter < 10) m_closetCounter += 1;
        }
        if (m_closetCounter >= 10) m_foxyGotYou = 1;   // g147
        if (m_closetCounter < 10)  m_foxyGotYou = 0;   // g148
    }

    // the bite: flashing while he is at stage 3 (g255) — non-fatal, but it
    // does not stop the drain; Fredbear's version kills (g371)
    if (viewingCloset && m_closetCounter >= 6 && m_fredbearSndT <= 0.0f) {
        m_fredbearSndT = 3.0f;
        if (m_fredbearAI > 0) {
            TriggerJumpscare(358);                     // the fredbear scare
        } else {
            Sfx("snd_screamshort", false, 16, 100);
            m_biteT = 1.2f;                            // the bite overlay
        }
    }
    if (m_fredbearSndT > 0.0f) m_fredbearSndT -= dt;
}

// ------------------------------------------------------------
// the bed lane (g227-254, 399, 433)
// ------------------------------------------------------------

void FNaF4Game::TickFreddy(f32 dt, bool viewingBed) {
    if (m_gameover || m_fredbearAI > 0) return;

    m_freddyT += dt;
    if (m_freddyT >= 4.0f) {          // g227: timer 4000, not viewing bed
        m_freddyT = 0.0f;
        if (!viewingBed) m_freddyCounter += m_freddyAI;
    }
    if (viewingBed) {                 // g231: drains 1/s while you look
        m_bedTickT += dt;
        if (m_bedTickT >= 1.0f) {
            m_bedTickT = 0.0f;
            if (m_freddyCounter > 0) m_freddyCounter -= 1;
        }
    }

    // the minimonsters loop (g247/248/251/252)
    if (viewingBed && m_freddyCounter >= 10)      ChVol(21, 50);
    else if (!viewingBed && m_freddyCounter >= 30) ChVol(21, 2);
    else if (!viewingBed && m_freddyCounter >= 20) ChVol(21, 1);
    else                                          ChVol(21, 0);

    // the bed attack (g253/254): counter >= 60 while in bed
    if (m_freddyCounter >= 60 && m_pos == P_BED && m_walkT <= 0.0f)
        TriggerJumpscare(620);

    // the hallucination wall (g283-288): counter >= 80 — DEVIATION: the
    // dump's delayed forced flash collapsed to an immediate attack
    if (m_freddyCounter >= 80)
        TriggerJumpscare(m_fredbearAI > 0 ? 358 : 368);
}

// ------------------------------------------------------------
// paranoia (g259-288)
// ------------------------------------------------------------

void FNaF4Game::TickParanoia(f32 dt) {
    m_dangerT += dt;
    if (m_dangerT >= 4.0f) {          // g275: every 4 s
        m_dangerT = 0.0f;
        i32 total = 0;
        total += (m_bonniePos == 1) ? 2 : (m_bonniePos == 0 ? 1 : 0);
        total += (m_chicaPos  == 1) ? 2 : (m_chicaPos  == 0 ? 1 : 0);
        if (m_foxyIn) total += 2;
        if (m_freddyCounter < 20)      total += 0;
        else if (m_freddyCounter < 30) total += 1;
        else if (m_freddyCounter < 50) total += 3;
        else                           total += 5;
        m_totalDanger = total;
        // the flash schedule (g276-279): 5/4/3/2 s by danger
        if (total >= 5)      m_bfTimer = 2.0f;
        else if (total == 4) m_bfTimer = 3.0f;
        else if (total == 3) m_bfTimer = 4.0f;
        else if (total == 2) m_bfTimer = 5.0f;
    }

    if (m_totalDanger >= 2) {
        m_bfTimer -= dt;
        if (m_bfTimer <= 0.0f && m_bfHold <= 0.0f) {
            // the show length (g276-279): (5+R5)/(10+R10)/(10+R10)/(10+R30)
            i32 frames;
            if      (m_totalDanger >= 5) frames = 10 + rand() % 30;
            else if (m_totalDanger == 4) frames = 10 + rand() % 10;
            else if (m_totalDanger == 3) frames = 10 + rand() % 10;
            else                         frames = 5  + rand() % 5;
            m_bfHold = (f32)frames / 60.0f;
        }
    }
    if (m_bfHold > 0.0f) {
        m_bfHold -= dt;
        m_blackFlashA = (m_bfHold > 0.1f) ? 255.0f
                       : (m_bfHold / 0.1f) * 255.0f;   // fast fade
    } else {
        m_blackFlashA = 0.0f;
    }
}

} // namespace fnaf
