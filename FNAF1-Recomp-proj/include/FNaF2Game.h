/**
 * FNaF2Game.h: v2.31 — FNAF2 game-state skeleton.
 *
 * The counters mirror the dump by NAME (frame 3 "Frame 1",
 * Events/frame_3_Frame 1_events.txt, 751 groups): viewing, lit?,
 * night, battery life, time of the night, in danger, office occupied,
 * mask, plus the AI counters (old/new Freddy/Bonnie/Chica/Foxy,
 * Balloon Boy, Sockpuppet, Golden Freddy) which arrive with the AI
 * stages. Dump-verified constants:
 *   battery life := 7000 at night start; -1 per frame while lit?==1
 *     (group 170); flashlight is HOLD (group 35) and is blocked by
 *     battery<=0, mask up, viewing!=0, "in danger"!=0 and by Balloon
 *     Boy having stolen it ("balloon boy" x "got you box").
 *   clock: "AM" alterable[0] >= 70 -> hour change; 12 -> 1 (group 484),
 *     else +1 (group 485) => 70 s per hour, night 12AM..6AM ~= 7 min.
 * The input is fed as plain fields so the class stays engine-free.
 */

#ifndef FNAF2_GAME_H
#define FNAF2_GAME_H

#include "Types.h"

namespace fnaf {

struct FNaF2Inputs {         // translated from GameInput by the module
    bool aPressed;           // confirm / monitor toggle (edge)
    bool upPressed;          // menu up (edge)
    bool downPressed;        // menu down (edge)
    bool leftPressed;        // cam cycle - (edge)
    bool rightPressed;       // cam cycle + (edge)
    bool lightHeld;          // flashlight hold (Ctrl on PC / LB on pad)
    bool maskHeld;            // v2.33: Freddy mask hold (LT on pad)
    bool windHeld;            // v2.33: music-box wind hold (RT on pad)
    f32  lookDir;            // office pan -1..1

    FNaF2Inputs() : aPressed(false), upPressed(false), downPressed(false),
                    leftPressed(false), rightPressed(false),
                    lightHeld(false), maskHeld(false), windHeld(false),
                    lookDir(0.0f) {}
};

class FNaF2Game {
public:
    enum Screen {
        SCR_DISCLAIMER = 4,  // frame 0 "Frame 17" (own warning per game)
        SCR_TITLE = 0,       // frame 1 "title"
        SCR_NIGHTSTART = 1,  // frame 2 "what day" (night card)
        SCR_OFFICE = 2,      // frame 3 "Frame 1"
        SCR_6AM = 3          // frame 5 "next day" (win, -> title)
    };

    FNaF2Game();

    void ResetToTitle();
    void StartNight(i32 night);
    void Tick(f32 dt, const FNaF2Inputs& in);

    // state for the module renderer
    Screen GetScreen()        const { return m_screen; }
    i32    GetNight()         const { return m_night; }
    i32    GetHour()          const { return m_timeOfNight; }
    i32    GetBatteryLife()   const { return m_batteryLife; }
    bool   IsLit()            const { return m_litQ != 0; }
    i32    GetMaskState()     const { return m_maskState; }  // 0 off..2 on
    f32    GetMusicBox()      const { return m_musicBox; }   // 0..kMusicMax
    bool   IsMasked()         const { return m_maskState == 2; }
    i32    GetViewing()       const { return m_viewing; }
    i32    GetOptionSelected()const { return m_optionSelected; }
    f32    GetCardTimer()     const { return m_cardT; }
    f32    GetClock()         const { return m_time; }

private:
    void ToggleMonitor();    // viewing 0 <-> cam 1
    void CycleCam(i32 dir);  // 1..12 wrap

    Screen m_screen;
    i32    m_night;            // "night"
    i32    m_timeOfNight;      // "time of the night" 12,1..5 (6 = win)
    f32    m_amClock;          // "AM" alterable[0] — 70 s per hour
    i32    m_batteryLife;      // "battery life" (7000 at night start)
    i32    m_litQ;             // "lit?" 0/1
    i32    m_viewing;          // "viewing" 0 = down, 1..12 = camera id
    i32    m_maskState;        // "mask" alterable[0]: 0 off,1 down,2 on,3 up
    f32    m_maskT;            // mask transition timer
    f32    m_musicBox;         // "music box counter" wind level
    f32    m_musicDrainAcc;    // drain accumulator
    i32    m_inDanger;         // "in danger" (blocks the light)
    bool   m_maskOn;           // "mask" alterable[0] != 0 (AI stage)
    bool   m_bbGotLight;       // "balloon boy" x "got you box" (BB stage)
    i32    m_optionSelected;   // title: 0 = New Game, 1 = Custom (locked)
    f32    m_cardT;            // night-card / 6AM card timer
    f32    m_time;             // free-running anim clock
};

} // namespace fnaf

#endif // FNAF2_GAME_H
