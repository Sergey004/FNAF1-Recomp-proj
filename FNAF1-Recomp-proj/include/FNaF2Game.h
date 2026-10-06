/**
 * FNaF2Game.h: v2.59 — FNAF2 game state, wave 1 (playable core).
 *
 * Implements docs/FNAF2_MECHANICS.md (the office frame digest, 751 groups):
 * 11 AI characters (old/toy Freddy/Bonnie/Chica, old Foxy, Mangle, BB,
 * Puppet, Golden) with the dump's 5 s opportunity rolls, per-night/hour
 * schedule, movement graphs (one node per arming), the danger pipeline
 * (time allowed -> got you stages -> the box), the mask, the flashlight
 * with Foxy's dark-charge, vent lights, the music box with the Puppet
 * emerge, and the jumpscare dispatch with the in-place night restart.
 *
 * The class stays engine-free: input arrives as plain fields, audio runs
 * through callbacks owned by the module.
 */

#ifndef FNAF2_GAME_H
#define FNAF2_GAME_H

#include "Types.h"
#include "Progress.h"    // v2.62: GameProgressF2 (the "freddy2" save struct)

namespace fnaf {

struct FNaF2Inputs {         // translated from GameInput by the module
    bool aPressed;           // confirm (edge)
    bool bPressed;           // back (edge) — v2.62: Escape mirror on the new screens
    bool upPressed;          // menu up (edge)
    bool downPressed;        // menu down (edge)
    bool leftPressed;        // cam cycle - (edge)
    bool rightPressed;       // cam cycle + (edge)
    bool lightHeld;          // flashlight hold (Ctrl on PC / LB on pad)
    bool maskHeld;            // v2.33: Freddy mask hold (RB on pad)
    bool windHeld;            // v2.33: music-box wind hold (X on pad)
    bool ventLightLHeld;     // v2.59: left vent light hold (LT)
    bool ventLightRHeld;     // v2.59: right vent light hold (RT)
    f32  lookDir;            // office pan -1..1
    // v2.62: the 8-bit minigames' held directions (D-pad OR left stick) +
    // the customize mode-cycle edges
    bool mgUp, mgDown, mgLeft, mgRight;
    bool lbPressed, rbPressed;
    f32  lookDirY;           // stick Y (-1 down..+1 up) for the dream pan

    FNaF2Inputs() : aPressed(false), bPressed(false), upPressed(false),
                    downPressed(false), leftPressed(false), rightPressed(false),
                    lightHeld(false), maskHeld(false), windHeld(false),
                    ventLightLHeld(false), ventLightRHeld(false),
                    lookDir(0.0f),
                    mgUp(false), mgDown(false), mgLeft(false), mgRight(false),
                    lbPressed(false), rbPressed(false), lookDirY(0.0f) {}
};

// Audio hooks the module fills (all optional). Volumes are 0..100
// Clickteam-style; the module converts and routes to its channels.
struct FNaF2AudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    // v2.62: the add-on achievement slots (0..9 -> spa ids 11..20); the
    // slot map lives in Achievements.h
    void (*unlockAch)(i32 slot);
    FNaF2AudioHooks() : play(0), stop(0), channelVolume(0), unlockAch(0) {}
};

// ------------------------------------------------------------
// v2.62: the shared 8-bit minigame state (the movement model that
// FNAF3/4/SL minigames reuse). Grid-step motion gated by a frame
// accumulator; sensors are checked against obstacle rectangles.
// ------------------------------------------------------------
struct FNaF2MgState {
    i32 game;              // 1 SAVETHEM hub, 2 take cake, 3 gifts, 4 foxy party
    i32 px, py;            // player anchor (hit box), world px
    i32 facing;            // 0 up, 1 right, 2 down, 3 left
    f32 gateT;             // frame accumulator for the step gate
    i32 addTimer;          // the speed penalty (extra frames per step)
    bool readyMove;        // the dump's one-step-per-window flag
    f32 t;                 // seconds in the minigame

    // ---- SAVETHEM hub (frame 19) ----
    i32 h, v;              // room grid (the dump's horizontal/vertical)
    i32 spawnPick;         // Random(4)+1
    bool newFrame;         // room changed -> re-dress
    f32 letterT;
    i32 letters;           // the SAVETHEM voice step
    f32 rollT;             // the every-30 s Random(3) exit roll
    // one Puppet chaser at a time (dump: per-room phantoms; equivalent effect)
    bool chaserOn;  f32 chaserT;  f32 chX, chY;  i32 chDir;  i32 chSteps;
    // he-was-here shuttle
    f32 heT;  i32 heDir;  f32 heX, heY;
    // Purple Guy (1/101 per right-edge wrap)
    bool manOn;  f32 manT;  f32 manX, manY;
    // Golden Freddy cameo (4/101 per room change, harmless)
    bool gfOn;  f32 gfT;  f32 gfX, gfY;
    bool youCantOn;  f32 youCantX, youCantY;

    // ---- TAKE CAKE (frame 23) ----
    i32 kidSad[6];
    f32 kidT;
    bool murder;           // bear.alterable[0]: past the 20 s mark
    i32 carStage;  f32 carT;  f32 carX;
    f32 manStageT;

    // ---- GIVE GIFTS (frame 24) ----
    bool headGifted[4];
    i32 gifts;             // child 5.alterable[6]
    bool phaseB;           // bear.alterable[15]
    i32 lives;             // child 5.alterable[0]
    f32 attackT;           // the scripted attack anim timer (-1 idle)
    i32 attackAnim;        // v2.62: the attack's anim value (15/20/21 — the
                           // frame lists live on the "attack animation" object)

    // ---- FOXY PARTY (frame 25) ----
    i32 phase;             // 0 intro, 1 walk, 2 party
    i32 cycles;
    f32 phaseT;
    f32 popT;

    void Clear() {
        game = 0; px = 0; py = 0; facing = 2; gateT = 0.0f; addTimer = 0;
        readyMove = false; t = 0.0f;
        h = 0; v = 0; spawnPick = 0; newFrame = false;
        letterT = 0.0f; letters = 0; rollT = 0.0f;
        chaserOn = false; chaserT = 0.0f; chX = 0.0f; chY = 0.0f; chDir = 0; chSteps = 0;
        heT = 0.0f; heDir = 1; heX = 0.0f; heY = 0.0f;
        manOn = false; manT = 0.0f; manX = 0.0f; manY = 0.0f;
        gfOn = false; gfT = 0.0f; gfX = 0.0f; gfY = 0.0f;
        youCantOn = false; youCantX = 0.0f; youCantY = 0.0f;
        for (i32 i = 0; i < 6; ++i) kidSad[i] = 0;
        kidT = 0.0f; murder = false;
        carStage = 0; carT = 0.0f; carX = 0.0f; manStageT = 0.0f;
        for (i32 i = 0; i < 4; ++i) headGifted[i] = false;
        gifts = 0; phaseB = false; lives = 0; attackT = -1.0f; attackAnim = 0;
        phase = 0; cycles = 0; phaseT = 0.0f; popT = 0.0f;
    }
    FNaF2MgState() { Clear(); }
};

class FNaF2Game {
public:
    enum Screen {
        SCR_DISCLAIMER = 4,
        SCR_TITLE = 0,
        // v2.62: the rest of the dump's frame flow
        SCR_AD = 5,          // frame 8: HELP WANTED newspaper (after New Game)
        SCR_NIGHTSTART = 1,  // frame 2: the "Nst Night" card
        SCR_OFFICE = 2,      // frame 3: gameplay
        SCR_STATIC = 6,      // frame 4: post-night static (rare 1/10 app-end)
        SCR_NEXTDAY = 7,     // frame 5: 6 AM clock + the save + router
        SCR_DREAM = 8,       // frame 13: the panning between-night cutscene
        SCR_ERROR = 9,       // frame 14: "it's me" (dream exit, cine == 0)
        SCR_ERROR2 = 10,     // frame 15: "err" (dream exit, cine > 0)
        SCR_END5 = 11,       // frame 9: $100.50 paycheck (night 5)
        SCR_END6 = 12,       // frame 10: pink slip (night 6)
        SCR_END7 = 13,       // frame 11: robots scrapped (custom)
        SCR_CUSTOMIZE = 14,  // frame 12: custom-night AI setup
        SCR_RARE1 = 15,      // frame 16: toy face closeup
        SCR_RARE2 = 16,      // frame 17: withered Foxy closeup
        SCR_RARE3 = 17,      // frame 18: BB balloons room
        SCR_GAMEOVER = 18,   // frame 6: withered Freddy face (1/1000 -> 8bit)
        SCR_EIGHTBIT = 19,   // frame 19: the SAVETHEM overworld hub
        SCR_MGLOAD = 20,     // frame 21: the minigame rotation loader
        SCR_MG1 = 21,        // frame 23: TAKE CAKE TO THE CHILDREN
        SCR_MG2 = 22,        // frame 24: GIVE GIFTS, GIVE LIFE
        SCR_MG3 = 23,        // frame 25: Foxy's party
        SCR_ENDBARS = 24,    // frame 20: black + bars after the chain
        SCR_RAREEXIT = 25    // frame 22: rare post-night app-end loader
    };

    // characters (being attacked by ids: 1..9, 12 — dump G421-449)
    enum Char {
        C_OLD_FREDDY = 0, C_OLD_BONNIE, C_OLD_CHICA, C_OLD_FOXY,
        C_TOY_BONNIE, C_TOY_CHICA, C_TOY_FREDDY, C_MANGLE,
        C_BB, C_PUPPET, C_GOLDEN, C_COUNT
    };
    // rooms: cams 1..12 share the ids, halls/office/box are special
    enum Room {
        R_NONE = 0, R_CAM1 = 1, R_CAM2 = 2, R_CAM3 = 3, R_CAM4 = 4,
        R_CAM5 = 5, R_CAM6 = 6, R_CAM7 = 7, R_CAM8 = 8, R_CAM9 = 9,
        R_CAM10 = 10, R_CAM11 = 11, R_CAM12 = 12,
        R_HALL1 = 20, R_HALL2 = 21, R_OFFICE = 22, R_BOX = 23
    };

    struct CharState {
        i32 room;        // Room
        i32 ai;          // 0..20 (cap per dump)
        i32 alt0;        // 0 idle, 1 armed, 2 = walk one node
        i32 alt1;        // move cooldown frames
        f32 alt3;        // old Foxy: dark-charge seconds
        i32 alt9;        // old Foxy: light-on-him frames
        f32 alt25;       // old Freddy: continuous monitor-up seconds
        i32 alt18;       // Puppet: emerge stage 0..3
        CharState() : room(0), ai(0), alt0(0), alt1(0), alt3(0.0f),
                      alt9(0), alt25(0.0f), alt18(0) {}
    };

    FNaF2Game();

    void ResetToTitle();
    void StartNight(i32 night);
    void Tick(f32 dt, const FNaF2Inputs& in);

    // audio hooks (module wires them once)
    FNaF2AudioHooks audio;

    // ---- state for the module renderer ----
    Screen GetScreen()          const { return m_screen; }
    i32    GetNight()           const { return m_night; }
    i32    GetHour()            const { return m_timeOfNight; }
    i32    GetBatteryLife()     const { return m_batteryLife; }
    i32    GetBatteryMax()      const { return m_batteryMax; }
    bool   IsLit()              const { return m_litQ != 0; }
    i32    GetMaskState()       const { return m_maskState; }
    f32    GetMaskT()           const { return m_maskT; }
    bool   IsMasked()           const { return m_maskState == 2; }
    f32    GetMusicBox()        const { return m_musicGauge; }   // 0..2000
    i32    GetViewing()         const { return m_viewing; }
    i32    GetOptionSelected()  const { return m_optionSelected; }
    i32    GetOptionCount()     const { return m_optionCount; }
    i32    GetLastNight()       const { return m_lastNight; }
    bool   IsBeat5()            const { return m_beat5; }
    bool   IsBeat6()            const { return m_beat6; }
    f32    GetCardTimer()       const { return m_cardT; }
    f32    GetClock()           const { return m_time; }
    i32    GetVentLight(i32 side) const { return side == 0 ? m_ventL : m_ventR; }
    i32    GetAttacker()        const { return m_attacker; }        // 0 none
    i32    GetScareAnim()       const { return m_scareAnim; }       // attack anim value
    f32    GetScareTimer()      const { return m_scareT; }
    i32    GetDangerStage()     const { return m_gotYouStage; }     // 0/1/2
    f32    GetDangerDark()      const { return m_dangerFrames; }    // 0..300
    f32    GetMusicGauge()      const { return m_musicGauge; }      // 0..2000
    i32    GetPuppetStage()     const { return m_chars[C_PUPPET].alt18; }
    bool   IsPuppetWalking()    const { return m_puppetWalking; }
    i32    GetToxic()           const { return m_toxic; }
    i32    GetBlackoutTimer()   const { return m_blackoutTimer; }
    i32    GetRandomImage()     const { return m_randomImage; }
    bool   IsShadowBonnie()     const { return m_shadowT >= 0.0f; }
    f32    GetShadowT()         const { return m_shadowT; }
    bool   IsGoldenArmed()      const { return m_chars[C_GOLDEN].alt0 != 0; }
    i32    GetFreddyOfficeView()const { return m_freddyUnderTable ? 1 : 0; }
    bool   HasBBInOffice()      const { return m_chars[C_BB].room == R_BOX; }
    i32    GetMangleOfficeView()const { return m_mangleView; }
    bool   IsToyBonnieScare()   const { return m_toyBonnieScare; }
    f32    GetMoveStatic()      const { return m_moveStatic; }      // 0..1 burst
    bool   IsMusicBoxDanger()   const { return m_musicDanger; }     // gauge<=400 & out
    f32    GetVentTimer(i32 side) const { return side == 0 ? m_ventLT : m_ventRT; }
    bool   IsPhoneMuted()       const { return m_phoneMuted; }

    // ---- v2.62: the dump flow / minigames ----
    i32    GetNightNext()     const { return m_nightNext; }    // 6 AM's incremented night
    f32    GetScratchT()      const { return m_scratchT; }     // generic screen timer
    i32    GetRareRoll()      const { return m_rareRoll; }     // 1 = the rare branch
    f32    GetDreamPan()      const { return m_dreamPan; }
    f32    GetBlackout()      const { return m_blackout; }
    i32    GetDoingCustom()   const { return m_doingCustom; }
    i32    GetCustomMode()    const { return m_customMode; }
    i32    GetCustomAI(i32 i) const { return m_customAI[i]; }
    bool   GetAllAre20()      const { return m_allAre20; }
    bool   Is1987()           const { return m_combo1987; }
    const FNaF2MgState& Mg()  const { return m_mg; }
    bool   ExitRequested()    const { return m_exitRequested; }  // dump End application

    // the save bridge: the module loads at boot and writes when dirty
    void   ApplyProgressF2(const Progress::GameProgressF2& p);
    void   FillProgressF2(Progress::GameProgressF2& p) const;
    bool   ConsumeSaveDirty() { const bool d = m_saveDirty; m_saveDirty = false; return d; }

    // presence query for the feed renderer: is `ch` currently at `room`?
    bool CharAt(i32 ch, i32 room) const { return m_chars[ch].room == room; }

    // v2.59: dump scene selector — the "Active 16" animation value for the
    // current (viewing, lit?, presence) state; the renderer maps it to the
    // feed/hall image (docs/FNAF2_MECHANICS.md §5). 0 = "no matching view —
    // the feed keeps its previous image" (a real dump behavior on some cams).
    i32   ComputeSceneValue();
    // scene value -> pak image handle ("Active 16" anim table, h=80)
    static i32 SceneValueImg(i32 value);

private:
    void TickOffice(f32 dt, const FNaF2Inputs& in);
    void ResetNightInPlace();     // death restart (dump G450-458)
    void InitNightState();        // frame-start groups
    void TickAI(f32 dt);          // rolls + schedule + movement
    void TickDanger(f32 dt);      // encounter pipeline + box
    void TickMusicBox(f32 dt, bool winding);
    void TickMask(f32 dt, bool wantMask);
    void TickLights(f32 dt, bool lightHeld, bool ventLHeld, bool ventRHeld);
    void DispatchAttack(i32 ch);  // being attacked by -> scare -> restart
    bool AdvanceCharStep(i32 ch); // one movement-graph node (false = conditions unmet)
    void RetreatChar(i32 ch);     // masked/light retreats
    void Sfx(const char* s, bool loop, i32 ch, i32 vol);
    void SfxStop(const char* s);
    void ChVol(i32 ch, i32 vol);

    // v2.62: the new screens + the shared minigame engine
    void TickDream(f32 dt, const FNaF2Inputs& in);
    void TickCustomize(f32 dt, const FNaF2Inputs& in);
    void StartMinigame(i32 which);
    void TickEightBit(f32 dt, const FNaF2Inputs& in);
    void TickMg1(f32 dt, const FNaF2Inputs& in);
    void TickMg2(f32 dt, const FNaF2Inputs& in);
    void TickMg3(f32 dt, const FNaF2Inputs& in);
    void MgStep(const FNaF2Inputs& in, i32 stepPx);   // the shared grid-step
    void MgAttack(i32 animValue);                     // the scripted attack -> load
    bool HubObstacleAt(f32 x, f32 y) const;
    void HubDressRoom();                              // re-dress on room change

    Screen m_screen;
    i32    m_night;
    i32    m_timeOfNight;      // 12,1..5 (6 = win)
    f32    m_amClock;          // 70 s per hour
    i32    m_batteryLife;
    i32    m_batteryMax;
    i32    m_litQ;
    i32    m_viewing;
    i32    m_maskState;        // 0 off,1 lowering,2 on,3 raising
    f32    m_maskT;
    f32    m_musicGauge;       // 0..2000
    f32    m_musicDrainAcc;
    bool   m_musicDanger;      // gauge<=400 && puppet left cam 11
    f32    m_musicWindT;       // windup2 every 0.5 s
    i32    m_inDanger;
    i32    m_attacker;         // being attacked by (0 none)
    i32    m_timeAllowed;
    f32    m_timeLeft;
    i32    m_gotYouStage;      // 0/1/2
    f32    m_dangerFrames;     // darkening overlay accumulator 0..300
    f32    m_scareT;           // attack animation timer
    i32    m_scareAnim;        // attack animation value 12..21
    i32    m_ventL, m_ventR;   // vent light states
    f32    m_ventLT, m_ventRT; // auto-off timers
    i32    m_toxic;            // 0..20 cosmetic
    f32    m_toxicAcc;
    i32    m_blackoutTimer;
    i32    m_randomImage;
    f32    m_shadowT;          // Shadow Bonnie: -1 off, else 4 s to app end
    bool   m_freddyUnderTable; // old Freddy's office pose
    i32    m_mangleView;       // Active 20 value in office
    bool   m_toyBonnieScare;   // Active 19 (toy Bonnie office sprite)
    f32    m_moveStatic;       // feed static burst 0..1
    bool   m_puppetWalking;    // v2.59: the Puppet left the box and walks
    // v2.61: FNAF1-style title navigation (4 options, visibility by session
    // unlocks until the FNAF2 save system lands)
    i32    m_optionCount;      // visible options 1..4
    i32    m_lastNight;        // Continue's night (session memory)
    bool   m_beat5;            // 6th Night visible
    bool   m_beat6;            // Custom Night visible
    bool   m_phoneMuted;
    i32    m_optionSelected;
    f32    m_cardT;
    f32    m_time;
    f32    m_aiRollT;          // 5 s opportunity timer
    f32    m_boxRollT;         // 1 s box-race timer
    f32    m_puppetRollT;
    f32    m_occT;             // office occupied 4.9 s cycle
    bool   m_officeOccupied;
    CharState m_chars[C_COUNT];

    // v2.62: the dump flow state
    i32    m_nightNext;        // the 6 AM screen's incremented night number
    i32    m_cine;             // the dream counter (persisted)
    i32    m_turn;             // the minigame rotation (persisted)
    i32    m_doingCustom;      // 0 normal, 1..10 challenge mode
    i32    m_customMode;       // 1..10
    i32    m_customAI[10];     // the customize sliders
    bool   m_allAre20;
    bool   m_combo1987;
    bool   m_beat7;            // persisted (never read by the dump; kept)
    bool   m_cFlags[10];       // persisted challenge flags
    i32    m_rareRoll;         // per-screen rare roll (1 = rare)
    f32    m_scratchT;         // generic per-screen timer
    f32    m_dreamPan;         // the dream camera 0..(2500-1024)
    f32    m_blackout;         // the dream blackout fade 0..255
    bool   m_saveDirty;        // the module writes freddy2 when this flips
    bool   m_exitRequested;    // SCR_RAREEXIT: the dump's End application
    FNaF2MgState m_mg;
};

} // namespace fnaf

#endif // FNAF2_GAME_H
