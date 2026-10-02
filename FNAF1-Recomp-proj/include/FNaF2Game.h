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

namespace fnaf {

struct FNaF2Inputs {         // translated from GameInput by the module
    bool aPressed;           // confirm (edge)
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

    FNaF2Inputs() : aPressed(false), upPressed(false), downPressed(false),
                    leftPressed(false), rightPressed(false),
                    lightHeld(false), maskHeld(false), windHeld(false),
                    ventLightLHeld(false), ventLightRHeld(false),
                    lookDir(0.0f) {}
};

// Audio hooks the module fills (all optional). Volumes are 0..100
// Clickteam-style; the module converts and routes to its channels.
struct FNaF2AudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    FNaF2AudioHooks() : play(0), stop(0), channelVolume(0) {}
};

class FNaF2Game {
public:
    enum Screen {
        SCR_DISCLAIMER = 4,
        SCR_TITLE = 0,
        SCR_NIGHTSTART = 1,
        SCR_OFFICE = 2,
        SCR_6AM = 3
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
};

} // namespace fnaf

#endif // FNAF2_GAME_H
