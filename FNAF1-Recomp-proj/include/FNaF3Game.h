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
        SCR_NEXTDAY    = 4    // frame 5 "next day" (payday card)
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

    f32    m_pan;            // office scroll 0..1488
    f32    m_cardT;
    f32    m_time;
};

} // namespace fnaf

#endif // FNAF3_GAME_H
