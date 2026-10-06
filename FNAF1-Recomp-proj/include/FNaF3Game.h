/**
 * FNaF3Game.h: v2.61 — FNAF3 game state, wave 1 (playable night loop).
 *
 * Implements the office-frame digest (Events/frame_3_Frame 1_events.txt,
 * 773 groups): the hour clock (raw-ms timers — 40 s/hour night 1, 60 s
 * nights 2+), the Springtrap room graph (10 rooms + 5 vents + the four
 * attack stages that only advance while a screen is open), the audio-lure
 * adjacency table, the phantoms (BB/Foxy/Mangle/Puppet/Chica + Golden and
 * Shadow Freddy), the maintenance panel with the audio/camera/vent error
 * meters, vent sealing, the vent-error blackout, and the IN-PLACE night
 * restart on death (Group 606 reloads frame 3 — this build has no
 * game-over screen from the office).
 *
 * The class stays engine-free: input arrives as plain fields, audio runs
 * through callbacks owned by the module (the FNaF2Game contract).
 */

#ifndef FNAF3_GAME_H
#define FNAF3_GAME_H

#include "Types.h"
#include "Progress.h"    // v2.62: GameProgressF3 (the "freddy3" save struct)

namespace fnaf {

struct FNaF3Inputs {         // translated from GameInput by the module
    bool aPressed;           // confirm (edge)
    bool bPressed;           // back / close (edge)
    bool xPressed;           // action: lure / seal (edge)
    bool yPressed;           // maintenance panel (edge)
    bool lbPressed;          // monitor flip (edge)
    bool rbPressed;          // room map <-> vent map (edge)
    bool upPressed;
    bool downPressed;
    bool leftPressed;
    bool rightPressed;
    f32  lookDir;            // office pan -1..1

    FNaF3Inputs() : aPressed(false), bPressed(false), xPressed(false),
                    yPressed(false), lbPressed(false), rbPressed(false),
                    upPressed(false), downPressed(false),
                    leftPressed(false), rightPressed(false), lookDir(0.0f) {}
};

// Audio hooks the module fills (all optional). Volumes are 0..100
// Clickteam-style; the module converts and routes to its channels.
struct FNaF3AudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    FNaF3AudioHooks() : play(0), stop(0), channelVolume(0) {}
};

class FNaF3Game {
public:
    enum Screen {
        SCR_DISCLAIMER = -1,
        SCR_TITLE      = 0,   // frame 1 "title"
        SCR_NIGHTSTART = 1,   // frame 2 "what day" card
        SCR_OFFICE     = 2,   // frame 3 "Frame 1"
        SCR_STATIC6    = 3,   // frame 4 "static" (6 AM transition)
        SCR_NEXTDAY    = 4,   // frame 5 "next day" (payday card)
        // v2.62: the rest of the dump's flow
        SCR_AD         = 5,   // frame 8: the "COMING SOON" newspaper (after New Game)
        SCR_RARE2      = 6,   // frame 13: the post-gameover rare screen (garble, 5 s)
        SCR_ENDCHOOSER = 7,   // frame 17: cutscene != 5 -> what day; == 5 -> the ends
        SCR_ENDBAD     = 8,   // frame 9: bad end (mb2 + beatgame)
        SCR_ENDGOOD    = 9,   // frame 10: good end (the "ending" song + beatgame)
        SCR_END2       = 10,  // frame 11: the end 2 (mb2 + beatgame; night 6)
        // v2.63: the rest of the dump's flow (jumps = storyboard slots)
        SCR_WAIT       = 11,  // frame 7 "wait": 100 ms black -> office
        SCR_GAMEOVER   = 12,  // frame 6: 5 s / A -> title; 1/1000 -> rare2
        SCR_RARE1      = 13,  // frame 12: the boot rare screen (1/1000 at the disclaimer)
        SCR_RARE3      = 14,  // frame 14: the night-card rare screen (1/1000) -> what day
        SCR_LOAD       = 15,  // frame 18: the between-night glitch (5 s) -> cutscene
        SCR_CUTSCENE   = 16,  // frame 16: the retro decommission scenes (5x5 rooms)
        SCR_MG         = 17,  // frames 19-24: the six Atari minigames (m_mgGame)
        SCR_EXTRAS     = 18   // frame 25: the extras menu
    };

    // The six minigames (frames 19-24). Entry points are the office secrets:
    // the arcade cabinet (night 2) -> MG_MANGLE, the BB toy on CAM 08 ->
    // MG_BB, the cupcake run -> MG_TOYCHICA, the 5-2-4-8 keypad -> MG_GFREDDY,
    // the dark room (night 5) -> MG_RWQ, the puppet toy on CAM 03 -> MG_MARION.
    // The extras menu replays all but the Marion.
    enum Mg3Game {
        MG_NONE = 0,
        MG_BB = 1, MG_MANGLE = 2, MG_TOYCHICA = 3,
        MG_GFREDDY = 4, MG_RWQ = 5, MG_MARION = 6
    };

    // Springtrap's places: the dump moves one invisible tracker ("dhfgh")
    // between room markers 01-10, the five vent-cam markers, the four
    // attack stages and the two GOT YOU markers (night 1 parks him off-map).
    enum Room3 {
        R3_OFFMAP = 0,
        R3_01 = 1, R3_02 = 2, R3_03 = 3, R3_04 = 4, R3_05 = 5,
        R3_06 = 6, R3_07 = 7, R3_08 = 8, R3_09 = 9, R3_10 = 10,
        R3_ST1 = 21, R3_ST2 = 22, R3_ST3 = 23, R3_ST4 = 24,
        R3_GY = 25, R3_GY2 = 26,
        R3_V11 = 31, R3_V12 = 32, R3_V13 = 33, R3_V14 = 34, R3_V15 = 35
    };

    // ---- v2.63: the six Atari minigames (frames 19-24). One platformer
    // skeleton drives all six: 100 ms ticks (fall 10 px, walk 15/20 px),
    // the 60 ms rise tick (20 px, jump counter 7/9/10), "feel" sensors vs
    // the obstacle rects, the balloon bounce floors (after BB), the
    // viewport that scrolls in 1024x768 pages inside a 3072x2304 world and
    // the 200-frame win counter. Dump deviations: pixel-perfect backdrop
    // collision becomes obstacle rects, and the exact per-room platform
    // shapes are coarse. ----
    struct Mg3State {
        i32  game;          // Mg3Game
        bool fromExtras;    // the extras replay flag ("extras game?")
        f32  px, py;        // the hit box (46x46, center-based like the dump)
        i32  facing;        // 0 right, 1 left
        i32  jumpCnt;       // the rise counter (0 = grounded)
        bool jumpHold;      // W held (the cut releases the jump)
        f32  fallT, walkT, riseT;   // the 100/100/60 ms tick accumulators
        f32  camX, camY;    // viewport top-left in world px (1024x768 pages)
        bool scrolled;      // "secret": the camera has moved (exit gating)
        f32  winT;          // the alt2 win counter (frames at 60 fps -> s)
        bool won;           // "win" == 1
        i32  collects;      // "Counter" (pickups taken)
        f32  cakeT;         // the big-cake feed counter (k-feeding scenes)
        bool feeding;       // the freeze cutscene (Toy Chica / big cakes)
        // per-game extras
        bool kidFollow[4];  // Mangle: the four kids collected
        i32  fed;           // Toy Chica: guests fed (exit at 4)
        i32  view;          // RWQ: the S-teleport view 1..5
        i32  party;         // Marion: finale stage (0 none, 1..4)
        f32  partyT;        // Marion: the float timeline
        i32  taken[12];    // pickup state (1 = taken), index per game table
        void Clear() {
            game = 0; fromExtras = false; px = 0; py = 0; facing = 0;
            jumpCnt = 0; jumpHold = false;
            fallT = 0.0f; walkT = 0.0f; riseT = 0.0f;
            camX = 0.0f; camY = 0.0f; scrolled = false;
            winT = 0.0f; won = false; collects = 0; cakeT = 0.0f; feeding = false;
            for (i32 i = 0; i < 4; ++i) kidFollow[i] = false;
            fed = 0; view = 1; party = 0; partyT = 0.0f;
            for (i32 i = 0; i < 12; ++i) taken[i] = 0;
        }
        Mg3State() { Clear(); }
    };

    // ---- v2.63: the cutscenes frame (16): you walk the retro pizzeria
    // (5x5 room grid, one 1024x768 screen per room) toward the room where
    // the Purple Guy dismantles the animatronic you play (scene = the cine
    // counter 1..4); scene 5 opens the back room where he hides in the
    // Springtrap suit and dies. ----
    struct CutsceneState {
        i32  scene;         // 1..5 (the "cutscene" counter)
        i32  v, h;          // room grid 1..5 (vertical, horizontal)
        f32  px, py;        // hit box on the 1024x768 screen
        i32  facing;        // 0 right, 1 left, 2 up, 3 down
        f32  moveT;         // the 250 ms step gate
        f32  t;             // scene clock
        bool errShown;      // the (2,5) blocked-up ERR fired (arms the kill)
        i32  manStage;      // 0 none, 1 hunting, 2 kill anim
        f32  manX, manY;    // purple guy pos
        f32  manT;          // his 100 ms homing tick
        f32  killT;         // the death fade (frames)
        f32  errT;          // the ERR display timer
        f32  shadowX[3];    // shadow right/up/down drift offsets (-1000 off)
        bool shadowOn[3];
        f32  hintT;         // the controls hint / follow-me timer
        f32  ratT;          // the rat scurry timer
        f32  ratX;
        f32  rainT;         // rain spawn timer
        i32  rainN;         // live drops
        f32  dropY[12];
        f32  dropX[12];
        // scene 5 finale (room 1,5)
        bool finaleOn;      // the suit sequence runs
        i32  suitStage;     // 0..6
        f32  suitT;
        f32  manRunX;
        i32  trips;         // "count trips"
        f32  patrolT;
        f32  patrolX;
        i32  patrolState;
        void Clear() {
            scene = 1; v = 2; h = 3; px = 500.0f; py = 300.0f; facing = 0;
            moveT = 0.0f; t = 0.0f; errShown = false;
            manStage = 0; manX = 0.0f; manY = 0.0f; manT = 0.0f;
            killT = 0.0f; errT = 0.0f;
            for (i32 i = 0; i < 3; ++i) { shadowX[i] = 0.0f; shadowOn[i] = false; }
            hintT = 5.0f; ratT = 0.0f; ratX = -60.0f; rainT = 0.0f; rainN = 0;
            for (i32 i = 0; i < 12; ++i) { dropY[i] = 0.0f; dropX[i] = 0.0f; }
            finaleOn = false; suitStage = 0; suitT = 0.0f; manRunX = 0.0f;
            trips = 0; patrolT = 0.0f; patrolX = 0.0f; patrolState = 0;
        }
        CutsceneState() { Clear(); }
    };

    // ---- v2.63: the extras menu (frame 25): five rows, the minigame and
    // jumpscare viewers gated by goodend/beat6, the four cheats. ----
    struct ExtrasState {
        i32  row;           // 0 animatronics, 1 minigames, 2 jumpscares, 3 cheats, 4 exit
        i32  viewer;        // the animatronics viewer index 0..6
        i32  mgPick;        // the minigame replay pick 0..4
        i32  jsPick;        // the jumpscare pick 0..5
        f32  jsT;           // the playing jumpscare timer
        f32  cooldown;      // the cheat toggle cooldown
        void Clear() {
            row = 0; viewer = 0; mgPick = 0; jsPick = 0; jsT = 0.0f; cooldown = 0.0f;
        }
        ExtrasState() { Clear(); }
    };

    FNaF3Game();

    void ResetToTitle();
    void StartNight(i32 night);
    void Tick(f32 dt, const FNaF3Inputs& in);

    // audio hooks (module wires them once)
    FNaF3AudioHooks audio;

    // ---- state for the module renderer ----
    Screen GetScreen()        const { return m_screen; }
    i32    GetNight()         const { return m_night; }
    i32    GetTimeOfNight()   const { return m_timeOfNight; }   // 0=12AM..5, 6 win
    f32    GetCardTimer()     const { return m_cardT; }
    f32    GetClock()         const { return m_time; }
    f32    GetPan()           const { return m_pan; }           // office scroll 0..1488
    bool   IsAggressive()     const { return m_aggressive; }

    // Springtrap
    i32    GetSpringtrapRoom()const { return m_stRoom; }
    i32    GetPlayEffect()    const { return m_playEffect; }    // last vent sound 1..4
    f32    GetRunPastT()      const { return m_runPastT; }      // hall dash overlay
    bool   IsBigScare()       const { return m_bigScare; }      // him in the monitor
    i32    GetGotYou()        const { return m_gotYou; }        // 0/1/2
    f32    GetScareTimer()    const { return m_scareT; }
    i32    GetScareImg()      const { return m_scareImg; }      // 778 / 792

    // cameras / monitor
    i32    GetViewing()       const { return m_viewing; }       // 0 office, 2 monitor
    i32    GetYouIn()         const { return m_youIn; }         // selected cam 1..15
    bool   IsVentMap()        const { return m_mapVent; }
    i32    GetSelIdx()        const { return m_selIdx; }        // map selector 0..15
    i32    GetSealedVent()    const { return m_sealedVent; }    // 0 none, 11..15
    i32    GetSealTarget()    const { return m_sealTarget; }    // sealing now
    f32    GetSealProgress()  const { return m_sealProgress; }
    f32    GetSealDuration()  const { return m_sealDuration; }
    i32    GetPlayCounter()   const { return m_playCounter; }   // 0..7 lure charge

    // maintenance panel
    bool   IsPanelOpen()      const { return m_panelOpen; }
    i32    GetPanelCursor()   const { return m_panelCursor; }   // 1..5
    i32    GetRebooting()     const { return m_rebooting; }     // 0..4
    f32    GetRebootProgress()const { return m_rebootProgress; }
    i32    GetAudioMeter()    const { return m_audioMeter; }
    i32    GetCameraMeter()   const { return m_cameraMeter; }
    i32    GetVentMeter()     const { return m_ventMeter; }
    f32    GetBlackoutAlpha() const { return m_blackoutAlpha; } // 0..255 fade
    bool   IsHallucinating()  const { return m_hallucinating; }
    f32    GetMoveStatic()    const { return m_moveStatic; }    // 0..1 flare

    // phantoms (0 = off, 1 = armed, 2 = visible; Chica 3 = at the window)
    i32    GetPhBB()          const { return m_phBB; }
    i32    GetPhFoxy()        const { return m_phFoxy; }
    i32    GetPhMangle()      const { return m_phMangle; }
    i32    GetPhPuppet()      const { return m_phPuppet; }
    f32    GetPhPuppetRushT() const { return m_phPuppetRushT; }   // img 320 rush
    i32    GetPhChica()       const { return m_phChica; }
    i32    GetPhGF()          const { return m_phGF; }
    f32    GetPhGFWalkT()     const { return m_phGFWalkT; }
    bool   IsShadowFreddy()   const { return m_shadowFreddy; }
    f32    GetWhiteFlash()    const { return m_whiteFlash; }    // scare flash 0..1
    f32    GetFoxyScareT()    const { return m_foxyScareT; }    // office Foxy anim

    // title menu (rows: new game / load game / nightmare / extra)
    i32    GetOptionSelected()const { return m_optionSelected; }
    i32    GetLastNight()     const { return m_lastNight; }
    bool   IsBeat5()          const { return m_beat5; }

    // v2.63: new screens
    i32    GetRareId()        const { return m_rareId; }        // 1/2/3 poster
    i32    GetGameOverRare()  const { return m_gameOverRare; }  // 1 = the rare roll hit
    const Mg3State&   Mg()    const { return m_mg; }
    const CutsceneState& Cs() const { return m_cs; }
    const ExtrasState& Ex()    const { return m_ex; }
    bool   IsBeat6()          const { return m_beat6; }
    bool   IsGoodEnd()        const { return m_goodend; }
    bool   IsFourthStar()     const { return m_fourthStar; }
    bool   HasBB()            const { return m_bb; }     // the balloon unlock
    bool   HasCake()          const { return m_cake; }
    // the cheats (the office consults them; the extras menu writes them)
    bool   CheatFast()        const { return m_fastNights; }
    bool   CheatVentProof()   const { return m_ventProof; }
    bool   CheatHyper()       const { return m_hyper; }
    bool   CheatNoCams()      const { return m_noCams; }

    // v2.63: the minigame entry points (the office secrets call these)
    void   StartMinigame(i32 game, bool fromExtras);
    void   ExitMinigameToExtras();

    // v2.62: the freddy3 save bridge (the module loads at boot and writes
    // when the next-day screen flips the dirty bit)
    void   ApplyProgressF3(const Progress::GameProgressF3& p);
    void   FillProgressF3(Progress::GameProgressF3& p) const;
    bool   ConsumeSaveDirty() { const bool d = m_saveDirty; m_saveDirty = false; return d; }

    // feed image table (dump "camera screen" act #17 values): empty/Springtrap
    static i32 FeedImg(i32 cam, bool springtrap);
    // is Springtrap currently visible on this cam? (the feed draws his frame)
    bool   SpringtrapOnCam(i32 cam) const;

private:
    void InitNightState();       // frame-start groups (also the death restart)
    void TickOffice(f32 dt, const FNaF3Inputs& in);
    void TickClock(f32 dt);
    void TickSpringtrap(f32 dt); // move counter + action roll + room graph
    void ExecuteAction(i32 act); // one movement-graph step
    void EnterVent(i32 vent);    // sealed -> bounce, open -> resolve
    void TickLure(f32 dt);
    void PlayLure(i32 cam);      // the adjacency table
    void TickPanel(f32 dt);
    void TickPhantoms(f32 dt);
    void PhantomScare(i32 who);  // shared scream/lockout/vent error
    void TickGotYou(f32 dt);
    void ResetNightInPlace();    // dump Group 606: frame 3 reloads itself
    void Sfx(const char* s, bool loop, i32 ch, i32 vol);
    void SfxStop(const char* s);

    // v2.63: the new screens
    void TickWhatDay(f32 dt, const FNaF3Inputs& in);   // the night card + wait
    void TickStaticDeath(f32 dt);                      // the death static (frame 4)
    void TickGameOver(f32 dt, const FNaF3Inputs& in);  // frame 6
    void TickRare(f32 dt, const FNaF3Inputs& in);      // frames 12/13/14
    void TickLoad(f32 dt);                             // frame 18
    void TickCutscene(f32 dt, const FNaF3Inputs& in);  // frame 16
    void TickMinigame(f32 dt, const FNaF3Inputs& in);  // frames 19-24
    void TickExtras(f32 dt, const FNaF3Inputs& in);    // frame 25
    void MgStep(f32 dt, const FNaF3Inputs& in);        // the shared platformer
    bool MgObstacle(f32 x, f32 y) const;               // wall probe (rect map)
    bool MgBalloonAt(f32 x, f32 y) const;              // the bounce floors
    void MgCheckPickups();                             // collects/exits/cakes
    void GoWhatDay();                                  // the card -> wait -> office
    void WriteNightProgress();                         // the next-day INI keys

    Screen m_screen;
    i32    m_night;
    i32    m_timeOfNight;    // 0 = 12 AM .. 5, 6 = 6 AM win
    f32    m_hourClock;      // seconds into the current hour
    i32    m_ai;             // per-night level (0, 2..5, 7)
    i32    m_timeLimit;      // phantom stare frames (100/90/80/70/60/50)
    bool   m_aggressive;
    f32    m_aggrT;          // 15 s aggressive reset
    f32    m_aggrRollT;      // 5 s Random(5) < AI roll

    // Springtrap ("dhfgh")
    i32    m_stRoom;         // Room3
    f32    m_moveCounter;
    i32    m_totalTurns;
    i32    m_action;         // 1..4 (0 = none)
    bool   m_queued;         // alt17: ST1 -> waiting for a screen
    i32    m_playEffect;     // vent sound id 1..4
    f32    m_runPastT;       // hall dash overlay timer
    bool   m_bigScare;       // GOT YOU 2 while monitor up
    f32    m_picT;           // pic random re-roll (10 s)
    i32    m_picRandom;      // 0/1 fork at rooms 05/02

    // cameras / monitor
    i32    m_viewing;        // 0 office, 2 monitor
    i32    m_youIn;          // selected cam 1..15
    bool   m_mapVent;        // vent map toggled
    i32    m_selIdx;         // 0..14 buttons, 15 = toggle button
    i32    m_sealedVent;     // what vent is closed (11..15, 0 none)
    i32    m_sealTarget;     // going to seal
    f32    m_sealProgress;   // counts up to the seal duration
    f32    m_sealDuration;
    i32    m_playCounter;    // 0..7 (7 = lure ready)
    f32    m_playT;          // 1.5 s refill

    // lure
    f32    m_lureT;          // 2 s lifetime (-1 off)
    i32    m_lureCountdown;  // Random(100) frames
    i32    m_lureTarget;     // Room3 destination

    // maintenance panel
    bool   m_panelOpen;
    i32    m_panelCursor;    // 1..5
    i32    m_rebooting;      // 1..4
    f32    m_rebootProgress;
    f32    m_rebootT;        // 1 s progress tick
    i32    m_audioMeter;
    i32    m_cameraMeter;
    i32    m_ventMeter;
    f32    m_camUseAcc;      // monitor-up seconds (12 s -> camera damage)
    f32    m_ventErrT;       // per-AI vent decay timer
    f32    m_ignoreAcc;      // panel ignored seconds
    f32    m_boAcc;          // vent-error frames (blackout pipeline)
    f32    m_blackoutAlpha;  // 0..255
    bool   m_hallucinating;
    f32    m_moveStatic;     // feed flare 0..1

    // phantoms
    i32    m_phBB;           // 0/1 armed/2 shown
    f32    m_phBBStare;
    i32    m_phFoxy;         // shown in the office
    f32    m_foxyScareT;
    i32    m_phMangle;
    f32    m_phMangleStare;
    i32    m_phPuppet;
    f32    m_phPuppetStare;
    f32    m_phPuppetRushT;
    i32    m_phChica;        // 2 on cam, 3 at the window
    f32    m_phChicaStare;
    i32    m_phGF;           // 1 armed, 2 walking
    f32    m_phGFT;          // 60 s arm roll
    f32    m_phGFStepT;      // 1 s walk steps
    f32    m_phGFWalkT;      // walk-across anim clock
    f32    m_phGFStare;
    bool   m_shadowFreddy;
    f32    m_phantomRollT;   // 20 s shared roll timer
    f32    m_scareCooldown;  // 10 -> 0
    f32    m_frozen;         // monitor lockout
    f32    m_whiteFlash;     // 0..1 scare flash

    // got you / death
    i32    m_gotYou;         // 0/1/2
    f32    m_gotYouT;        // yank frames
    f32    m_scareT;         // jumpscare anim timer
    i32    m_scareImg;       // 778 / 792

    // title menu / session
    i32    m_optionSelected;
    i32    m_lastNight;
    bool   m_beat5;
    i32    m_cine;            // v2.62: persisted (the dump's cutscene counter)
    bool   m_saveDirty;

    // v2.63: the new-flow state
    i32    m_rareId;          // which rare screen (1 boot / 2 gameover / 3 card)
    i32    m_gameOverRare;    // the gameover 1/1000 roll result
    f32    m_waitT;           // the 100 ms wait screen
    bool   m_died;            // the office death routed to the static+gameover
    Mg3State       m_mg;
    CutsceneState  m_cs;
    ExtrasState    m_ex;
    bool   m_beat6;
    bool   m_goodend;
    bool   m_fourthStar;
    bool   m_bb, m_cake, m_k1, m_k2, m_k3, m_k4;
    bool   m_fastNights, m_ventProof, m_hyper, m_noCams;

    f32    m_pan;            // office scroll 0..1488
    f32    m_cardT;
    f32    m_time;
};

} // namespace fnaf

#endif // FNAF3_GAME_H
