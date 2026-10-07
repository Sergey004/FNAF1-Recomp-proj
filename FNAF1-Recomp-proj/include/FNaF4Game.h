/**
 * FNaF4Game.h: v2.61 — FNAF4 game state, wave 1 (playable night loop).
 *
 * Implements the bedroom-frame digest (Events/frame_3_level_events.txt,
 * 480 groups): the follow-state machine (center / left door / right door /
 * closet / bed), the four threat lanes (Nightmare Bonnie/Chica doors with
 * the listen-for-breathing defense, Nightmare Foxy's closet with the
 * forced flashlight, Nightmare Freddy's bed counter), Nightmare Fredbear
 * (night 5+), the paranoia black-flash pipeline, the hour clock (raw-ms
 * timers — 60 s per hour) and the jumpscare flow. Timer convention: the
 * dumped "timer N" values are RAW MILLISECONDS (the project-wide reading;
 * the dump's "~Ns" labels divide by 50 and read 20x long).
 *
 * The class stays engine-free: input arrives as plain fields, audio runs
 * through callbacks owned by the module (the FNaF2Game contract).
 */

#ifndef FNAF4_GAME_H
#define FNAF4_GAME_H

#include "Types.h"
#include "Progress.h"    // v2.64: GameProgressF4 (the "fn4" save struct)

namespace fnaf {

struct FNaF4Inputs {         // translated from GameInput by the module
    bool aPressed;           // confirm (edge, title)
    bool aHeld;              // flashlight / peek hold
    bool xHeld;              // door-shut hold (at the doors)
    bool bPressed;           // get up (edge)
    bool upPressed, downPressed, leftPressed, rightPressed;
    f32  lookDir;            // spare (the bedroom pans per position)

    FNaF4Inputs() : aPressed(false), aHeld(false), xHeld(false), bPressed(false),
                    upPressed(false), downPressed(false),
                    leftPressed(false), rightPressed(false), lookDir(0.0f) {}
};

// Audio hooks the module fills (all optional). Volumes are 0..100
// Clickteam-style; the module converts and routes to its channels.
struct FNaF4AudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    FNaF4AudioHooks() : play(0), stop(0), channelVolume(0) {}
};

class FNaF4Game {
public:
    enum Screen {
        SCR_DISCLAIMER = -1,
        SCR_TITLE      = 0,   // frame 1 "titlescreen"
        SCR_NIGHTSTART = 1,   // frame 2 "what night" card
        SCR_BEDROOM    = 2,   // frame 3 "level"
        SCR_NIGHTWIN   = 3,   // frame 5 "night win" (the 6 AM clock digits)
        // v2.64: the dump's full flow (jumps = storyboard slots: 0 level,
        // 1 game over, 2 what night, 3 night win, 4 title, 5 intro plush,
        // 6 plush game, 7 lockbox, 8 game over 2, 9 extras, 10 load extras,
        // 11 disclaimer, 12 Cutscenes, 13 ending, 14 test, 15 nightmare
        // jumpscare, 16 demo, 18 BB game)
        SCR_GAMEOVER   = 4,   // frame 4: death -> 7 s -> title
        SCR_GAMEOVER2  = 5,   // frame 8: the minigame catch -> what night/extras
        SCR_INTRO      = 6,   // frames 6/17: the minigame intros (6 s / A)
        SCR_PLUSH      = 7,   // frame 7: Fun with Plushtrap
        SCR_LOCKBOX    = 8,   // frame 9: the unlock box (after night 7)
        SCR_LOADX      = 9,   // frame 11: instant -> lockbox
        SCR_EXTRAS     = 10,  // frame 10
        SCR_CUTSCENE   = 11,  // frame 12: the walkable house scenes
        SCR_ENDING     = 12,  // frame 13: the typewriter dialogue
        SCR_TEST       = 13,  // frame 14: the test-room skip
        SCR_NJSCARE    = 14,  // frame 15: the nightmare jumpscare hold
        SCR_BB         = 15   // frame 18: Fun with Balloon Boy
    };

    // the follow states (dump follow.alterable[0] values kept as comments)
    enum Pos {
        P_CENTER = 0,    // 0
        P_LEFT   = 1,    // 10
        P_RIGHT  = 2,    // 17
        P_CLOSET = 3,    // 33
        P_BED    = 4     // 43
    };

    FNaF4Game();

    void ResetToTitle();
    void StartNight(i32 night);
    void Tick(f32 dt, const FNaF4Inputs& in);

    // audio hooks (module wires them once)
    FNaF4AudioHooks audio;

    // ---- state for the module renderer ----
    Screen GetScreen()         const { return m_screen; }
    i32    GetNight()          const { return m_night; }
    i32    GetHour()           const { return m_hour; }          // 0=12AM..5
    f32    GetCardTimer()      const { return m_cardT; }
    f32    GetClock()          const { return m_time; }
    Pos    GetPosition()       const { return m_pos; }
    bool   IsWalking()         const { return m_walkT > 0.0f; }
    f32    GetWalkT()          const { return m_walkT; }         // 0.45..0
    i32    GetWalkDir()        const { return m_walkDir; }       // art id
    bool   Peeking()           const { return m_peeking; }
    bool   IsDoorShut(i32 side)const { return side == 0 ? m_leftShut : m_rightShut; }
    i32    GetListening()      const { return m_listening; }     // 0/1 left/2 right

    // lanes (far = 0, near = 1; foxy counter 0..10; freddy counter 0..60+)
    i32    GetBonniePos()      const { return m_bonniePos; }
    i32    GetChicaPos()       const { return m_chicaPos; }
    bool   IsFoxyInCloset()    const { return m_foxyIn; }
    i32    GetClosetCounter()  const { return m_closetCounter; } // 0..10
    i32    GetFoxyGotYou()     const { return m_foxyGotYou; }
    bool   IsFredbearCloset()  const { return m_fredbearAI > 0; }
    i32    GetFreddyCounter()  const { return m_freddyCounter; }
    i32    GetTotalDanger()    const { return m_totalDanger; }
    f32    GetBlackFlashA()    const { return m_blackFlashA; }   // 0..255

    // attacks (0 none; else the fullscreen overlay image)
    i32    GetAttackImg()      const { return m_attackImg; }
    f32    GetAttackT()        const { return m_attackT; }
    f32    GetBiteT()          const { return m_biteT; }        // cosmetic bite
    bool   IsGameOver()        const { return m_gameover; }

    // title menu (rows: new game / continue / 6th night / extra)
    i32    GetOptionSelected() const { return m_optionSelected; }
    i32    GetLastNight()      const { return m_lastNight; }
    bool   IsBeat5()           const { return m_beat5; }

    // ---- v2.64: the minigame / flow state (see FNaF4Game.cpp for the
    // dump digests). Plushtrap/BB share one engine: the 9-position hall
    // graph (in chair / stage 1 / far left+right / stage 2 / close
    // left+right / stage 3 / got you), the Ctrl flash (A hold on the pad),
    // the darkness timer ("becoming active"), the per-position view anims
    // and the stage-3 flash = the win. ----
    struct PtState {
        i32  game;          // 0 plushtrap, 1 BB
        bool fromExtras;    // "minigame play" >= 1
        i32  hallPos;       // 0 in chair, 1 stage1, 2 far left, 3 far right,
                            // 4 stage2, 5 close left, 6 close right,
                            // 7 stage3, 8 got you
        i32  viewState;     // the dump's Active.alt0 (0/1/2/3/98/99)
        i32  viewAnim;      // the dump's Active anim (12..23, 0)
        f32  darkT;         // "becoming active" accumulator (0..500)
        f32  moveGateT;     // the 2 s move gate
        f32  stepT;         // the 500 ms stage-3 extra move (BB)
        i32  fork;          // "random" = Random(2)+1
        f32  clockT;        // the countdown clock
        i32  clock;         // seconds left ("Counter")
        bool won;           // reward / BB reward
        f32  winT;          // alt2 150 frames
        bool scare;         // alt0 = 99: the jumpscare (anim 23)
        f32  bbVoiceT;      // the BB laugh roll
        void Clear() {
            game = 0; fromExtras = false; hallPos = 0; viewState = 0; viewAnim = 0;
            darkT = 0.0f; moveGateT = 0.0f; stepT = 0.0f; fork = 0;
            clockT = 0.0f; clock = 90; won = false; winT = 0.0f; scare = false;
            bbVoiceT = 0.0f;
        }
        PtState() { Clear(); }
    };

    struct CutsceneState {
        i32  scene;         // the INI "scene" + 1 (0 = the title intro)
        f32  px, py;        // the hit box in the 5120x3840 house
        f32  camX, camY;
        f32  stepT;         // the 100 ms walk gate
        f32  textT;         // the typewriter/dialogue beat (labeled stop-gap)
        bool done;          // "end" == 1
        f32  doneT;         // the 1 s exit beat
        void Clear() {
            scene = 0; px = 2560.0f; py = 1920.0f; camX = 2048.0f; camY = 1536.0f;
            stepT = 0.0f; textT = 0.0f; done = false; doneT = 0.0f;
        }
        CutsceneState() { Clear(); }
    };

    struct ExtrasState {
        i32  row;           // the dump's "selection" 0..9
        i32  pick;          // the left/right viewer pick
        void Clear() { row = 0; pick = 0; }
        ExtrasState() { Clear(); }
    };

    // getters for the module renderer
    const PtState&       Pt() const { return m_pt; }
    const CutsceneState& Cut() const { return m_cut; }
    const ExtrasState&   Ex() const { return m_ex; }
    i32  GetNightWinDigit(i32 i) const { return m_winDigit[i]; }   // 0 flicker,1 set
    i32  GetNightWinVal(i32 i)   const { return m_winVal[i]; }
    bool IsMinigamePlay()        const { return m_minigamePlay; }
    bool CheatFast()             const { return m_fastNights; }
    bool CheatAllNightmare()     const { return m_allNightmare; }

    // the minigame entry points (the office/extras call these)
    void StartMinigame(i32 game, bool fromExtras);
    void GoWhatNight(i32 night);

    // the fn4 save bridge (the module loads at boot, writes on the beats)
    void ApplyProgressF4(const Progress::GameProgressF4& p);
    void FillProgressF4(Progress::GameProgressF4& p) const;
    bool ConsumeSaveDirty() { const bool d = m_saveDirty; m_saveDirty = false; return d; }

private:
    void InitNightState();
    void TickBedroom(f32 dt, const FNaF4Inputs& in);
    void TickClock(f32 dt);
    void ApplyNightAI();                    // the per-night/hour AI tables
    void TickLanes(f32 dt);
    void TickFoxy(f32 dt, bool viewingCloset);
    void TickFreddy(f32 dt, bool viewingBed);
    void TickParanoia(f32 dt);
    void TriggerJumpscare(i32 overlayImg);  // scream2 + gameover flow
    void Sfx(const char* s, bool loop, i32 ch, i32 vol);
    void SfxStop(const char* s);
    void ChVol(i32 ch, i32 vol);

    // v2.64: the new flow screens
    void TickWhatNight(f32 dt, const FNaF4Inputs& in);  // the real card
    void TickNightWin(f32 dt, const FNaF4Inputs& in);   // the real 6 AM
    void TickGameOver(f32 dt, const FNaF4Inputs& in);
    void TickGameOver2(f32 dt);
    void TickIntro(f32 dt, const FNaF4Inputs& in);
    void TickMinigame(f32 dt, const FNaF4Inputs& in);   // Plushtrap + BB
    void TickLockbox(f32 dt, const FNaF4Inputs& in);
    void TickExtras(f32 dt, const FNaF4Inputs& in);
    void TickCutscene(f32 dt, const FNaF4Inputs& in);
    void TickEnding(f32 dt, const FNaF4Inputs& in);
    void WriteNightResult();                            // the night-win INI keys

    Screen m_screen;
    i32    m_night;
    i32    m_hour;             // 0 = 12 AM .. 5, 6 = 6 AM
    f32    m_hourClock;
    f32    m_cardT;
    f32    m_time;

    // player
    Pos    m_pos;
    f32    m_walkT;            // walk animation 0.45..0
    i32    m_walkDir;          // dark-overlay art (45 left / 160 right / 57 bed)
    Pos    m_walkTo;
    bool   m_peeking;
    bool   m_leftShut, m_rightShut;
    bool   m_prevLeftShut, m_prevRightShut;
    i32    m_listening;        // 0 none, 1 left door, 2 right door

    // lanes
    i32    m_bonnieAI, m_chicaAI, m_freddyAI, m_foxyAI, m_fredbearAI;
    i32    m_bonniePos, m_chicaPos;          // 0 far, 1 near
    f32    m_laneRollT;                      // 5 s shared roll
    f32    m_bonnieIgnoreT, m_chicaIgnoreT;  // 10 s retreat
    f32    m_bonnieNearT, m_chicaNearT;      // 1 s ticks at near (alterable[6])
    bool   m_bonnieBed, m_chicaBed;          // alterable[7]: bedroom attack armed
    f32    m_bonnieVisitT, m_chicaVisitT;    // shut-door visit (3 s)

    // closet lane (Foxy or Fredbear)
    bool   m_foxyIn;
    i32    m_closetCounter;                  // 0..10
    i32    m_foxyGotYou;                     // forced flashlight flag
    f32    m_foxyRollT;                      // 5 s entry roll
    f32    m_closetTickT;                    // 1 s counter tick
    f32    m_fredbearSndT;                   // bite/scare cooldown

    // bed lane
    i32    m_freddyCounter;
    i32    m_miniArt;                        // the <=10 art pick (hash)
    f32    m_freddyT;                        // 4 s accumulate tick
    f32    m_bedTickT;                       // 1 s drain tick
    f32    m_biteT;                          // the non-fatal bite overlay

    // paranoia
    i32    m_totalDanger;
    f32    m_dangerT;                        // 4 s recompute
    f32    m_bfTimer;                        // black-flash scheduler
    f32    m_blackFlashA;                    // 0..255
    f32    m_bfHold;                         // frames left shown

    // attack / death
    i32    m_attackImg;
    f32    m_attackT;
    bool   m_gameover;

    // title menu / session
    i32    m_optionSelected;
    i32    m_lastNight;
    bool   m_beat5;

    // v2.64: the persisted fn4 state
    i32    m_scene;          // the INI "scene" (the last played cutscene)
    bool   m_beat6, m_beat7, m_beat8;
    bool   m_s1, m_s2, m_s3, m_s4, m_s5, m_s6;   // the challenge stars
    bool   m_testFlag;
    i32    m_shadow;         // the shadow-night pick (1 = night 7, 2 = 8)
    bool   m_minigamePlay;   // the extras replay flag
    bool   m_cheatHouseMap, m_fastNights, m_cheatRadar;
    bool   m_blindMode, m_instaFoxy, m_madFreddy, m_allNightmare;
    PtState        m_pt;
    CutsceneState  m_cut;
    ExtrasState    m_ex;
    i32    m_winDigit[4];    // the 6 AM digits: 0 flickering, 1 settled
    i32    m_winVal[4];
    f32    m_digitT;
    f32    m_winT;
    f32    m_lockT;
    f32    m_introT;
    f32    m_goT;
    f32    m_endT;
    i32    m_endLine;
    f32    m_endLetterT;
    bool   m_saveDirty;      // the module writes fn4 when this flips
    bool   m_ptFlashPrev;    // the minigame flash edge
    f32    m_lockLid;        // the lockbox lid animation 0..2
};

} // namespace fnaf

#endif // FNAF4_GAME_H
