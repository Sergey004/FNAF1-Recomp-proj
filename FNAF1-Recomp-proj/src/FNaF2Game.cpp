/**
 * FNaF2Game.cpp: v2.59 — FNAF2 wave 1 (playable core). See FNaF2Game.h
 * and docs/FNAF2_MECHANICS.md. Every rule cites its office-frame group.
 */

#include "FNaF2Game.h"
#include <cstdlib>

namespace fnaf {

static const f32 kSecondsPerHour = 70.0f;   // g483/484/485: AM +1/s, >=70 -> hour
static const f32 kCardSeconds    = 2.6f;    // frame 2: alt0 > 130 ticks
static const f32 kSixAmSeconds   = 5.0f;    // frame 4 "static" hold
static const f32 kMaskT          = 0.30f;   // mask anims ~9 frames @ spd75
static const i32 kMusicMax       = 2000;    // music button.alt0 cap (g556)

// night drain per 50 ms tick, not winding (g494-501): N1..N7+
static const i32 kMusicDrain[8]  = { 0, 2, 2, 3, 4, 5, 6, 6 };
// battery max per night (g688-692)
static const i32 kBatteryByNight[8] = { 7000, 7000, 6000, 5000, 4000, 3000, 3000, 3000 };
// time allowed (danger window frames) per night (g388-394)
static const i32 kTimeAllowed[8] = { 100, 100, 80, 60, 55, 50, 50, 45 };

static i32 NightIdx(i32 night) { return (night < 1) ? 1 : (night > 7 ? 7 : night); }

FNaF2Game::FNaF2Game() { ResetToTitle(); }

void FNaF2Game::Sfx(const char* s, bool loop, i32 ch, i32 vol) {
    if (audio.play) audio.play(s, loop, ch, vol);
}
void FNaF2Game::SfxStop(const char* s) { if (audio.stop) audio.stop(s); }
void FNaF2Game::ChVol(i32 ch, i32 vol) { if (audio.channelVolume) audio.channelVolume(ch, vol); }

void FNaF2Game::ResetToTitle() {
    m_screen = SCR_DISCLAIMER;
    m_cardT = 0.0f;
    m_night = 1;
    m_time = 0.0f;
    m_optionSelected = 0;
    m_optionCount = 2;   // dump title rows: new game + continue ALWAYS visible
    m_lastNight = 1;
    m_beat5 = false;
    m_beat6 = false;
    m_phoneMuted = false;
    for (i32 i = 0; i < C_COUNT; ++i) m_chars[i] = CharState();
}

void FNaF2Game::InitNightState() {
    // frame-start groups: hour 12 (g486), night re-read (g487), battery by
    // night (g688-692), music gauge 2000 (g493), positions re-init (g194),
    // all AI zeroed then scheduled (g517+).
    m_timeOfNight = 12;
    m_amClock = 0.0f;
    m_batteryMax  = kBatteryByNight[NightIdx(m_night)];
    m_batteryLife = m_batteryMax;
    m_litQ = 0;
    m_viewing = 0;
    m_maskState = 0; m_maskT = 0.0f;
    m_musicGauge = (f32)kMusicMax;
    m_musicDrainAcc = 0.0f;
    m_musicDanger = false;
    m_musicWindT = 0.0f;
    m_inDanger = 0;
    m_attacker = 0;
    m_timeAllowed = kTimeAllowed[NightIdx(m_night)];
    m_timeLeft = 0.0f;
    m_gotYouStage = 0;
    m_dangerFrames = 0.0f;
    m_scareT = -1.0f; m_scareAnim = 0;
    m_ventL = m_ventR = 0; m_ventLT = m_ventRT = 0.0f;
    m_toxic = 0; m_toxicAcc = 0.0f;
    m_blackoutTimer = 0;
    m_randomImage = 0;
    m_shadowT = -1.0f;
    m_freddyUnderTable = false;
    m_mangleView = 0;
    m_toyBonnieScare = false;
    m_moveStatic = 0.0f;
    m_puppetWalking = false;
    m_aiRollT = 0.0f; m_boxRollT = 0.0f; m_puppetRollT = 0.0f; m_occT = 0.0f;
    m_officeOccupied = false;
    for (i32 i = 0; i < C_COUNT; ++i) m_chars[i] = CharState();
    // start positions (g194): old trio cam 8, toys cam 9, mangle cam 12,
    // puppet cam 11, BB cam 10 (per the movement graphs' tails)
    m_chars[C_OLD_FREDDY].room = R_CAM8;  m_chars[C_OLD_BONNIE].room = R_CAM8;
    m_chars[C_OLD_CHICA].room  = R_CAM8;  m_chars[C_OLD_FOXY].room  = R_CAM8;
    m_chars[C_TOY_FREDDY].room = R_CAM9;  m_chars[C_TOY_BONNIE].room = R_CAM9;
    m_chars[C_TOY_CHICA].room  = R_CAM9;  m_chars[C_MANGLE].room    = R_CAM12;
    m_chars[C_BB].room         = R_CAM10; m_chars[C_PUPPET].room     = R_CAM11;
    m_chars[C_GOLDEN].room     = R_NONE;
}

void FNaF2Game::StartNight(i32 night) {
    m_night = night;
    m_screen = SCR_NIGHTSTART;
    m_cardT = 0.0f;
    InitNightState();
}

void FNaF2Game::ResetNightInPlace() {
    // dump G450-458: the office jumps to itself — the night restarts with
    // everything re-initialized (same night).
    InitNightState();
}

// ---------------------------------------------------------------------------
// AI schedule (g518-528): per night/hour AI levels. Zeroed every loop for
// nights != 7 (g517), night 7 loads the custom counters (g622).
// ---------------------------------------------------------------------------
static void ApplySchedule(FNaF2Game::CharState* c, i32 night, i32 hour) {
    // H = the in-game hour as the dump counts it (12 then 1..5); the
    // schedule keys off the "time of the night" value.
    switch (night) {
        case 1:
            if (hour == 2) { c[FNaF2Game::C_TOY_BONNIE].ai = 2; c[FNaF2Game::C_TOY_CHICA].ai = 2; }
            if (hour == 3) { c[FNaF2Game::C_TOY_BONNIE].ai = 3; c[FNaF2Game::C_TOY_FREDDY].ai = 2; }
            break;
        case 2:
            if (hour == 1) {
                c[FNaF2Game::C_TOY_FREDDY].ai = 2; c[FNaF2Game::C_TOY_BONNIE].ai = 3;
                c[FNaF2Game::C_OLD_FOXY].ai = 1;   c[FNaF2Game::C_BB].ai = 3;
                c[FNaF2Game::C_MANGLE].ai = 3;     c[FNaF2Game::C_TOY_CHICA].ai = 3;
                c[FNaF2Game::C_GOLDEN].ai = 1 + (rand() % 10);   // rnd, capped 10
            }
            break;
        case 3:
            c[FNaF2Game::C_OLD_BONNIE].ai = 1; c[FNaF2Game::C_BB].ai = 1;
            c[FNaF2Game::C_OLD_CHICA].ai = 1;  c[FNaF2Game::C_OLD_FOXY].ai = 2;
            c[FNaF2Game::C_GOLDEN].ai = 1 + (rand() % 10);
            if (hour == 1) {
                c[FNaF2Game::C_OLD_FREDDY].ai = 2; c[FNaF2Game::C_OLD_CHICA].ai = 2;
                c[FNaF2Game::C_OLD_FOXY].ai = 3;   c[FNaF2Game::C_TOY_BONNIE].ai = 1;
                c[FNaF2Game::C_OLD_BONNIE].ai = 3; c[FNaF2Game::C_BB].ai = 2;
                c[FNaF2Game::C_TOY_CHICA].ai = 1;
            }
            break;
        case 4:
            c[FNaF2Game::C_MANGLE].ai = 5;     c[FNaF2Game::C_BB].ai = 3;
            c[FNaF2Game::C_OLD_FOXY].ai = 7;   c[FNaF2Game::C_OLD_BONNIE].ai = 1;
            c[FNaF2Game::C_GOLDEN].ai = 1 + (rand() % 100);
            if (hour == 2) {
                c[FNaF2Game::C_OLD_CHICA].ai = 4; c[FNaF2Game::C_OLD_FREDDY].ai = 3;
                c[FNaF2Game::C_OLD_BONNIE].ai = 4; c[FNaF2Game::C_TOY_BONNIE].ai = 1;
            }
            break;
        case 5:
            c[FNaF2Game::C_TOY_FREDDY].ai = 5; c[FNaF2Game::C_OLD_FREDDY].ai = 2;
            c[FNaF2Game::C_OLD_BONNIE].ai = 2; c[FNaF2Game::C_OLD_CHICA].ai = 2;
            c[FNaF2Game::C_MANGLE].ai = 1;     c[FNaF2Game::C_BB].ai = 5;
            c[FNaF2Game::C_OLD_FOXY].ai = 5;   c[FNaF2Game::C_GOLDEN].ai = 1 + (rand() % 100);
            c[FNaF2Game::C_TOY_BONNIE].ai = 2; c[FNaF2Game::C_TOY_CHICA].ai = 2;
            if (hour == 1) {
                c[FNaF2Game::C_OLD_FREDDY].ai = 5; c[FNaF2Game::C_OLD_BONNIE].ai = 5;
                c[FNaF2Game::C_OLD_CHICA].ai = 5;  c[FNaF2Game::C_OLD_FOXY].ai = 7;
                c[FNaF2Game::C_MANGLE].ai = 10;    c[FNaF2Game::C_TOY_FREDDY].ai = 1;
            }
            break;
        case 6:
            c[FNaF2Game::C_OLD_FREDDY].ai = 5; c[FNaF2Game::C_OLD_BONNIE].ai = 5;
            c[FNaF2Game::C_OLD_CHICA].ai = 5;  c[FNaF2Game::C_MANGLE].ai = 3;
            c[FNaF2Game::C_BB].ai = 5;         c[FNaF2Game::C_OLD_FOXY].ai = 10;
            c[FNaF2Game::C_GOLDEN].ai = 1 + (rand() % 10);
            if (hour == 2) {
                c[FNaF2Game::C_OLD_FREDDY].ai = 10; c[FNaF2Game::C_OLD_BONNIE].ai = 10;
                c[FNaF2Game::C_OLD_CHICA].ai = 10;  c[FNaF2Game::C_TOY_BONNIE].ai = 5;
                c[FNaF2Game::C_TOY_CHICA].ai = 5;   c[FNaF2Game::C_BB].ai = 9;
                c[FNaF2Game::C_MANGLE].ai = 10;     c[FNaF2Game::C_TOY_FREDDY].ai = 5;
                c[FNaF2Game::C_GOLDEN].ai = 3;      c[FNaF2Game::C_OLD_FOXY].ai = 15;
            }
            break;
        default: break;   // night 7: custom counters (the menu wrote them)
    }
}

void FNaF2Game::DispatchAttack(i32 ch) {
    if (m_attacker != 0 || m_scareT >= 0.0f) return;
    m_attacker = ch;
    // attack animation value: 11 + attacker id (1..9); Golden (12) -> 21
    m_scareAnim = (ch == C_GOLDEN) ? 21 : 11 + ch;
    m_scareT = 0.0f;
    Sfx("snd_Xscream3", false, 12, 100);
    if (ch == C_PUPPET) { /* puppet kill flag for the post-death frames */ }
}

// one movement-graph node per character (dump g239-300); returns false when
// the step's conditions aren't met (the arm stays for the next roll).
bool FNaF2Game::AdvanceCharStep(i32 ch) {
    CharState& c = m_chars[ch];
    const bool hallLit = (m_litQ != 0);   // office hall light freezes entries
    const bool monUp = (m_viewing != 0);

    switch (ch) {
        case C_OLD_FREDDY:
            // g245/250: waits until old Bonnie AND old Chica left cam 8
            if (c.room == R_CAM8) {
                if (m_chars[C_OLD_BONNIE].room == R_CAM8 ||
                    m_chars[C_OLD_CHICA].room == R_CAM8) return false;
                if (hallLit) return false;
                c.room = R_CAM7; return true;
            }
            if (c.room == R_CAM7) { if (hallLit) return false; c.room = R_CAM3; return true; }
            if (c.room == R_CAM3) {
                if (hallLit) return false;
                c.room = (rand() % 2 == 0) ? R_HALL2 : R_CAM7;   // decide path coin
                return true;
            }
            if (c.room == R_HALL2) {
                // g244: enters the office ONLY while the monitor is up
                if (!monUp || m_officeOccupied || m_inDanger) return false;
                c.room = R_OFFICE; m_freddyUnderTable = true; m_officeOccupied = true;
                return true;
            }
            return false;

        case C_OLD_BONNIE:
            if (c.room == R_CAM8) {
                if (m_chars[C_OLD_FREDDY].alt0 == 1) return false;  // g245 gate
                if (hallLit) return false;
                c.room = R_CAM7; return true;
            }
            if (c.room == R_CAM7)  { if (hallLit) return false; c.room = R_HALL1; return true; }
            if (c.room == R_HALL1) { if (hallLit) return false; c.room = R_CAM1;  return true; }
            if (c.room == R_CAM1)  { c.room = R_CAM5; return true; }
            if (c.room == R_CAM5)  {
                if (!monUp || m_officeOccupied || m_inDanger) return false;
                c.room = R_OFFICE; m_officeOccupied = true; return true;
            }
            return false;

        case C_OLD_CHICA:
            if (c.room == R_CAM8) {
                if (m_chars[C_OLD_FREDDY].alt0 == 1) return false;
                if (hallLit) return false;
                c.room = R_CAM4; return true;
            }
            if (c.room == R_CAM4) { c.room = R_CAM2; return true; }
            if (c.room == R_CAM2) { if (hallLit) return false; c.room = R_CAM6; Sfx("snd_ventwalk1", false, 15, 80); return true; }
            if (c.room == R_CAM6) {
                if (!monUp || m_officeOccupied || m_inDanger) return false;
                c.room = R_OFFICE; m_officeOccupied = true; return true;
            }
            return false;

        case C_OLD_FOXY:
            // g254/255: cam 8 -> hall 1 -> box (both need the light OFF)
            if (c.room == R_CAM8) {
                if (hallLit) { c.alt3 = 0.0f; return false; }   // g686 drain
                c.room = R_HALL1; return true;
            }
            if (c.room == R_HALL1) {
                if (hallLit) return false;
                c.room = R_BOX;
                // who got you = 4 (old Foxy) for the post-death frames
                return true;
            }
            return false;

        case C_TOY_FREDDY:
            if (c.room == R_CAM9) {
                if (m_chars[C_TOY_CHICA].room == R_CAM9) return false;  // g215/217
                c.room = R_CAM10; return true;
            }
            if (c.room == R_CAM10) { if (hallLit) return false; c.room = R_HALL1; return true; }
            if (c.room == R_HALL1) { if (hallLit) return false; c.room = R_HALL2; return true; }
            if (c.room == R_HALL2) {
                if (!monUp || m_officeOccupied || m_inDanger) return false;
                c.room = R_OFFICE; m_officeOccupied = true; return true;
            }
            return false;

        case C_TOY_BONNIE:
            if (c.room == R_CAM9) {
                if (m_chars[C_TOY_CHICA].room == R_CAM9) return false;  // g219/221
                c.room = R_CAM3; return true;
            }
            if (c.room == R_CAM3)  { c.room = R_CAM4; return true; }
            if (c.room == R_CAM4)  { c.room = R_CAM2; return true; }
            if (c.room == R_CAM2)  { if (hallLit) return false; c.room = R_CAM6; return true; }
            if (c.room == R_CAM6)  {
                if (!monUp || m_officeOccupied || m_inDanger) return false;
                c.room = R_OFFICE; m_officeOccupied = true;
                m_toyBonnieScare = true; return true;
            }
            return false;

        case C_TOY_CHICA:
            if (c.room == R_CAM9) {
                if (m_chars[C_TOY_BONNIE].room == R_CAM9) return false;
                c.room = R_CAM7; return true;
            }
            if (c.room == R_CAM7)  { if (hallLit) return false; c.room = R_HALL1; return true; }
            if (c.room == R_HALL1) { if (hallLit) return false; c.room = R_CAM1;  return true; }
            if (c.room == R_CAM1)  { c.room = R_CAM5; return true; }
            if (c.room == R_CAM5)  {
                if (!monUp || m_officeOccupied || m_inDanger) return false;
                c.room = R_OFFICE; m_officeOccupied = true; return true;
            }
            return false;

        case C_MANGLE:
            if (c.room == R_CAM12) { c.room = R_CAM11; return true; }
            if (c.room == R_CAM11) { c.room = R_CAM10; return true; }
            if (c.room == R_CAM10) { c.room = R_CAM7;  return true; }
            if (c.room == R_CAM7)  { if (hallLit) return false; c.room = R_HALL1; return true; }
            if (c.room == R_HALL1) { if (hallLit) return false; c.room = R_CAM2;  return true; }
            if (c.room == R_CAM2)  { c.room = (rand() % 2 == 0) ? R_CAM6 : R_CAM1; return true; }
            if (c.room == R_CAM6 || c.room == R_CAM1) {
                // g267/268: waits for the monitor flip to finish, then the box
                if (m_maskState != 0) return false;
                c.room = R_BOX;
                return true;
            }
            return false;

        case C_BB:
            if (c.room == R_CAM10) { c.room = R_CAM7; return true; }
            if (c.room == R_CAM7)  { c.room = R_CAM3; return true; }
            if (c.room == R_CAM3)  { c.room = R_CAM1; return true; }
            if (c.room == R_CAM1)  { c.room = R_CAM5; return true; }
            if (c.room == R_CAM5)  {
                if (!monUp || m_officeOccupied) return false;
                c.room = R_OFFICE; m_officeOccupied = true; return true;
            }
            return false;

        case C_PUPPET: {
            // g269-276: emerges by the music box, walks cam 11 -> 10 -> 7 ->
            // {3|4 by route} -> {1|2} -> office -> (10%/s) -> box
            if (c.alt18 < 3) return false;   // not fully out yet
            if (c.room == R_CAM11) { c.room = R_CAM10; m_puppetWalking = true; return true; }
            if (c.room == R_CAM10) { c.room = R_CAM7;  return true; }
            if (c.room == R_CAM7)  { c.room = (rand() % 2 == 0) ? R_CAM3 : R_CAM4; return true; }
            if (c.room == R_CAM3 || c.room == R_CAM4) {
                c.room = (rand() % 2 == 0) ? R_CAM1 : R_CAM2; return true;
            }
            if (c.room == R_CAM1 || c.room == R_CAM2) { c.room = R_OFFICE; return true; }
            if (c.room == R_OFFICE) {
                // g479: 10% per second -> the box; the mask does NOT stop him
                if ((rand() % 100) < 10) { c.room = R_BOX; return true; }
                return false;
            }
            return false;
        }

        case C_GOLDEN:
            return false;   // wave 1: hallucination only, no movement graph

        default: return false;
    }
}

void FNaF2Game::RetreatChar(i32 ch) {
    CharState& c = m_chars[ch];
    // masked/unlit retreats: back to the pre-office room with a huge cooldown
    if (c.room == R_OFFICE) {
        m_officeOccupied = false;
        if (ch == C_OLD_FREDDY) m_freddyUnderTable = false;
        if (ch == C_TOY_BONNIE) m_toyBonnieScare = false;
        c.room = (ch == C_OLD_FREDDY) ? R_CAM3 :
                 (ch == C_OLD_BONNIE) ? R_CAM5 :
                 (ch == C_OLD_CHICA)  ? R_CAM6  :
                 (ch == C_TOY_FREDDY) ? R_CAM9  :
                 (ch == C_TOY_BONNIE) ? R_CAM6  :
                 (ch == C_TOY_CHICA)  ? R_CAM5  :
                 (ch == C_MANGLE)     ? R_CAM7  : R_NONE;
    } else if (c.room == R_HALL1 || c.room == R_HALL2) {
        c.room = (ch == C_OLD_FREDDY) ? R_CAM3 : R_CAM8;
    }
    c.alt1 = 500 + rand() % 500;
    c.alt0 = 0;
}

void FNaF2Game::TickAI(f32 dt) {
    // g517/518-528: schedule re-applied each loop (nights != 7); night 7 keeps
    // the custom counters from the menu.
    if (m_night != 7) {
        for (i32 i = 0; i < C_COUNT; ++i) m_chars[i].ai = 0;
        ApplySchedule(m_chars, m_night, m_timeOfNight);
    }
    // caps (g651/652/678-685)
    for (i32 i = 0; i < C_COUNT; ++i) {
        i32 cap = (i == C_OLD_FOXY) ? 17 : (i == C_GOLDEN ? 10 : 15);
        if (m_chars[i].ai > cap) m_chars[i].ai = cap;
    }
    if (m_night < 6) m_chars[C_GOLDEN].ai = 0;   // g626

    CharState& foxy = m_chars[C_OLD_FOXY];
    // old Foxy dark-charge (g646/647): +1/s unlit; DOUBLE while masking with
    // a clear office; drains at cam 8 when the hall is lit (handled above).
    if (m_litQ == 0) {
        f32 rate = 1.0f;
        if (m_maskState == 2 && !m_officeOccupied) rate = 2.0f;
        foxy.alt3 += rate * dt;
    }

    // 5 s opportunity rolls (g198-208)
    m_aiRollT += dt;
    if (m_aiRollT >= 5.0f) {
        m_aiRollT -= 5.0f;
        for (i32 i = 0; i < C_COUNT; ++i) {
            CharState& c = m_chars[i];
            if (c.ai <= 0 || c.alt0 != 0 || c.alt1 > 0) continue;
            bool hit = false;
            if (i == C_OLD_FOXY) {
                // charge-weighted (g202): Random(21) + Random(5)*charge <= AI
                hit = ((rand() % 21) + (rand() % 5) * (i32)foxy.alt3) <= c.ai;
            } else if (i == C_MANGLE || i == C_BB || i == C_GOLDEN) {
                hit = ((rand() % 20) + 1) < c.ai;            // strict <
            } else {
                hit = ((rand() % 20) + 1) <= c.ai;
            }
            if (hit) c.alt0 = 1;
        }
    }

    // arming -> one node (g209-225 + g226-238 cooldown ticks)
    for (i32 i = 0; i < C_COUNT; ++i) {
        CharState& c = m_chars[i];
        if (c.alt1 > 0) { c.alt1 -= 1; continue; }
        if (c.alt0 != 1) continue;
        // watched-gates: stage characters don't move while you watch them
        if (m_viewing == c.room && m_viewing != 0 &&
            (i == C_OLD_FREDDY || i == C_TOY_FREDDY ||
             i == C_TOY_BONNIE || i == C_TOY_CHICA)) {
            c.alt1 = 30;   // "stun time" beat (g315-322 approximation)
            continue;
        }
        if (i == C_MANGLE && m_viewing == c.room) { c.alt0 = 0; continue; } // g222/223
        c.alt0 = 2;
        if (AdvanceCharStep(i)) {
            c.alt0 = 0;
            c.alt1 = 0;
            // v2.59: a movement in view = a feed static burst (g333-342)
            if (m_viewing != 0 && m_viewing == c.room)
                m_moveStatic = (f32)(20 + rand() % 100) / 100.0f;
        } else {
            c.alt0 = 1;   // stays armed; retried next roll
        }
    }

    // old Foxy at hall 1 under the light: alt3=0, alt9 += 1/frame; retreat
    // at alt9 > 100+night (g590/g668)
    if (foxy.room == R_HALL1 && m_litQ) {
        foxy.alt3 = 0.0f;
        foxy.alt9 += 1;
        if (foxy.alt9 > 100 + m_night) {
            foxy.alt9 = 0;
            foxy.room = R_CAM8;
            foxy.alt1 = 500 + rand() % 500;
            foxy.alt0 = 0;
            Sfx("snd_metalrun", false, 19, 100);
        }
    } else if (foxy.room != R_HALL1) {
        foxy.alt9 = 0;
    }

    // old Freddy monitor-up timer (g620/621 + g407-410): while he is at
    // hall stage 2 and you keep the monitor up too long -> straight to box
    {
        CharState& f = m_chars[C_OLD_FREDDY];
        if (f.room == R_HALL2 && m_viewing != 0) {
            f.alt25 += dt;
            const f32 limit = 2.0f + 20.0f * (f32)((rand() % m_night) + 1) / 20.0f;
            if (f.alt25 >= limit) { f.room = R_BOX; f.alt25 = 0.0f; }
        } else f.alt25 = 0.0f;
    }

    // old Foxy reaching the box = instant attack (g255: who got you = 4)
    if (m_chars[C_OLD_FOXY].room == R_BOX && m_attacker == 0 && m_scareT < 0.0f) {
        DispatchAttack(C_OLD_FOXY);
    }
    // Mangle at the box = attack (g267/268: who got you = 5)
    if (m_chars[C_MANGLE].room == R_BOX && m_attacker == 0 && m_scareT < 0.0f) {
        DispatchAttack(C_MANGLE);
    }
    // Puppet at the box = attack (g439)
    if (m_chars[C_PUPPET].room == R_BOX && m_attacker == 0 && m_scareT < 0.0f) {
        DispatchAttack(C_PUPPET);
    }
    // BB at the office waits for the flip, then the box (non-lethal: he
    // just blocks the lights) — wave 1: he parks at the office.
}

void FNaF2Game::TickDanger(f32 dt) {
    // office-entered danger pipeline (g307-312, g388-402, g403-420)
    for (i32 i = 0; i < C_COUNT; ++i) {
        const i32 ch = i;
        if (m_chars[ch].room != R_OFFICE) continue;
        if (ch == C_OLD_FREDDY || ch == C_OLD_BONNIE || ch == C_OLD_CHICA) {
            if (!m_inDanger && m_viewing == 0 && m_gotYouStage == 0 && m_attacker == 0) {
                // g310-312: old anims set danger with the monitor DOWN
                m_inDanger = 1;
                m_attacker = ch + 1;   // being attacked by ids 1..3
                m_timeAllowed = kTimeAllowed[NightIdx(m_night)];
                m_timeLeft = (f32)m_timeAllowed;
                m_gotYouStage = 1;
                m_dangerFrames = 0.0f;
            }
        } else if (ch == C_TOY_FREDDY) {
            if (!m_inDanger && m_viewing == 0 && m_gotYouStage == 0 && m_attacker == 0) {
                m_inDanger = 1;
                m_attacker = 7;
                m_timeAllowed = kTimeAllowed[NightIdx(m_night)];
                m_timeLeft = (f32)m_timeAllowed;
                m_gotYouStage = 1;
                m_dangerFrames = 0.0f;
            }
        } else if (ch == C_TOY_BONNIE) {
            // g411: toy Bonnie + monitor UP -> straight to the box
            if (m_viewing != 0 && m_attacker == 0 && m_scareT < 0.0f) {
                m_chars[ch].room = R_BOX;
                DispatchAttack(5);
            }
        }
    }

    if (!m_inDanger || m_attacker == 0) return;

    // danger countdown (g395-398)
    if (m_gotYouStage == 1) {
        m_timeLeft -= dt * 60.0f;
        if (m_timeLeft <= 0.0f) m_gotYouStage = 2;
        // masked during the window: defused (g398) — attacker goes home
        if (m_maskState == 2) {
            m_gotYouStage = 0;
            m_inDanger = 0;
            const i32 ch = m_attacker - 1;
            if (m_attacker >= 1 && m_attacker <= 9) RetreatChar(ch);
            m_attacker = 0;
            m_dangerFrames = 0.0f;
            return;
        }
    } else if (m_gotYouStage == 2) {
        // the darkening overlay accumulates; at 300 -> check and move
        m_dangerFrames += dt * 60.0f;
        if (m_dangerFrames >= 300.0f) {
            const i32 ch = m_attacker - 1;
            if (m_maskState == 2 && (ch == C_OLD_FREDDY || ch == C_TOY_FREDDY)) {
                // g592-595: masked box-escape 10%/s
                if ((rand() % 100) < 10) {
                    RetreatChar(ch);
                    m_inDanger = 0; m_attacker = 0; m_gotYouStage = 0;
                    m_dangerFrames = 0.0f;
                    return;
                }
            } else if (m_maskState != 2 || ch > C_TOY_FREDDY) {
                // stage 2 -> the box (g403-420)
                m_chars[ch].room = R_BOX;
                m_inDanger = 0; m_gotYouStage = 0; m_dangerFrames = 0.0f;
                // the box kills are handled by the per-second race below
                return;
            }
        }
    }

    // the box race (g423-426 vs g592-595): per second 50% kill vs 10% escape
    m_boxRollT += dt;
    if (m_boxRollT >= 1.0f) {
        m_boxRollT -= 1.0f;
        for (i32 i = 0; i < C_COUNT; ++i) {
            if (m_chars[i].room != R_BOX) continue;
            if (m_maskState == 2 && i <= C_TOY_FREDDY && i != C_OLD_FOXY && i != C_MANGLE && i != C_PUPPET) {
                if ((rand() % 100) < 50) { DispatchAttack(i); return; }
                if ((rand() % 100) < 10) { RetreatChar(i); continue; }
            } else {
                if ((rand() % 100) < 50) { DispatchAttack(i); return; }
            }
        }
    }
}

void FNaF2Game::TickMusicBox(f32 dt, bool winding) {
    // winding (g491/492): hold the button on CAM 11: +5/frame, windup2 0.5 s
    if (winding) {
        m_musicGauge += 5.0f * dt * 60.0f;
        m_musicWindT += dt;
        if (m_musicWindT >= 0.5f) { m_musicWindT -= 0.5f; Sfx("snd_windup2", false, 12, 80); }
        if (m_musicGauge < 300.0f) m_musicGauge = 300.0f;   // g506 snap
    } else {
        // drain per 50 ms (g494-501), paused on night-1 12-1 AM (g494/g606)
        const bool n1frozen = (m_night == 1 && (m_timeOfNight == 12 || m_timeOfNight == 1));
        if (!n1frozen) {
            m_musicDrainAcc += dt;
            while (m_musicDrainAcc >= 0.05f) {
                m_musicDrainAcc -= 0.05f;
                m_musicGauge -= (f32)kMusicDrain[NightIdx(m_night)];
            }
        }
    }
    if (m_musicGauge > (f32)kMusicMax) m_musicGauge = (f32)kMusicMax;
    if (m_musicGauge < 0.0f) m_musicGauge = 0.0f;

    // Puppet emerge (g359-361): at gauge 0, per second Random(20)+1 <= AI
    CharState& pup = m_chars[C_PUPPET];
    if (m_musicGauge <= 0.0f && pup.alt18 < 3 && pup.room == R_CAM11) {
        m_puppetRollT += dt;
        if (m_puppetRollT >= 1.0f) {
            m_puppetRollT -= 1.0f;
            if ((rand() % 20) + 1 <= pup.ai) pup.alt18 += 1;
        }
    }
    // danger faces (g507-514): gauge <= 400 and he left cam 11
    m_musicDanger = (m_musicGauge <= 400.0f && pup.room != R_CAM11);

    // melody volume by cam (g460-463) + silence at 0/monitor down (g503-505)
    if (m_musicGauge <= 0.0f || m_viewing == 0) ChVol(13, 0);
    else if (m_viewing == 11) ChVol(13, 40);
    else if (m_viewing == 10 || m_viewing == 12) ChVol(13, 15);
    else if (m_viewing == 9)  ChVol(13, 5);
    else ChVol(13, 0);
}

void FNaF2Game::TickMask(f32 dt, bool wantMask) {
    // mask machinery (g162-166): 0 off -> 1 lowering -> 2 on -> 3 raising
    switch (m_maskState) {
        case 0: if (wantMask) { m_maskState = 1; m_maskT = 0.0f;
                                Sfx("snd_FENCING_43_GEN-HDF10954", false, 7, 80); } break;
        case 1: m_maskT += dt; if (m_maskT >= kMaskT) m_maskState = 2; break;
        case 2: if (!wantMask) { m_maskState = 3; m_maskT = 0.0f;
                                 Sfx("snd_FENCING_42_GEN-HDF10953", false, 7, 80); } break;
        case 3: m_maskT += dt; if (m_maskT >= kMaskT) m_maskState = 0; break;
    }
    const bool on = (m_maskState == 2);
    // mask closes the monitor (g747) and kills the flashlight (g42)
    if (on && m_viewing != 0) m_viewing = 0;
    // breathing loop (g168/169)
    if (on) ChVol(8, 60); else ChVol(8, 0);
    // toxic meter (g709-712): +1/0.5 s masked, -1/0.5 s off — cosmetic
    m_toxicAcc += dt;
    if (m_toxicAcc >= 0.5f) {
        m_toxicAcc -= 0.5f;
        if (on  && m_toxic < 20) m_toxic += 1;
        if (!on && m_toxic > 0)  m_toxic -= 1;
    }
    // BB/Mangle mask-defense (g178/g265): 10% per second
    if (on) {
        m_boxRollT += 0.0f;   // (the box race is in TickDanger)
        for (i32 i = 0; i < C_COUNT; ++i) {
            if (m_chars[i].room != R_OFFICE) continue;
            if ((i == C_BB || i == C_MANGLE || i == C_TOY_CHICA || i == C_TOY_BONNIE) &&
                (rand() % 100) < 10) {
                RetreatChar(i);
            }
        }
    }
}

void FNaF2Game::TickLights(f32 dt, bool lightHeld, bool ventLHeld, bool ventRHeld) {
    // vent lights (g187-190 + g173-175/181-183): hold, auto-off 0.2 s,
    // blocked while BB is at the desk (error)
    const bool bbDesk = HasBBInOffice();
    m_ventL = (ventLHeld && !bbDesk && m_maskState == 0) ? 1 : 0;
    m_ventR = (ventRHeld && !bbDesk && m_maskState == 0) ? 1 : 0;
    if (bbDesk && (ventLHeld || ventRHeld))
        Sfx("snd_error", false, 12, 80);
    // flashlight (g35-43): hold, blocked by battery/danger/mask/BB
    const bool canLight = m_batteryLife > 0 && m_maskState == 0 &&
                          m_inDanger == 0 && !bbDesk && m_viewing == 0;
    m_litQ = (lightHeld && canLight) ? 1 : 0;
    if (lightHeld && !m_litQ && m_batteryLife > 0 && m_maskState == 0)
        Sfx("snd_error", false, 12, 60);   // denied click (g38)
    // battery (g170): -1 per frame while lit
    if (m_litQ) {
        m_batteryLife -= 1;
        if (m_batteryLife < 0) m_batteryLife = 0;
    }
    // light hum (g52/53): buzzlight 60 lit / 0 off
    ChVol(2, m_litQ ? 60 : 0);
}

// ---------------------------------------------------------------------------
// v2.59: the scene selector — a direct transcription of the cam-view groups
// (g44-136). Groups fire in ORDER and the LAST matching one wins, so the
// table below is evaluated top-to-bottom and the final match returns.
// Presence checks read the character rooms (R_CAMn == the dump's cam zones).
// ---------------------------------------------------------------------------
i32 FNaF2Game::ComputeSceneValue() {
    const bool lit = (m_litQ != 0);
    const i32 v = m_viewing;
    i32 value = 0;
    // NOTE: the macro parameter must NOT be named "room" — it would
    // substitute the `.room` member access inside the body.
#define AT(ch, r) (m_chars[(ch)].room == (r))
#define VAL(x) value = (x)
    if (v == 9) {
        if (!lit) { if (AT(C_TOY_BONNIE,9)&&AT(C_TOY_CHICA,9)) VAL(46); else if (AT(C_TOY_CHICA,9)) VAL(39); else if (AT(C_TOY_FREDDY,9)) VAL(41); else VAL(89); }
        else      { if (AT(C_TOY_BONNIE,9)&&AT(C_TOY_CHICA,9)) VAL(12); else if (AT(C_TOY_CHICA,9)) VAL(40); else if (AT(C_TOY_FREDDY,9)) VAL(42); else VAL(89); }
    }
    if (v == 5) {
        if (!lit) VAL(13);
        else {
            if (!AT(C_TOY_CHICA,5) && !AT(C_OLD_BONNIE,5)) { if (m_randomImage != 3) VAL(14); else { VAL(96); m_blackoutTimer += 1; } }
            if (AT(C_OLD_BONNIE,5)) VAL(60);
            else if (AT(C_TOY_CHICA,5)) VAL(52);
            if (AT(C_BB,5)) VAL(100);
        }
    }
    if (v == 6) {
        if (!lit) VAL(15);
        else {
            if (AT(C_MANGLE,6)) VAL(81);
            else if (AT(C_OLD_CHICA,6)) VAL(68);
            else if (AT(C_TOY_BONNIE,6)) VAL(45);
            else VAL(16);
        }
    }
    if (v == 10) {
        if (!lit) { if (AT(C_BB,10)) VAL(17); else VAL(85); }
        else {
            if (AT(C_TOY_FREDDY,10)) { if (AT(C_BB,10)) VAL(54); else VAL(86); }
            else { if (AT(C_BB,10)) VAL(18); else VAL(87); }
        }
    }
    if (v == 12) {
        if (!lit) VAL(19);
        else { if (AT(C_MANGLE,12)) VAL(20); else VAL(80); }
    }
    if (v == 7) {
        if (!lit) { if (AT(C_TOY_CHICA,7)) { if (AT(C_OLD_BONNIE,7)) VAL(21); else VAL(48); } else VAL(21); }
        else {
            if (AT(C_OLD_BONNIE,7)) VAL(57);
            else if (AT(C_OLD_FREDDY,7) && !AT(C_TOY_CHICA,7)) VAL(70);
            else if (AT(C_TOY_CHICA,7)) VAL(49);
            else VAL(22);
        }
    }
    if (v == 8) {
        if (!lit) VAL(23);
        else {
            if (AT(C_OLD_FREDDY,8) && AT(C_OLD_FOXY,8)) {
                if (m_randomImage > 100) VAL(64); else { VAL(94); m_blackoutTimer += 1; }
            }
            else if (AT(C_OLD_BONNIE,8)) { if (AT(C_OLD_CHICA,8)) { if (AT(C_OLD_FREDDY,8)) VAL(63); else VAL(62); } else VAL(24); }
            else VAL(23);
        }
    }
    if (v == 11) {
        if (!lit) VAL(25);
        else {
            const CharState& p = m_chars[C_PUPPET];
            if (p.alt18 >= 3) { if (m_randomImage > 100) VAL(79); else { VAL(95); m_blackoutTimer += 1; } }
            else if (p.alt18 == 2) VAL(78);
            else if (p.alt18 == 1) VAL(77);
            else VAL(26);
        }
    }
    if (v == 1) {
        if (!lit) VAL(27);
        else {
            if (AT(C_OLD_BONNIE,1)) VAL(59);
            else if (AT(C_TOY_CHICA,1)) { if (AT(C_OLD_BONNIE,1)) VAL(51); else VAL(28); }
            else VAL(0);   // lit & empty matches NO group — the feed sticks (dump quirk)
        }
    }
    if (v == 3) {
        if (!lit) { if (AT(C_OLD_FREDDY,3)) VAL(71); else VAL(0); }
        else {
            if (AT(C_OLD_FREDDY,3)) VAL(72);
            else if (AT(C_TOY_BONNIE,3)) VAL(43);
            else VAL(0);
        }
    }
    if (v == 4) {
        if (!lit) { if (AT(C_TOY_BONNIE,4)) VAL(90); else VAL(31); }
        else {
            if (AT(C_OLD_CHICA,4)) { if (AT(C_TOY_BONNIE,4)) VAL(88); else VAL(67); }
            else if (AT(C_TOY_BONNIE,4)) VAL(91);
            else VAL(0);
        }
    }
    if (v == 2) {
        if (!lit) { if (AT(C_OLD_CHICA,2)) VAL(65); else VAL(0); }
        else {
            if (AT(C_OLD_CHICA,2)) { if (AT(C_TOY_BONNIE,2)) VAL(44); else VAL(66); }
            else VAL(0);
        }
    }
    if (v == 0) {
        // office (g123-136): dark office / lit hall views
        const CharState& g = m_chars[C_GOLDEN];
        const bool blur = (m_moveStatic > 0.0f);   // "hall movement" window
        if (!lit) { VAL(35); return value; }
        if (blur) { VAL(99); return value; }
        if (AT(C_OLD_FREDDY, R_HALL2)) { VAL(73); return value; }
        if (AT(C_TOY_FREDDY, R_HALL2)) { VAL(56); return value; }
        if (AT(C_GOLDEN, R_CAM8) && g.alt0 != 0) { VAL(93); return value; }  // manifested
        if (AT(C_OLD_BONNIE, R_HALL1)) { if (AT(C_OLD_FOXY, R_HALL1)) VAL(97); else VAL(58); return value; }
        if (AT(C_OLD_FOXY, R_HALL1)) { VAL(84); return value; }
        if (AT(C_MANGLE, R_HALL1)) { VAL(76); return value; }
        if (AT(C_TOY_CHICA, R_HALL1)) { VAL(50); return value; }
        if (AT(C_TOY_FREDDY, R_HALL1)) { VAL(55); return value; }
        VAL(36);   // lit empty hall
        return value;
    }
#undef AT
#undef VAL
    return value;
}

// "Active 16" anim table (handle 80) — value -> pak image handle.
i32 FNaF2Game::SceneValueImg(i32 value) {
    static const i32 kImg[102] = {
        0,
        0,0,0,0,0,0,0,0,0,0,0,               // 1..11 unused
        118, 38, 39, 32, 33, 41, 50, 77, 81, 51,   // 12..21
        58, 37, 18, 76, 40, 174, 189, 82, 83, 43,  // 22..31
        45, 80, 84, 92, 123, 166, 168, 176, 177, 178, // 32..41
        179, 85, 181, 182, 117, 180, 440, 442, 183, 443, // 42..51
        444, 79, 510, 192, 193, 194, 196, 197, 198, 199, // 52..61
        200, 201, 202, 203, 204, 205, 206, 207, 212, 209, // 62..71
        213, 211, 74, 208, 214, 69, 71, 72, 75, 222, // 72..81
        224, 244, 188, 322, 513, 325, 333, 498, 129, 130, // 82..91
        148, 521, 269, 378, 523, 599, 600, 632, 381, 0,   // 92..100+1 pad
    };
    if (value < 0 || value > 100) return 0;
    return kImg[value];
}

void FNaF2Game::TickOffice(f32 dt, const FNaF2Inputs& in) {
    // ---------------------------------------------------------------
    // v2.61: direct pad mapping — NO cursor (user call: console games use
    // buttons, not pointers). A toggles the monitor; RB holds the mask;
    // X winds the box on CAM 11; LT/RT hold the vent lights; D-pad cycles
    // the cameras; LB is the flashlight.
    // ---------------------------------------------------------------
    if (in.aPressed && m_maskState == 0) {
        const bool raising = (m_viewing == 0);
        m_viewing = raising ? 9 : 0;   // raise -> cam 9 default (g149)
        Sfx(raising ? "snd_STEREO_CASSETTE__90097701" : "snd_STEREO_CASSETTE__90097704",
            false, 7, 80);
    }
    if (in.leftPressed  && m_viewing != 0) {
        m_viewing = (m_viewing <= 1) ? 12 : m_viewing - 1;
        Sfx("snd_COMPUTER_DIGITAL_L2076605", false, 4, 80);   // cam-switch blip (ch4)
    }
    if (in.rightPressed && m_viewing != 0) {
        m_viewing = (m_viewing >= 12) ? 1 : m_viewing + 1;
        Sfx("snd_COMPUTER_DIGITAL_L2076605", false, 4, 80);
    }

    TickMask(dt, in.maskHeld);
    TickLights(dt, in.lightHeld, in.ventLightLHeld, in.ventLightRHeld);
    TickMusicBox(dt, in.windHeld && m_viewing == 11);
    TickAI(dt);
    TickDanger(dt);

    // scare phase: the attack animation plays, then the night restarts
    if (m_scareT >= 0.0f) {
        m_scareT += dt;
        if (m_scareT >= 0.75f) ResetNightInPlace();
        return;
    }

    // misc per-frame: random image roulette (g632), monitor-down only
    if (m_viewing == 0) m_randomImage = 1 + rand() % 1000;
    // movement static burst decay
    if (m_moveStatic > 0.0f) m_moveStatic -= dt * 2.0f;

    // ---- clock, g483/484/485: AM +1/s, >= 70 -> hour ----
    m_amClock += dt;
    if (m_amClock >= kSecondsPerHour) {
        m_amClock -= kSecondsPerHour;
        m_timeOfNight = (m_timeOfNight == 12) ? 1 : m_timeOfNight + 1;
        if (m_timeOfNight == 6) {
            m_screen = SCR_6AM;
            m_cardT = 0.0f;
            // v2.61: session unlocks (the FNAF2 INI is a later wave) —
            // night+1 for Continue, 6th Night after night 5, Custom after 6
            m_lastNight = (m_night >= 5) ? 5 : m_night + 1;
            if (m_night == 5) m_beat5 = true;
            if (m_night == 6) m_beat6 = true;
        }
    }
}

void FNaF2Game::Tick(f32 dt, const FNaF2Inputs& in) {
    m_time += dt;

    switch (m_screen) {
        case SCR_DISCLAIMER:
            m_cardT += dt;
            if (m_cardT >= 3.5f || in.aPressed) { m_screen = SCR_TITLE; m_cardT = 0.0f; }
            break;

        case SCR_TITLE: {
            // v2.61: FNAF1-style menu navigation — up/down moves the selector
            // through the VISIBLE options (New Game + Continue are ALWAYS
            // there per the dump rows; 6th Night after beating night 5;
            // Custom after night 6), wrapping inside the visible list; A
            // confirms. Continue with no progress still starts night 1
            // (dump groups 42/57: lives left <= 0 -> night number := 1).
            if (in.upPressed)   m_optionSelected = (m_optionSelected + m_optionCount - 1) % m_optionCount;
            if (in.downPressed) m_optionSelected = (m_optionSelected + 1) % m_optionCount;
            if (in.aPressed) {
                switch (m_optionSelected) {
                    case 0: StartNight(1); break;   // New Game
                    case 1: StartNight(m_lastNight < 1 ? 1 :
                                       (m_lastNight > 5 ? 5 : m_lastNight)); break;   // Continue (cap 5, g58)
                    case 2: StartNight(6); break;   // 6th Night
                    case 3: StartNight(7); break;   // Custom Night
                }
            }
            break;
        }

        case SCR_NIGHTSTART:
            m_cardT += dt;
            if (m_cardT >= kCardSeconds) { m_screen = SCR_OFFICE; m_cardT = 0.0f; }
            break;

        case SCR_OFFICE:
            TickOffice(dt, in);
            break;

        case SCR_6AM:
            m_cardT += dt;
            if (m_cardT >= kSixAmSeconds) {
                // to the title KEEPING the session unlocks (a fresh boot
                // still resets them — the FNAF2 INI is a later wave).
                // Dump title groups: the 6th night row rides beatgame, the
                // custom row rides beat6, continue is always there (v2.61);
                // groups 43/44 open the selector on Continue when a night
                // was played (level > 1), otherwise on New Game.
                m_screen = SCR_TITLE;
                m_cardT = 0.0f;
                m_optionCount = 2 + (m_beat5 ? 1 : 0) + (m_beat6 ? 1 : 0);
                m_optionSelected = (m_lastNight > 1) ? 1 : 0;
            }
            break;
    }
}

} // namespace fnaf
