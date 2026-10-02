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
        SCR_NIGHTWIN   = 3    // frame 5 "night win" (the 5->6 AM clock)
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
    void ChVol(i32 ch, i32 vol);

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
};

} // namespace fnaf

#endif // FNAF4_GAME_H
