/**
 * SLGame.h: v2.65 — Sister Location game state, wave 1.
 *
 * Implements the pinned movement/game-flow model (docs/SL_MECHANICS.md; dump:
 * Dumps/Sister Location, 36 frames): rooms ARE frames (the window is
 * 1280x720, rooms are wider and pan — "you are the camera" with the
 * Perspective-eased tween), room-to-room travel is the global `go to`
 * counter armed by gameplay + the frame-4 "load" router (200 ms), in-room
 * movement = W-hold progress (the 15-tick crawl latch, Shift = the fast/
 * loud variant), and the Script Event spine drives the voice script.
 *
 * Wave 1 plays the core chain: Warning -> title -> Elevator (the ride) ->
 * the Main Hub (vent picks) -> the walk rooms (Ballora with the dance,
 * Funtime with the flash) -> Baby's Room -> win night -> tv show -> Girl
 * Voice -> the next night. The scripted content (HandUnit beats per night,
 * the P&S face buttons, the breaker task, Under Desk, the scooping chain,
 * the custom night, the 8-bit game) is labeled skeleton for the next waves.
 *
 * The class stays engine-free (the FNaF2Game contract).
 */

#ifndef SL_GAME_H
#define SL_GAME_H

#include "Types.h"
#include "Progress.h"    // GameProgressSL (the "sl" save struct)

namespace fnaf {

struct SLInputs {            // translated from GameInput by the module
    bool aPressed;           // confirm / shock buttons (edge)
    bool bPressed;           // back (edge)
    bool xPressed;           // shock/action (edge)
    bool yPressed;           // extras-heavy action (edge)
    bool lbPressed, rbPressed;
    bool upPressed, downPressed, leftPressed, rightPressed;
    bool wHeld;              // LS up (the "walk" hold)
    bool aHeld;              // A hold (the task fill / the flash hold)
    bool shiftHeld;          // RT (the fast/loud walk)
    bool flasherPressed;     // X-edge expression (Funtime flash)
    f32  lookDir;            // pan
    SLInputs() : aPressed(false), bPressed(false), xPressed(false),
                 yPressed(false), lbPressed(false), rbPressed(false),
                 upPressed(false), downPressed(false), leftPressed(false),
                 rightPressed(false), wHeld(false), aHeld(false),
                 shiftHeld(false), flasherPressed(false), lookDir(0.0f) {}
                 flasherPressed(false), lookDir(0.0f) {}
};

struct SLAudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    // the music fades by channel volume; pan is the Ballora side cue
    void (*pan)(i32 channel, i32 panValue);                      // -100..100
    SLAudioHooks() : play(0), stop(0), channelVolume(0), pan(0) {}
};

class SLGame {
public:
    // the storyboard slots ARE room frames (the dump's order == slots here)
    enum Screen {
        SCR_WARNING   = 0,    // frame 0 (the legal splash; INI load there)
        SCR_TITLE     = 1,    // frame 1
        SCR_ELEVATOR  = 2,    // frame 2 (1900x1000, the ride)
        SCR_VENT      = 3,    // frame 3 (1700x1000)
        SCR_LOAD      = 4,    // frame 4 (the travel router, 200 ms)
        SCR_HUB       = 5,    // frame 5 (the Circus Control, 1900x1000)
        SCR_BABY      = 6,    // frame 6 (Baby's gallery, 1700x900)
        SCR_TOVENT    = 7,    // frame 7 (100 ms hop -> Elevator)
        SCR_BALLORA   = 8,    // frame 8 (2200x1100, the dance)
        SCR_BREAKER   = 9,    // frame 9
        SCR_FUNTIME   = 10,   // frame 10 (Foxy flash)
        SCR_PS        = 11,   // frame 11 (Parts and Service)
        SCR_WINNIGHT  = 12,   // frame 12 (Jingle_4b + INI current += 1)
        SCR_UNDERDESK = 13,   // frame 13 (Bidybab eye-match)
        SCR_DEATH     = 14,   // frame 14 (the 8-bit death router)
        SCR_GAMEOVER  = 15,   // frame 15 (the per-night respawn router)
        SCR_CUTN4     = 16,   // frame 16 (the first-launch Baby monologue)
        SCR_INTRO4    = 17,   // frame 17 (the night-4 start card)
        SCR_EXTRAS    = 18,   // frame 18
        SCR_PS2       = 19,   // frame 19 (night 5, Ennard)
        SCR_SCOOPING  = 20,   // frame 20 (night 4 end scene)
        SCR_REDFADE   = 21,   // frame 21
        SCR_BATHROOM  = 22,   // frame 22 (night-5 mirror scene)
        SCR_CREDITS   = 23,   // frame 23
        SCR_TVSHOW    = 24,   // frame 24
        SCR_GIRLVOICE = 25,   // frame 25 (the line_1..6 interludes)
        SCR_GAME8BIT  = 26,   // frame 26 (the 12800x720 platformer)
        SCR_FINAL     = 27,   // frame 27 (night 5 encounter)
        SCR_KEYPAD    = 28,   // frame 28 (the keycard keypad)
        SCR_CUSTMENU  = 29,   // frame 29
        SCR_CUSTWAIT  = 30,   // frame 30 (2 s loader)
        SCR_CUSTLEVEL = 31,   // frame 31 (the custom-night office)
        SCR_CUSTDEATH = 32,   // frame 32
        SCR_CUSTWIN   = 33,   // frame 33
        SCR_CUST8BIT  = 34,   // frame 34
        SCR_FINALCS   = 35    // frame 35
    };

    SLGame();

    void ResetToTitle();
    void Tick(f32 dt, const SLInputs& in);
    void GoTo(i32 code);              // arm the global "go to" counter
    void ArmRouter();                 // fade + 200 ms -> SCR_LOAD consume

    SLAudioHooks audio;

    // ---- state for the module renderer ----
    Screen GetScreen()    const { return m_screen; }
    i32    GetNight()     const { return m_night; }
    i32    GetOptionSelected() const { return m_optionSelected; }
    f32    GetPan()       const { return m_pan; }
    f32    GetPanTarget() const { return m_panTarget; }
    f32    GetCardT()     const { return m_cardT; }
    i32    GetGoTo()      const { return m_goTo; }
    i32    GetScriptEvent() const { return m_scriptEvent; }

    // the walk model
    i32    GetProgress()  const { return m_progress; }
    i32    GetDistance()  const { return m_distance; }     // Ballora hearing
    i32    GetLeftPan()   const { return m_leftPan; }      // her side cue
    f32    GetFlashCharge() const { return m_flashCharge; }// Funtime
    i32    GetFoxyDist()  const { return m_foxyDist; }
    i32    GetNotch()     const { return m_notch; }        // the vent 100-tick notches
    i32    GetVentGoing() const { return m_ventGoing; }
    bool   IsJumpscared() const { return m_scareT > 0.0f; }

    // the fnaf_sl save bridge
    void ApplyProgressSL(const Progress::GameProgressSL& p);
    void FillProgressSL(Progress::GameProgressSL& p) const;
    bool ConsumeSaveDirty() { const bool d = m_saveDirty; m_saveDirty = false; return d; }
    void WipeSave();
    bool IsBeat1() const { return m_beat1; }   // extras unlocked
    bool IsBeat3() const { return m_beat3; }   // custom unlocked

private:
    void Sfx(const char* s, bool loop, i32 ch, i32 vol);
    void SfxStop(const char* s);
    void SfxPan(i32 ch, i32 pan);
    void TickWarning(f32 dt, const SLInputs& in);
    void TickTitle(const SLInputs& in);
    void TickElevator(f32 dt, const SLInputs& in);
    void TickVent(f32 dt, const SLInputs& in);
    void TickHub(f32 dt, const SLInputs& in);
    void TickBallora(f32 dt, const SLInputs& in);
    void TickFuntime(f32 dt, const SLInputs& in);
    void TickBreaker(f32 dt, const SLInputs& in);
    void TickBreakerHold(f32 dt);            // the wave-1 task hold
    void TickPS(f32 dt, const SLInputs& in); // the face-button task
    void TickDesk(f32 dt, const SLInputs& in); // Under Desk (eye-match)
    void TickBaby(f32 dt, const SLInputs& in);
    void TickToVent(f32 dt);
    void TickWinNight(f32 dt);
    void TickTvShow(f32 dt);
    void TickGirlVoice(f32 dt);
    void TickDeath(f32 dt, const SLInputs& in);
    void TickGameOver(f32 dt, const SLInputs& in);
    void TickLoad(f32 dt);
    void TickWalkRoom(f32 dt, const SLInputs& in);   // the shared walk skeleton
    void NightStart();                                // the presets per night

    Screen m_screen;
    i32    m_night;          // the dump's "night 2" counter (1..6)
    i32    m_optionSelected;
    bool   m_started;        // a night is in progress
    i32    m_dieRoute;       // the game-over resume "go to" code

    // the router
    i32    m_goTo;           // the global counter armed by gameplay
    i32    m_scriptEvent;    // the voice-script index
    i32    m_print;          // text printing (blocks travel)
    f32    m_loadT;          // the 200 ms consume tick (5 hits = 1 s)

    // the pan tween ("you are the camera")
    f32    m_pan;
    f32    m_panTarget;

    // the walk model (docs §3): the crawl latch + progress
    i32    m_crawl;          // the 15-tick W-latch
    bool   m_wHeldOnce;      // the 2 s arm
    f32    m_wArmT;
    i32    m_quick;          // the "quick count" accumulator (>= 5 -> +1)
    i32    m_progress;
    i32    m_notch;          // vent alt1 (100 ticks per notch, 10 to pass)
    i32    m_ventGoing;      // the vent exit id 1..3
    f32    m_stepSndT;

    // Ballora (docs §4): `distance` = how much she heard you
    i32    m_distance;
    bool   m_moving;         // she is advancing (after 5 s continuous walk)
    f32    m_contWalkT;
    i32    m_leftPan;        // -100..100 (her side)
    bool   m_dancer;         // created at progress >= 400
    f32    m_panT;
    f32    m_moveT;          // her own movements (the cue rolls)
    i32    m_dirSide;

    // Funtime (docs §4): the Space flash attracts Foxy
    f32    m_flashCharge;    // 0..50 (2 s refill)
    i32    m_foxyDist;
    bool   m_foxyApproach;
    f32    m_foxyRollT;
    i32    m_foxyPos;        // 0..2 (position 1/2/3)
    f32    m_foxyPosT;
    bool   m_backwards;      // the backwards trip flag
    bool   m_haveKeycard;

    // Elevator: the ride state machine (stop elevator 0..4)
    i32    m_elevatorState;
    f32    m_elevatorT;      // the movement anim clock
    f32    m_elevButtonT;

    // ---- v2.66: the task rooms (the dump's group digests) ----
    // Breaker Room (frame 9): the reboot = hold A on the panel to fill
    // 100 per panel, 3 panels; the Funtime-Freddy counter rises with the
    // noise you make (steps, the door on entry, ...); his win at >= 7.
    i32    m_brkPanel;       // 0..3 panels done
    f32    m_brkFill;        // the current panel fill 0..100
    i32    m_brkFreddy;      // his approach counter (the Freddy counter)
    f32    m_brkRollT;       // the 3.5 s noise roll
    f32    m_brkAnimT;       // his pose anim

    // Parts and Service (frame 11): the Script Event button chain
    // (begin at 150; the clicks advance it; the 120 s timer threat + the
    // Bonnie puppets spawn at the spawn triggers when it expires)
    // + P&S 2 (frame 19) night 5: Ennard.
    f32    m_psTimer;        // the 120 s countdown (kill at 0)
    i32    m_psPuppets;      // spawn counter
    bool   m_psPuppet1, m_psPuppet2;   // live
    f32    m_psPup1T, m_psPup2T;       // hide-off ticks
    i32    m_psKill;

    // Under Desk (frame 13): the eye-match (keep the cursor over the face
    // spot while "hello_in_there" plays); fail = jumpscare -> death
    f32    m_deskHold;       // hold time on the spot
    f32    m_deskTotal;      // the sequence
    bool   m_deskPeek;

    // the shared scare/death pieces
    f32    m_scareT;         // active jumpscare clock (0 = none)
    i32    m_scareImg;       // which scare art
    f32    m_cardT;          // per-screen timer
    f32    m_time;

    // the win / girl-voice lines
    f32    m_winT;
    f32    m_tvT;
    f32    m_girlT;

    // the fnaf_sl save state (keys: current/intro/beat1/beat3/keycard/104)
    bool   m_saveDirty;
    i32    m_current;        // INI current (the night 1..5 the save holds)
    bool   m_intro;          // INI intro (night-4 intro seen)
    bool   m_beat1, m_beat3; // extras / custom unlocked
    bool   m_keycard;        // star 2
    i32    m_endsceneno;
    bool   m_star104;
};

} // namespace fnaf

#endif // SL_GAME_H
