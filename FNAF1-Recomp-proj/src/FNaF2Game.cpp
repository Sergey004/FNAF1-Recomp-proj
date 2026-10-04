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
    // v2.62: the dump flow state (the module overrides from freddy2 at boot)
    m_nightNext = 1;
    m_cine = 0;
    m_turn = 0;
    m_doingCustom = 0;
    m_customMode = 0;
    for (i32 i = 0; i < 10; ++i) m_customAI[i] = 0;
    m_allAre20 = false;
    m_combo1987 = false;
    m_beat7 = false;
    for (i32 i = 0; i < 10; ++i) m_cFlags[i] = false;
    m_rareRoll = 0;
    m_scratchT = 0.0f;
    m_dreamPan = 0.0f;
    m_blackout = 0.0f;
    m_saveDirty = false;
    m_mg.Clear();
    for (i32 i = 0; i < C_COUNT; ++i) m_chars[i] = CharState();
}

void FNaF2Game::ApplyProgressF2(const Progress::GameProgressF2& p) {
    // the dump's title boot: level -> night number (Continue), beatgame ->
    // the 6th-night row, beat6 -> star 2 / custom, c1 -> star 3, plus the
    // dream/rotation counters
    m_lastNight = p.level < 1 ? 1 : (p.level > 5 ? 5 : p.level);   // title g58 cap
    m_cine = p.cine;
    m_turn = p.turn;
    m_beat5 = p.beatgame;
    m_beat6 = p.beat6;
    m_beat7 = p.beat7;
    for (i32 i = 0; i < 10; ++i) m_cFlags[i] = p.c[i];
    m_optionCount = 2 + (m_beat5 ? 1 : 0) + (m_beat6 ? 1 : 0);
    if (m_optionSelected >= m_optionCount) m_optionSelected = 0;
}

void FNaF2Game::FillProgressF2(Progress::GameProgressF2& p) const {
    p.level = m_nightNext;      // the 6 AM screen already incremented it
    p.cine = m_cine;
    p.turn = m_turn;
    p.beatgame = m_beat5;
    p.beat6 = m_beat6;
    p.beat7 = m_beat7;
    for (i32 i = 0; i < 10; ++i) p.c[i] = m_cFlags[i];
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
    // v2.62: night 7 = the custom night — the customize sliders ARE the AI
    // (dump frame 12: the ten counters; g622 loads them into the office).
    if (m_night >= 7) {
        static const i32 kMap[10] = {
            C_OLD_FREDDY, C_OLD_BONNIE, C_OLD_CHICA, C_OLD_FOXY, C_BB,
            C_TOY_FREDDY, C_TOY_BONNIE, C_TOY_CHICA, C_MANGLE, C_GOLDEN
        };
        for (i32 i = 0; i < 10; ++i)
            m_chars[kMap[i]].ai = m_customAI[i] < 0 ? 0 : (m_customAI[i] > 20 ? 20 : m_customAI[i]);
    }
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

    // scare phase: the attack animation plays, then the dump's gameover
    // screen (frame 6) — v2.62: the death leaves the office for the face
    // screen (10 s -> title, 1/1000 -> the 8-bit hub), replacing the old
    // in-place restart (the dump's only route INTO frame 6 is the card's
    // gameover edge; the real game shows the face after the scare).
    if (m_scareT >= 0.0f) {
        m_scareT += dt;
        if (m_scareT >= 0.75f) {
            m_screen = SCR_GAMEOVER;
            m_cardT = 0.0f;
            m_scareT = -1.0f;
            m_attacker = 0;
        }
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
            // frame 4 "static" (the stare loop; the save happens on the
            // next-day screen per the dump's own order)
            m_screen = SCR_STATIC;
            m_cardT = 0.0f;
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
                    case 0:
                        // dump title g24: New Game -> level = 1 -> the AD
                        // newspaper -> the card
                        m_night = 1;
                        m_doingCustom = 0;
                        m_screen = SCR_AD;
                        m_cardT = 0.0f;
                        break;
                    case 1: StartNight(m_lastNight < 1 ? 1 :
                                       (m_lastNight > 5 ? 5 : m_lastNight)); break;   // Continue (cap 5, g58)
                    case 2: StartNight(6); break;   // 6th Night
                    case 3:
                        // the custom night goes through the customize screen
                        // (dump frame 12: READY -> night number = 7 -> card)
                        m_screen = SCR_CUSTOMIZE;
                        m_cardT = 0.0f;
                        m_scratchT = 0.0f;
                        break;
                }
            }
            break;
        }

        case SCR_AD:
            // dump frame 8: any key/90 s -> the night card (no sound objects)
            m_cardT += dt;
            if (m_cardT >= 90.0f || in.aPressed || in.upPressed || in.downPressed) {
                StartNight(m_night < 1 ? 1 : m_night);
            }
            break;

        case SCR_NIGHTSTART: {
            // dump frame 2: StopAll + blip3 + the blip flash, the rare roll
            // 1/1000, the card shows ~2.6 s ("card -> gameover" in the dump
            // is the known no-edge-into-frame-3 anomaly — the port bridges
            // card -> office as established in v2.58).
            if (m_cardT == 0.0f) {
                SfxStop("snd_static2"); SfxStop("snd_The_Sand_Temple_Loop_G");
                Sfx("snd_blip3", false, 3, 100);
                m_rareRoll = 1 + (rand() % 1000);
            }
            m_cardT += dt;
            if (m_cardT >= 2.6f) {
                if (m_rareRoll == 1) { m_screen = SCR_RARE3; m_scratchT = 0.0f; }
                else { m_screen = SCR_OFFICE; m_cardT = 0.0f; }
            }
            break;
        }

        case SCR_OFFICE:
            TickOffice(dt, in);
            break;

        case SCR_STATIC: {
            // dump frame 4: stare loop ch1 vol 100, random = Random(10)+1;
            // 5 s -> next day, 1-in-10 -> the rare app-end loader (frame 22)
            if (m_cardT == 0.0f) {
                SfxStop("snd_Xscream3");
                Sfx("snd_stare", true, 1, 100);
                m_rareRoll = 1 + (rand() % 10);
            }
            m_cardT += dt;
            if (m_cardT >= 5.0f) {
                SfxStop("snd_stare");
                if (m_rareRoll == 1) { m_screen = SCR_RAREEXIT; m_scratchT = 0.0f; }
                else {
                    // frame 5 group 1: night number += 1 THEN save
                    m_nightNext = m_night + 1;
                    m_screen = SCR_NEXTDAY;
                    m_cardT = 0.0f;
                    m_scratchT = 0.0f;
                    // the save writes (dump g1: level = night number, cine = 1;
                    // g14 beat6 at 7; g15 beat7 at 8 + all-20; g19 c<mode>)
                    m_saveDirty = true;
                    if (m_doingCustom > 0 && m_doingCustom <= 10)
                        m_cFlags[m_doingCustom - 1] = true;
                    if (m_nightNext == 7) m_beat6 = true;
                    if (m_nightNext == 8 && m_allAre20) m_beat7 = true;
                }
            }
            break;
        }

        case SCR_NEXTDAY: {
            // dump frame 5: the 5->6 digit roll + the crowd cheer (~1 s),
            // then the ~6 s route (the Active 3 alt[1] > 300 family):
            //   2 -> card; 3/4/5 -> dream; 6/7/8 -> the endings (counter := 5)
            m_cardT += dt;
            if (m_cardT >= 1.0f && m_scratchT == 0.0f) {
                m_scratchT = 1.0f;   // one-shot cheer marker
                Sfx("snd_CROWD_SMALL_CHIL_EC049202", false, 2, 100);
            }
            if (m_cardT >= 6.0f) {
                SfxStop("snd_Clocks_Chimes_Cl_02480702");
                if (m_nightNext == 2) StartNight(2);
                else if (m_nightNext >= 3 && m_nightNext <= 5) {
                    m_screen = SCR_DREAM;
                    m_scratchT = 0.0f; m_cardT = 0.0f;
                    m_dreamPan = 738.0f;   // the follow-screen anchor start
                    m_blackout = 0.0f;
                }
                else if (m_nightNext == 6) { m_nightNext = 5; m_screen = SCR_END5; m_cardT = 0.0f; m_beat5 = true; m_saveDirty = true; }
                else if (m_nightNext == 7) { m_nightNext = 5; m_screen = SCR_END6; m_cardT = 0.0f; m_saveDirty = true; }
                else                       { m_nightNext = 5; m_screen = SCR_END7; m_cardT = 0.0f; m_saveDirty = true; }
            }
            break;
        }

        case SCR_DREAM:
            TickDream(dt, in);
            break;

        case SCR_ERROR:
            // dump frame 14: cine counter += 1 on entry, INI cine at 10 s,
            // 30 s -> the night card (the next night already loaded)
            if (m_cardT == 0.0f) { m_cine += 1; m_saveDirty = true; }
            m_cardT += dt;
            if (m_cardT >= 30.0f) StartNight(m_nightNext < 1 ? 1 : m_nightNext);
            break;

        case SCR_ERROR2:
            // dump frame 15: 30 s -> the title
            m_cardT += dt;
            if (m_cardT >= 30.0f) { m_screen = SCR_TITLE; m_cardT = 0.0f; }
            break;

        case SCR_END5:
        case SCR_END6:
        case SCR_END7: {
            // dump frames 9/10/11: musicbox2 loop, beatgame = 1 on entry,
            // 150 s / Escape(/click on the third) -> title
            if (m_cardT == 0.0f) {
                SfxStop("snd_static2"); SfxStop("snd_The_Sand_Temple_Loop_G");
                Sfx("snd_musicbox2", true, 1, 100);
            }
            m_cardT += dt;
            const bool click = (m_screen == SCR_END7) && in.aPressed;
            if (m_cardT >= 150.0f || in.bPressed || click) {
                SfxStop("snd_musicbox2");
                m_screen = SCR_TITLE;
                m_cardT = 0.0f;
                m_optionCount = 2 + (m_beat5 ? 1 : 0) + (m_beat6 ? 1 : 0);
                m_optionSelected = 0;
            }
            break;
        }

        case SCR_CUSTOMIZE:
            TickCustomize(dt, in);
            break;

        case SCR_RARE1:
        case SCR_RARE2:
            // dump frames 16/17: popstatic loop, 100 s -> title
            if (m_cardT == 0.0f) Sfx("snd_popstatic", true, 1, 100);
            m_cardT += dt;
            if (m_cardT >= 100.0f || in.aPressed || in.bPressed) {
                SfxStop("snd_popstatic");
                m_screen = SCR_TITLE; m_cardT = 0.0f;
            }
            break;

        case SCR_RARE3:
            // dump frame 18: the night card's rare branch -> back to the card
            if (m_cardT == 0.0f) Sfx("snd_popstatic", true, 1, 100);
            m_cardT += dt;
            if (m_cardT >= 100.0f || in.aPressed || in.bPressed) {
                SfxStop("snd_popstatic");
                m_cardT = 0.0f;              // re-enter the card (fresh roll)
                m_screen = SCR_NIGHTSTART;
            }
            break;

        case SCR_GAMEOVER:
            // dump frame 6: StopAll, the rare 1/1000 roll, 10 s / any key ->
            // title, rare -> the 8-bit hub
            if (m_cardT == 0.0f) {
                m_rareRoll = 1 + (rand() % 1000);
            }
            m_cardT += dt;
            if (m_cardT >= 10.0f || in.aPressed || in.bPressed) {
                if (m_rareRoll == 1) StartMinigame(1);
                else { m_screen = SCR_TITLE; m_cardT = 0.0f; }
            }
            break;

        case SCR_EIGHTBIT:
            TickEightBit(dt, in);
            break;

        case SCR_MGLOAD:
            // dump frame 21: turn from the save, +1 per second (reset >= 5),
            // 2 s -> route
            if (m_cardT == 0.0f) {
                SfxStop("snd_S2"); SfxStop("snd_A2"); SfxStop("snd_V2"); SfxStop("snd_E2");
                SfxStop("snd_T2"); SfxStop("snd_H2"); SfxStop("snd_I2"); SfxStop("snd_M2");
                Sfx("snd_staticend2", true, 1, 50);
                m_scratchT = 0.0f;
            }
            m_cardT += dt;
            m_scratchT += dt;
            if (m_scratchT >= 1.0f) {
                m_scratchT -= 1.0f;
                m_turn = (m_turn >= 5) ? 0 : m_turn + 1;   // g9/g10
                m_saveDirty = true;
            }
            if (m_cardT >= 2.0f) {
                SfxStop("snd_staticend2");
                switch (m_turn) {
                    case 2: StartMinigame(2); break;   // TAKE CAKE
                    case 3: StartMinigame(3); break;   // GIVE GIFTS
                    case 5: StartMinigame(4); break;   // Foxy's party
                    default: m_screen = SCR_ENDBARS; m_cardT = 0.0f; break;
                }
            }
            break;

        case SCR_MG1: TickMg1(dt, in); break;
        case SCR_MG2: TickMg2(dt, in); break;
        case SCR_MG3: TickMg3(dt, in); break;

        case SCR_ENDBARS:
            // dump frame 20: staticend2, 2 s -> title
            if (m_cardT == 0.0f) Sfx("snd_staticend2", false, 1, 50);
            m_cardT += dt;
            if (m_cardT >= 2.0f) { m_screen = SCR_TITLE; m_cardT = 0.0f; }
            break;

        case SCR_RAREEXIT:
            // dump frame 22: staticend2 + bars, 10 s -> End application.
            // The console mirror of End application = back to the boot
            // selector (the established safe exit).
            if (m_cardT == 0.0f) Sfx("snd_staticend2", true, 1, 50);
            m_cardT += dt;
            if (m_cardT >= 10.0f) m_exitRequested = true;
            break;
    }
}

// ===========================================================================
// v2.62 — the dump's frame flow: the dream, the customize screen, and the
// shared 8-bit minigame engine (frames 19/21/23/24/25). Movement model:
// grid steps gated by a frame accumulator (the dump's `timer >= 10 +
// add_to_timer` family), one step per window, first held direction wins.
// Timers are raw ms per the project convention.
// ===========================================================================

static bool FNaF2MgTouch(f32 ax, f32 ay, f32 bx, f32 by, f32 range) {
    const f32 dx = ax - bx, dy = ay - by;
    return dx * dx + dy * dy < range * range;
}

void FNaF2Game::TickDream(f32 dt, const FNaF2Inputs& in) {
    // frame 13: the 2500x768 panning cutscene; the arrows move the anchor
    // +-4 px per tick (240 px/s), Scary_Space_B + ambience2 loop under it.
    if (m_cardT == 0.0f) {
        Sfx("snd_Scary_Space_B", true, 1, 100);
        Sfx("snd_ambience2", true, 2, 100);
        Sfx("snd_machineturn2", true, 3, 0);
        m_rareRoll = 1 + (rand() % 10);
    }
    m_cardT += dt;
    m_scratchT += dt;

    // the pan (dump groups 3-7: +-4/tick while Left/Right held)
    {
        f32 dir = 0.0f;
        if (in.mgLeft)  dir -= 1.0f;
        if (in.mgRight) dir += 1.0f;
        if (dir == 0.0f) dir = in.lookDir;
        if (dir != 0.0f) {
            m_dreamPan += dir * 240.0f * dt;
            if (m_dreamPan < 0.0f)   m_dreamPan = 0.0f;
            if (m_dreamPan > 1476.0f) m_dreamPan = 1476.0f;
            ChVol(3, 50);
        } else {
            ChVol(3, 0);
        }
    }

    // the static-overlay flicker (g8-13: re-roll every 100 ms; alpha tiers)
    {
        static f32 flickT = 0.0f;
        flickT += dt;
        if (flickT >= 0.1f) { flickT = 0.0f; m_rareRoll = 1 + (rand() % 10); }
    }

    // the blackout fade (g17-18: +1 per 110 ms)
    m_blackout += dt * (100.0f / 110.0f);
    if (m_blackout > 255.0f) m_blackout = 255.0f;

    // the timed voice beats (g19/20: Robot at 30 s, staticend2 + blip at 32)
    if (m_scratchT >= 30.0f && m_scratchT - dt < 30.0f) Sfx("snd_Robot", false, 4, 100);
    if (m_scratchT >= 32.0f && m_scratchT - dt < 32.0f) Sfx("snd_staticend2", false, 4, 100);

    // the exit (g21/22): 33 s -> error (cine == 0, first-run dream) / error 2
    if (m_scratchT >= 33.0f) {
        SfxStop("snd_Scary_Space_B"); SfxStop("snd_ambience2"); SfxStop("snd_machineturn2");
        m_screen = (m_cine == 0) ? SCR_ERROR : SCR_ERROR2;
        m_cardT = 0.0f;
        m_scratchT = 0.0f;
    }
}

// the ten challenge presets (dump frame 12 groups 46-55), verbatim AI sets.
// Slider order: [0]freddy [1]bonie [2]chica [3]foxy [4]BB [5]toyF [6]toyB
// [7]toyC [8]mangle [9]golden.
void FNaF2Game::TickCustomize(f32 dt, const FNaF2Inputs& in) {
    (void)dt;
    if (m_cardT == 0.0f) {
        m_optionSelected = 0;          // the AI-row cursor
        if (m_customMode == 0) m_doingCustom = 0;
    }
    m_cardT += dt;

    // row select (up/down edges)
    if (in.upPressed)   m_optionSelected = (m_optionSelected + 9) % 10;
    if (in.downPressed) m_optionSelected = (m_optionSelected + 1) % 10;

    // +/- on the selected row (the dump's arrows: 0..20, coin on each press;
    // every arrow click also clears doing custom — the sliders are manual)
    bool changed = false;
    if (in.rightPressed && m_customAI[m_optionSelected] < 20) { m_customAI[m_optionSelected] += 1; changed = true; }
    if (in.leftPressed  && m_customAI[m_optionSelected] > 0)  { m_customAI[m_optionSelected] -= 1; changed = true; }
    if (changed) { m_doingCustom = 0; Sfx("snd_coin", false, 3, 80); }

    // the mode cycle (LB/RB; dump groups 41-45 + the preset tables 46-55)
    if (in.lbPressed || in.rbPressed) {
        m_customMode += in.rbPressed ? 1 : -1;
        if (m_customMode < 1) m_customMode = 10;
        if (m_customMode > 10) m_customMode = 1;
        for (i32 i = 0; i < 10; ++i) m_customAI[i] = 0;
        switch (m_customMode) {
            case 1:  // "20/20/20/20"
                m_customAI[0] = 20; m_customAI[1] = 20; m_customAI[2] = 20; m_customAI[3] = 20;
                break;
            case 2:  // "New and Shiny"
                m_customAI[5] = 10; m_customAI[6] = 10; m_customAI[7] = 10;
                m_customAI[8] = 10; m_customAI[4] = 10;
                break;
            case 3:  // "Double Trouble"
                m_customAI[1] = 20; m_customAI[6] = 20; m_customAI[3] = 5;
                break;
            case 4:  // "Night of Misfits"
                m_customAI[4] = 20; m_customAI[8] = 20; m_customAI[9] = 10;
                break;
            case 5:  // "Foxy Foxy"
                m_customAI[3] = 20; m_customAI[8] = 20;
                break;
            case 6:  // "Ladies Night"
                m_customAI[2] = 20; m_customAI[7] = 20; m_customAI[8] = 20;
                break;
            case 7:  // "Freddy's Circus"
                m_customAI[0] = 20; m_customAI[5] = 20; m_customAI[3] = 10;
                m_customAI[4] = 10; m_customAI[9] = 10;
                break;
            case 8:  // "Cupcake Challenge"
                for (i32 i = 0; i < 10; ++i) m_customAI[i] = 5;
                break;
            case 9:  // "Fazbear Fever"
                for (i32 i = 0; i < 10; ++i) m_customAI[i] = 10;
                break;
            case 10: // "Golden Freddy"
                for (i32 i = 0; i < 10; ++i) m_customAI[i] = 20;
                break;
        }
        m_doingCustom = m_customMode;
        Sfx("snd_coin", false, 3, 80);
    }

    // the all-20 flag (dump groups 30-40) + the 1987 combo (g24-28)
    m_allAre20 = true;
    for (i32 i = 0; i < 10; ++i) if (m_customAI[i] < 20) m_allAre20 = false;
    m_combo1987 = (m_customAI[0] == 1 && m_customAI[1] == 9 &&
                   m_customAI[2] == 8 && m_customAI[3] == 7);

    // READY (dump group 2): night number = 7 -> the card
    if (in.aPressed) StartNight(7);
    if (in.bPressed) { m_screen = SCR_TITLE; m_cardT = 0.0f; }   // Escape mirror
}

// ------------------------------------------------------------
// the shared minigame engine
// ------------------------------------------------------------

void FNaF2Game::StartMinigame(i32 which) {
    m_mg.Clear();
    m_mg.game = which;
    switch (which) {
        case 1: {   // frame 19 SAVETHEM: the spawn table (dump groups 77/82-84)
            m_screen = SCR_EIGHTBIT;
            m_mg.spawnPick = 1 + (rand() % 4);
            switch (m_mg.spawnPick) {
                case 1: m_mg.h = 1; m_mg.v = 3; m_mg.px = 638; m_mg.py = 396; break;
                case 2: m_mg.h = 3; m_mg.v = 4; m_mg.px = 388; m_mg.py = 396; break;
                case 3: m_mg.h = 2; m_mg.v = 5; m_mg.px = 512; m_mg.py = 396; break;
                default: m_mg.h = 0; m_mg.v = 0; m_mg.px = 503; m_mg.py = 365; break;
            }
            m_mg.newFrame = true;
            m_mg.facing = 2;
            break;
        }
        case 2:     // frame 23 TAKE CAKE
            m_screen = SCR_MG1;
            m_mg.px = 533; m_mg.py = 459; m_mg.facing = 2;
            for (i32 i = 0; i < 6; ++i) m_mg.kidSad[i] = rand() % 10;
            break;
        case 3:     // frame 24 GIVE GIFTS
            m_screen = SCR_MG2;
            m_mg.px = 519; m_mg.py = 461; m_mg.facing = 2;
            break;
        case 4:     // frame 25 FOXY'S PARTY
            m_screen = SCR_MG3;
            m_mg.px = 530; m_mg.py = 277; m_mg.facing = 1;
            break;
    }
    m_cardT = 0.0f;
    m_scratchT = 0.0f;
}

void FNaF2Game::MgAttack(i32 animValue) {
    // the scripted attack (frames 23/24/25): the attack animation + Xscream2,
    // then -> the load frame. The dump's attack anim cells live in the
    // object's animation bank (not resolvable) — the renderer flashes.
    (void)animValue;
    if (m_mg.attackT >= 0.0f) return;
    m_mg.attackT = 0.0f;
    Sfx("snd_Xscream2", false, 1, 100);
}

void FNaF2Game::MgStep(const FNaF2Inputs& in, i32 stepPx) {
    // the shared gate (frames 23/24/25): timer >= 10 + add_to_timer frames
    // -> one step of stepPx in the first held direction
    m_mg.gateT += 1.0f;
    if (m_mg.gateT >= (f32)(10 + m_mg.addTimer)) {
        m_mg.gateT = 0.0f;
        m_mg.readyMove = true;
    }
    if (!m_mg.readyMove) return;
    m_mg.readyMove = false;      // dump group 8 consumes the flag either way

    i32 dx = 0, dy = 0;
    if (in.mgUp)         { dy = -stepPx; m_mg.facing = 0; }
    else if (in.mgDown)  { dy =  stepPx; m_mg.facing = 2; }
    else if (in.mgLeft)  { dx = -stepPx; m_mg.facing = 3; }
    else if (in.mgRight) { dx =  stepPx; m_mg.facing = 1; }
    else return;

    m_mg.px += dx;
    m_mg.py += dy;
    // the walkable band (the dump gates on the background collision, which
    // needs pixel data — the port clamps to the rooms' walkable rect;
    // labeled approximation)
    const i32 maxX = (m_mg.game == 4) ? 1980 : 960;
    if (m_mg.px < 60)  m_mg.px = 60;
    if (m_mg.px > maxX) m_mg.px = maxX;
    if (m_mg.py < 150) m_mg.py = 150;
    if (m_mg.py > 700) m_mg.py = 700;
}

// hub obstacle test: the room's furniture + the border walls (the dump's
// sensor collisions; rects from the frame 19 layout)
bool FNaF2Game::HubObstacleAt(f32 x, f32 y) const {
    // the screen border walls
    if (x < 30.0f || x > 994.0f || y < 90.0f || y > 700.0f) return true;
    const i32 h = m_mg.h, v = m_mg.v;
    // party tables (rooms (3,1),(4,1),(4,3),(3,3),(2,4) x2 — dump groups 41-45)
    if ((h == 3 && v == 1) || (h == 4 && v == 1) || (h == 4 && v == 3) ||
        (h == 3 && v == 3) || (h == 2 && v == 4)) {
        static const f32 kTable[4][4] = { {294,190,90,60}, {737,190,90,60}, {737,410,90,60}, {294,410,90,60} };
        for (i32 i = 0; i < 4; ++i)
            if (x >= kTable[i][0] - 20.0f && x <= kTable[i][0] + kTable[i][2] + 20.0f &&
                y >= kTable[i][1] - 20.0f && y <= kTable[i][1] + kTable[i][3] + 20.0f) return true;
    }
    if (h == 5 && v == 2) {   // the table fan
        if (x > 700.0f && x < 800.0f && y > 400.0f && y < 500.0f) return true;
    }
    if (h == 1 && v == 4) {   // the stage
        if (x > 440.0f && x < 700.0f && y > 250.0f && y < 380.0f) return true;
    }
    if (h == 2 && v == 1) {   // the dead bots
        if (x > 150.0f && x < 320.0f && y > 470.0f && y < 580.0f) return true;
    }
    return false;
}

void FNaF2Game::HubDressRoom() {
    // dump group 40 + the per-room dressing groups (fire-once): roll the
    // Golden Freddy cameo and place the room's NPCs
    m_mg.newFrame = false;
    m_mg.gfOn = false;
    m_mg.manOn = false;
    m_mg.chaserOn = false;
    m_mg.youCantOn = false;
    // the furniture roll (4/101 per room change)
    if ((rand() % 100) + 1 <= 4) {
        m_mg.gfOn = true;
        m_mg.gfX = 500.0f + (f32)(rand() % 200);
        m_mg.gfY = 300.0f + (f32)(rand() % 150);
    }
    const i32 h = m_mg.h, v = m_mg.v;
    // he-was-here in (4,2) (group 28)
    if (h == 4 && v == 2) { m_mg.heX = 447.0f; m_mg.heY = 514.0f; m_mg.heDir = 1; m_mg.heT = 0.0f; }
    // you cant (4,1),(1,4),(4,4); facing-left variants (3,5),(2,3)
    if ((h == 4 && v == 1) || (h == 1 && v == 4) || (h == 4 && v == 4)) {
        m_mg.youCantOn = true; m_mg.youCantX = 194.0f; m_mg.youCantY = 599.0f;
    } else if ((h == 3 && v == 5) || (h == 2 && v == 3)) {
        m_mg.youCantOn = true; m_mg.youCantX = 620.0f; m_mg.youCantY = 460.0f;
    }
    // the Puppet chasers: right (2,2),(2,3),(3,4); down (2,4); up ((3,2) if
    // pupp>=1 / (4,2) if pupp>=2 — collapsed to the room roll)
    if ((h == 2 && v == 2) || (h == 2 && v == 3) || (h == 3 && v == 4)) {
        m_mg.chaserOn = true; m_mg.chDir = 1; m_mg.chX = 231.0f; m_mg.chY = 377.0f;
    } else if (h == 2 && v == 4) {
        m_mg.chaserOn = true; m_mg.chDir = 2; m_mg.chX = 512.0f; m_mg.chY = 200.0f;
    } else if ((h == 3 && v == 2) || (h == 4 && v == 2)) {
        m_mg.chaserOn = true; m_mg.chDir = 0; m_mg.chX = 512.0f; m_mg.chY = 600.0f;
    }
    if (m_mg.chaserOn) { m_mg.chSteps = 0; m_mg.chaserT = 0.0f; }
}

void FNaF2Game::TickEightBit(f32 dt, const FNaF2Inputs& in) {
    // frame 19 SAVETHEM: 20 px steps on a 200 ms gate, sensor collisions,
    // the room grid with wrap, the letter voice, the NPCs
    if (m_cardT == 0.0f) {
        Sfx("snd_ComputerInteriorLong_EVL02_14", true, 2, 50);
        m_mg.letters = 0; m_mg.letterT = 0.0f; m_mg.rollT = 0.0f;
    }
    m_cardT += dt;
    m_mg.t += dt;

    // the SAVETHEM letter voice (groups 1-10: one letter every 3 s, loop)
    m_mg.letterT += dt;
    if (m_mg.letterT >= 3.0f) {
        m_mg.letterT = 0.0f;
        static const char* const kLetters[8] = {
            "snd_S2", "snd_A2", "snd_V2", "snd_E2", "snd_T2", "snd_H2", "snd_E2", "snd_M2"
        };
        Sfx(kLetters[m_mg.letters & 7], false, 5, 100);
        m_mg.letters = (m_mg.letters + 1) & 7;
    }

    if (m_mg.newFrame) HubDressRoom();

    // the movement: one 20 px step per 200 ms gate (groups 13-17)
    m_mg.gateT += dt;
    if (m_mg.gateT >= 0.2f) {
        m_mg.gateT = 0.0f;
        i32 dx = 0, dy = 0;
        if (in.mgUp)         { dy = -20; m_mg.facing = 0; }
        else if (in.mgDown)  { dy =  20; m_mg.facing = 2; }
        else if (in.mgLeft)  { dx = -20; m_mg.facing = 3; }
        else if (in.mgRight) { dx =  20; m_mg.facing = 1; }
        if (dx != 0 || dy != 0) {
            // the sensor sits one step ahead (the dump's move-* actives);
            // blocked by furniture or the room borders
            const f32 sx = (f32)m_mg.px + dx * 1.5f;
            const f32 sy = (f32)m_mg.py + dy * 1.5f;
            if (!HubObstacleAt(sx, sy)) {
                m_mg.px += dx; m_mg.py += dy;
            }
            // the edge wraps (groups 32-35): the room grid + reposition
            if (m_mg.px > 1000) { m_mg.px = 60;  m_mg.h = (m_mg.h % 5) + 1; m_mg.newFrame = true;
                                  if ((rand() % 101) == 1) { m_mg.manOn = true; m_mg.manX = 283.0f; m_mg.manY = 375.0f; m_mg.manT = 0.0f; } }
            if (m_mg.px < 20)   { m_mg.px = 960; m_mg.h = (m_mg.h == 1) ? 5 : m_mg.h - 1; m_mg.newFrame = true; }
            if (m_mg.py > 740)  { m_mg.py = 110; m_mg.v = (m_mg.v % 5) + 1; m_mg.newFrame = true; }
            if (m_mg.py < 80)   { m_mg.py = 690; m_mg.v = (m_mg.v == 1) ? 5 : m_mg.v - 1; m_mg.newFrame = true; }
        }
    }

    // he-was-here shuttles +-10 px every 0.5 s, reversing after 15 steps
    if (m_mg.heX != 0.0f) {
        m_mg.heT += dt;
        if (m_mg.heT >= 0.5f) {
            m_mg.heT = 0.0f;
            m_mg.heX += 10.0f * m_mg.heDir;
            if (m_mg.heX > 560.0f || m_mg.heX < 340.0f) m_mg.heDir = -m_mg.heDir;
        }
    }
    // the Puppet chaser: 50 px per 0.5 s, 10 steps, then gone (groups 90-95)
    if (m_mg.chaserOn) {
        m_mg.chaserT += dt;
        if (m_mg.chaserT >= 0.5f) {
            m_mg.chaserT = 0.0f;
            switch (m_mg.chDir) {
                case 0: m_mg.chY -= 50.0f; break;
                case 1: m_mg.chX += 50.0f; break;
                case 2: m_mg.chY += 50.0f; break;
                case 3: m_mg.chX -= 50.0f; break;
            }
            m_mg.chSteps += 1;
            if (m_mg.chSteps > 10) m_mg.chaserOn = false;
        }
    }
    // Purple Guy walks left at 200 px/s (group 106)
    if (m_mg.manOn) {
        m_mg.manT += dt;
        m_mg.manX -= 200.0f * dt;
        if (m_mg.manX < -60.0f) m_mg.manOn = false;
    }

    // the player collisions (the player box ~31x31 at px,py)
    if (m_mg.youCantOn &&
        FNaF2MgTouch((f32)m_mg.px, (f32)m_mg.py, m_mg.youCantX, m_mg.youCantY, 46.0f)) {
        // group 103: -> the load frame (the minigame rotation)
        SfxStop("snd_ComputerInteriorLong_EVL02_14");
        m_screen = SCR_MGLOAD; m_cardT = 0.0f; return;
    }
    if (m_mg.manOn &&
        FNaF2MgTouch((f32)m_mg.px, (f32)m_mg.py, m_mg.manX, m_mg.manY, 50.0f)) {
        // group 107: -> rare1 (the withered Foxy screen)
        SfxStop("snd_ComputerInteriorLong_EVL02_14");
        m_screen = SCR_RARE2; m_cardT = 0.0f; return;
    }
    if (m_mg.gfOn) {
        m_mg.gfT += dt;               // touching gf only plays his anim
        if (m_mg.gfT > 3.0f) m_mg.gfOn = false;
    }

    // the every-30 s Random(3) exit roll (group 108)
    m_mg.rollT += dt;
    if (m_mg.rollT >= 30.0f) {
        m_mg.rollT = 0.0f;
        if ((rand() % 3) == 1) {
            SfxStop("snd_ComputerInteriorLong_EVL02_14");
            m_screen = SCR_MGLOAD; m_cardT = 0.0f;
        }
    }
}

void FNaF2Game::TickMg1(f32 dt, const FNaF2Inputs& in) {
    // frame 23 TAKE CAKE TO THE CHILDREN
    if (m_cardT == 0.0f) {
        Sfx("snd_staticend2", true, 2, 20);
        m_mg.letters = 0; m_mg.letterT = 0.0f;
    }
    m_cardT += dt;
    m_mg.t += dt;

    // the letter voice: "SAVE HIM" (groups 30-40)
    m_mg.letterT += dt;
    if (m_mg.letterT >= 2.0f) {
        m_mg.letterT = 0.0f;
        static const char* const kLetters[7] = {
            "snd_S2", "snd_A2", "snd_V2", "snd_E2", "snd_H2", "snd_I2", "snd_M2"
        };
        Sfx(kLetters[m_mg.letters % 7], false, 5, 100);
        m_mg.letters += 1;
    }

    MgStep(in, 15);

    // the kids' sadness (groups 11-16): +1/s; the bear feeding resets it
    m_mg.kidT += dt;
    if (m_mg.kidT >= 1.0f) {
        m_mg.kidT = 0.0f;
        for (i32 i = 0; i < 6; ++i) m_mg.kidSad[i] += 1;
    }
    if (!m_mg.murder) {
        static const f32 kKidX[6] = { 290,290,290,776,770,766 };
        static const f32 kKidY[6] = { 280,416,548,278,410,538 };
        for (i32 i = 0; i < 6; ++i) {
            if (m_mg.kidSad[i] >= 10 &&
                (f32)m_mg.px > kKidX[i] - 60.0f && (f32)m_mg.px < kKidX[i] + 60.0f &&
                (f32)m_mg.py > kKidY[i] - 70.0f && (f32)m_mg.py < kKidY[i] + 70.0f) {
                m_mg.kidSad[i] = 0;
                Sfx("snd_cake2", false, 3, 100);
            }
        }
    }

    // the murder script (groups 17-21): at 20 s the car arrives, then the
    // man; the player's step gate rots (+10 frames per second, then +25)
    if (!m_mg.murder && m_mg.t >= 20.0f) { m_mg.murder = true; m_mg.carStage = 1; m_mg.carT = 0.0f; m_mg.carX = 1220.0f; }
    if (m_mg.murder && m_mg.carStage == 1) {
        m_mg.carT += dt;
        if (m_mg.carT >= 0.5f) {
            m_mg.carT = 0.0f;
            m_mg.carX -= 50.0f;
            m_mg.manStageT += 1.0f;
            if (m_mg.manStageT >= 15.0f) { m_mg.carStage = 2; m_mg.manStageT = 0.0f; }
        }
    } else if (m_mg.carStage == 2) {
        // the man on screen: +10 to the step gate every second
        m_mg.manStageT += dt;
        if (m_mg.manStageT >= 1.0f) {
            m_mg.manStageT = 0.0f;
            m_mg.addTimer += 10;
            if (m_mg.addTimer > 170) {   // ~17 s of the man
                m_mg.carStage = 3; m_mg.carT = 0.0f;
            }
        }
    } else if (m_mg.carStage == 3) {
        // the car drives off: +25 per 0.5 s pass, then the attack roll
        m_mg.carT += dt;
        if (m_mg.carT >= 0.5f) {
            m_mg.carT = 0.0f;
            m_mg.carX -= 50.0f;
            m_mg.addTimer += 25;
        }
        if (m_mg.attackT < 0.0f && (rand() % 60) == 1) MgAttack(20);   // ~1/s roll
    }

    // the attack -> the load frame (group 43)
    if (m_mg.attackT >= 0.0f) {
        m_mg.attackT += dt;
        if (m_mg.attackT >= 0.75f) {
            SfxStop("snd_staticend2");
            m_screen = SCR_MGLOAD; m_cardT = 0.0f;
        }
    }
}

void FNaF2Game::TickMg2(f32 dt, const FNaF2Inputs& in) {
    // frame 24 GIVE GIFTS, GIVE LIFE (the Puppet is the player)
    if (m_cardT == 0.0f) {
        Sfx("snd_staticend2", true, 2, 20);
        m_mg.letters = 0; m_mg.letterT = 0.0f;
    }
    m_cardT += dt;

    // the letter voice: "HELP THEM" (H,E,L,P,T,H,E,M — L/P reuse E2/I2 slots
    // in the pak; the dump's #59/#61 are the two extra letter samples)
    m_mg.letterT += dt;
    if (m_mg.letterT >= 2.0f) {
        m_mg.letterT = 0.0f;
        static const char* const kLetters[8] = {
            "snd_H2", "snd_E2", "snd_I2", "snd_M2", "snd_T2", "snd_H2", "snd_E2", "snd_M2"
        };
        Sfx(kLetters[m_mg.letters & 7], false, 5, 100);
        m_mg.letters += 1;
    }

    MgStep(in, 15);

    static const f32 kHeadX[4] = { 235, 200, 814, 818 };
    static const f32 kHeadY[4] = { 204, 594, 190, 580 };
    if (m_mg.attackT < 0.0f) {
        if (!m_mg.phaseB) {
            // phase A: give the four gifts (groups 47-50)
            for (i32 i = 0; i < 4; ++i) {
                if (!m_mg.headGifted[i] &&
                    (f32)m_mg.px > kHeadX[i] - 55.0f && (f32)m_mg.px < kHeadX[i] + 55.0f &&
                    (f32)m_mg.py > kHeadY[i] - 55.0f && (f32)m_mg.py < kHeadY[i] + 55.0f) {
                    m_mg.headGifted[i] = true;
                    m_mg.gifts += 1;
                    Sfx("snd_cake2", false, 3, 100);
                }
            }
            // all four -> touch the center to switch to phase B (group 51)
            if (m_mg.gifts >= 4 &&
                (f32)m_mg.px > 480.0f && (f32)m_mg.px < 580.0f &&
                (f32)m_mg.py > 350.0f && (f32)m_mg.py < 430.0f) {
                m_mg.phaseB = true;
            }
        } else {
            // phase B: give the lives (groups 36-39) — the heads hide
            for (i32 i = 0; i < 4; ++i) {
                if (m_mg.headGifted[i] && m_mg.kidSad[i] == 0 &&
                    (f32)m_mg.px > kHeadX[i] - 55.0f && (f32)m_mg.px < kHeadX[i] + 55.0f &&
                    (f32)m_mg.py > kHeadY[i] - 55.0f && (f32)m_mg.py < kHeadY[i] + 55.0f) {
                    m_mg.kidSad[i] = 1;      // the phase-B done flag
                    m_mg.addTimer += 5;
                    m_mg.lives += 1;
                    Sfx("snd_effect3", false, 3, 100);
                }
            }
            // the volume ladder + the end (groups 41-45)
            if (m_mg.lives == 1)      ChVol(1, 40);
            else if (m_mg.lives == 2) ChVol(1, 30);
            else if (m_mg.lives == 3) ChVol(1, 15);
            else if (m_mg.lives >= 4) {
                ChVol(1, 15);
                if (m_mg.attackT < 0.0f) { m_mg.addTimer += 50; MgAttack(21); }
            }
        }
    }

    // the attack -> the load frame (group 34)
    if (m_mg.attackT >= 0.0f) {
        m_mg.attackT += dt;
        if (m_mg.attackT >= 0.75f) {
            SfxStop("snd_staticend2");
            m_screen = SCR_MGLOAD; m_cardT = 0.0f;
        }
    }
}

void FNaF2Game::TickMg3(f32 dt, const FNaF2Inputs& in) {
    // frame 25 FOXY'S PARTY: two camera pages, the phase/cycle machine
    if (m_cardT == 0.0f) {
        Sfx("snd_staticend2", true, 2, 20);
    }
    m_cardT += dt;

    // movement only in phase 1 (the dump's extra phase gate on groups 4-7);
    // the speed rot on the right page during the second visit (groups 59/60)
    if (m_mg.phase == 1 && m_mg.cycles >= 2)
        m_mg.addTimer = (m_mg.px >= 1024) ? 15 : 0;
    if (m_mg.phase == 1) MgStep(in, 25);

    switch (m_mg.phase) {
        case 0:   // the intro pin (groups 46-48): 5 s, then the arrow shows
            m_mg.phaseT += dt;
            if (m_mg.phaseT >= 5.0f) { m_mg.phase = 1; m_mg.phaseT = 0.0f; }
            break;
        case 1:   // walking right; the party trigger (group 49)
            if (m_mg.px >= 1290 && m_mg.px <= 1480 && m_mg.cycles < 2) {
                m_mg.phase = 2; m_mg.phaseT = 0.0f;
            }
            break;
        case 2:   // the party: fireworks for ~5 s, then back (groups 54-56)
            m_mg.phaseT += dt;
            m_mg.popT += dt;
            if (m_mg.popT >= 0.5f) {
                m_mg.popT = 0.0f;
                Sfx("snd_pop", false, 3, 100);
            }
            if (m_mg.phaseT >= 5.0f && m_mg.cycles < 2) {
                m_mg.phase = 0; m_mg.phaseT = 0.0f;
                m_mg.cycles += 1;
                m_mg.px = 400;   // reset toward the left page
            }
            break;
    }
    // the second visit brings the Purple Guy and the silent kids (group 57)

    // the fatal second party (groups 58/25/26)
    if (m_mg.cycles >= 2 && m_mg.px >= 1440 && m_mg.px <= 1560 &&
        m_mg.py >= 330 && m_mg.py <= 460 && m_mg.attackT < 0.0f) {
        MgAttack(15);
    }

    // the attack -> the load frame (group 26)
    if (m_mg.attackT >= 0.0f) {
        m_mg.attackT += dt;
        if (m_mg.attackT >= 0.75f) {
            SfxStop("snd_staticend2");
            m_screen = SCR_MGLOAD; m_cardT = 0.0f;
        }
    }
}

} // namespace fnaf
