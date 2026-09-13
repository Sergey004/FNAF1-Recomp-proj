/**
 * FNaF2Game.cpp: v2.31 — the FNAF2 state skeleton (see FNaF2Game.h).
 * Only the loop systems are live: title menu, night card, the office
 * clock (70 s/hour), the hold-flashlight with its battery and the BB
 * steal flag. The AI counters from the dump (old/new animatronics, BB,
 * Sockpuppet, Golden Freddy) plug into Tick in their own stages.
 */

#include "FNaF2Game.h"

namespace fnaf {

// dump constants (see header)
static const i32 kBatteryMax   = 7000;   // group 34x: battery life := 7000
static const f32 kSecondsPerHour = 70.0f; // "AM" alterable[0] >= 70 (g484/485)
static const f32 kCardSeconds  = 2.5f;    // night card hold (approx; pin from frame 2)
static const f32 kSixAmSeconds = 5.0f;    // 6AM cheer hold (approx; pin from frame 5)

FNaF2Game::FNaF2Game() { ResetToTitle(); }

void FNaF2Game::ResetToTitle() {
    m_screen = SCR_DISCLAIMER;   // v2.32: every game opens with ITS warning
    m_cardT = 0.0f;
    m_night = 1;
    m_timeOfNight = 12;
    m_amClock = 0.0f;
    m_batteryLife = kBatteryMax;
    m_litQ = 0;
    m_viewing = 0;
    m_inDanger = 0;
    m_maskOn = false;
    m_bbGotLight = false;
    m_optionSelected = 0;
    m_cardT = 0.0f;
    m_time = 0.0f;
}

void FNaF2Game::ToggleMonitor() {
    m_viewing = (m_viewing == 0) ? 1 : 0;   // raise -> CAM 01 (group 130-style)
}

void FNaF2Game::CycleCam(i32 dir) {
    if (m_viewing == 0) return;
    m_viewing += dir;
    if (m_viewing < 1)  m_viewing = 12;
    if (m_viewing > 12) m_viewing = 1;
}

void FNaF2Game::StartNight(i32 night) {
    m_night = night;
    m_screen = SCR_NIGHTSTART;
    m_cardT = 0.0f;
    // night-start init (frame-start groups): 12AM, full battery
    m_timeOfNight = 12;
    m_amClock = 0.0f;
    m_batteryLife = kBatteryMax;
    m_litQ = 0;
    m_viewing = 0;
    m_inDanger = 0;
    m_maskOn = false;
    m_bbGotLight = false;
}

void FNaF2Game::Tick(f32 dt, const FNaF2Inputs& in) {
    m_time += dt;

    switch (m_screen) {
        case SCR_DISCLAIMER: {
            // frame 0 "Frame 17": hold ~3.5 s or any key (group flow -> title)
            m_cardT += dt;
            if (m_cardT >= 3.5f || in.aPressed || in.upPressed || in.downPressed) {
                m_screen = SCR_TITLE;
                m_cardT = 0.0f;
            }
            break;
        }

        case SCR_TITLE: {
            // selector: New Game / Custom Night (locked until beaten 6)
            if (in.upPressed || in.downPressed)
                m_optionSelected = (m_optionSelected == 0) ? 1 : 0;
            if (in.aPressed && m_optionSelected == 0)
                StartNight(1);          // fresh profile: Night 1 (save stage later)
            break;
        }

        case SCR_NIGHTSTART: {
            m_cardT += dt;
            if (m_cardT >= kCardSeconds) {
                m_screen = SCR_OFFICE;
                m_cardT = 0.0f;
            }
            break;
        }

        case SCR_OFFICE: {
            // monitor: A toggles (raise -> CAM 01), D-pad cycles cams;
            // the flashlight is blocked while viewing != 0 (group 35)
            if (in.aPressed)    ToggleMonitor();
            if (in.leftPressed)  CycleCam(-1);
            if (in.rightPressed) CycleCam(1);

            // ---- flashlight, group 35/36: HOLD, blocked by battery,
            // mask, monitor up, "in danger", BB having the light ----
            const bool canLight =
                m_batteryLife > 0 && !m_maskOn && m_inDanger == 0 &&
                !m_bbGotLight && m_viewing == 0;
            m_litQ = (in.lightHeld && canLight) ? 1 : 0;

            // ---- battery, group 170: -1 per frame while lit ----
            if (m_litQ) {
                m_batteryLife -= 1;
                if (m_batteryLife < 0) m_batteryLife = 0;
            }

            // ---- clock, groups 484/485: "AM" >= 70 -> hour change ----
            m_amClock += dt;
            if (m_amClock >= kSecondsPerHour) {
                m_amClock -= kSecondsPerHour;
                m_timeOfNight = (m_timeOfNight == 12) ? 1 : m_timeOfNight + 1;
                if (m_timeOfNight == 6) {
                    m_screen = SCR_6AM;     // survived to 6 AM
                    m_cardT = 0.0f;
                    break;
                }
            }
            break;
        }

        case SCR_6AM: {
            m_cardT += dt;
            if (m_cardT >= kSixAmSeconds) ResetToTitle();
            break;
        }
    }
}

} // namespace fnaf
