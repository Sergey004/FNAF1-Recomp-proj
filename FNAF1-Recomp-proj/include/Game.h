/**
 * Five Nights at Freddy's 1 — Recompilation
 * Game.h: Main game class — ties all systems together
 *
 * This is the single point of entry for game logic.
 * Call Init(), then Tick() every frame (60fps).
 * Check GetState() for the current game state.
 */

#ifndef FNAF_GAME_H
#define FNAF_GAME_H

#include "Types.h"
#include "GameTimer.h"
#include "PowerSystem.h"
#include "DoorSystem.h"
#include "CameraSystem.h"
#include "AnimatronicAI.h"
#include "NightConfig.h"

namespace fnaf {

// ============================================================
//  Game: The main game state machine
//
//  States:
//   MENU → NIGHT_START → PLAYING → (JUMPSCARE | NIGHT_COMPLETE | GAME_OVER)
//  Also: PLAYING → POWER_OUT → (JUMPSCARE | NIGHT_COMPLETE)
// ============================================================

class Game {
public:
    Game();
    ~Game();

    // Initialize for a specific night (1-7)
    void Init(i32 night);
    // v2.46: Night 7 with the Customize screen's levels (groups 305-311:
    // the original copies the four global AI counters into the activity
    // counters on night 7). Call after Init(7)-equivalent setup.
    void InitCustomNight(const i32 levels[4]);

    // Called every logic tick (1/60 second)
    // Returns the current game state after processing.
    GameState Tick();

    // === Player Actions (called by platform input layer) ===

    // Toggle camera monitor up/down
    void ToggleCamera();
    void SetCameraUp(bool up);

    // Switch to a specific camera (1A-7)
    void SwitchCamera(CameraId cam);

    // Toggle a door
    void ToggleDoor(DoorSide side);
    void SetDoor(DoorSide side, bool closed);
    void SetDoorAmount(DoorSide side, float amount);   // v2.21 analog-door test

    // v2.22: advance the visual door slide (real-time dt). Called each render
    // frame so the non-analog door animates at the original 16-frame @30 FPS.
    void TickDoors(f32 dt);

    // Toggle a hallway light
    void ToggleLight(DoorSide side);
    void SetLight(DoorSide side, bool on);

    // v2.17 DEV: jump straight to the 6 AM completion (fires onNightComplete too)
    void DebugForceNightComplete();
    // v2.17 DEV: god mode (power never drains, animatronics never attack)
    void SetDebugGodMode(bool on);
    // v2.17 DEV: force the power-out sequence / a specific jumpscare
    void DebugTriggerPowerOut();
    void DebugTriggerJumpscare(AnimatronicId anim);
    // v2.22: Golden Freddy kills the player (reroute through the jumpscare
    // state with ANIM_COUNT as the sentinel; plays onJumpscare + XSCREAM).
    void DebugTriggerGoldenFreddy();

    // === State Queries ===

    GameState         GetState() const;
    i32               GetCurrentNight() const;
    const GameTimer&  GetTimer() const;
    const PowerSystem&GetPower() const;
    const DoorSystem& GetDoors() const;
    const CameraSystem&GetCameras() const;
    const AnimatronicAI& GetAI() const;

    // Power-out state
    bool IsPowerOut() const;
    f32  GetPowerOutTimer() const;
    
    // Power-out sub-phase (docs/AI_MECHANICS.md §8):
    // 0 = dark, 1 = music box + face flicker, 2 = buzz blink, 3 = black
    i32  GetPowerOutPhase() const;
    // Phase 1: is Freddy's lit face showing right now (25% re-roll / 0.5 s)
    bool IsFreddyFaceLit() const;
    // v2.22: "Active 2" face state 0..4 — the distinct power-out face sounds
    // (state 1 = computer-digital, 2/3/4 = garble1/2/3). Advances on each new
    // face flash during phase 1.
    i32  GetPowerOutFaceState() const;
    // Phase 2: buzz blink — office visible for the whole 20 ticks or not
    bool IsPowerOutBlinkOn() const;
    // v2.46: monitor static-out ticks (groups 194-198): set to 300 when an
    // animatronic moves while its room is on screen; RenderCamera blanks the
    // feed and storms static for the window; main.cpp plays the garble on
    // the rising edge. 0 = idle.
    i32  GetFeedStaticTicks() const { return m_feedStaticTicks; }
    
    // Jump scare info
    AnimatronicId GetJumpscareAnimatronic() const;
    bool HasJumpscareTriggered() const;
    // How long the scare state lasts (anim + hold), per animatronic
    f64  GetJumpscareDurationSec() const;

    // Set callbacks for platform integration
    void SetCallbacks(const GameCallbacks& cb);

    // Callbacks (const access for external systems)
    const GameCallbacks& GetCallbacks() const;

private:
    // Sub-systems
    GameTimer     m_timer;
    PowerSystem   m_power;
    DoorSystem    m_doors;
    CameraSystem  m_cameras;
    AnimatronicAI  m_ai;

    // Game state
    GameState      m_state;
    i32           m_currentNight;
    GameCallbacks  m_callbacks;

    // Night config
    const NightConfig* m_nightConfig;
    i32                m_lastAIHour; // Track last hour we applied AI changes

    // Movement opportunities are scheduled inside AnimatronicAI (own
    // interval per animatronic) — no shared counter anymore.
    
    // Power-out sub-state
    f32  m_powerOutTimer;       // Seconds since power went out
    i32  m_powerOutPhase;       // 0..3 (docs/AI_MECHANICS.md §8)
    f64  m_powerOutPhaseTimer;  // Seconds in the current phase
    f64  m_powerOutRollTimer;   // Sub-timer for the phase rolls
    bool m_freddyFaceLit;       // Phase 1 flicker state
    f64  m_faceLitTimer;        // 0.5 s re-roll accumulator
    i32  m_facePhase;           // v2.22: "Active 2" face state 0..4 (sound index)
    bool m_powerOutBlinkOn;     // Phase 2: office visible or hidden
    bool m_musicBoxPlaying;     // callback bookkeeping
    bool m_debugGodMode;        // v2.17 DEV: no power drain, no attacks
    i32  m_feedStaticTicks;     // v2.46: monitor static-out window (300 ticks)

    // Night start display timer
    f32  m_nightStartTimer;

    // Jump scare display timer
    f32  m_jumpscareTimer;
    bool m_jumpscareTriggered;
    AnimatronicId m_jumpscareAnimatronic;

    // Night complete display timer
    f32  m_nightCompleteTimer;

    // Internal processing
    void ProcessPlaying();
    void ProcessPowerOut();
    void ProcessJumpscare();
    void ProcessNightStart();
    void ProcessNightComplete();
    void ApplyHourDeltas(i32 hour);

    void NotifyTimeUpdate();
    void NotifyPowerUpdate();

    // Simple RNG for power-out duration
    i32 m_simpleRNG;
    i32 SimpleRandom(i32 min, i32 max);
};

} // namespace fnaf

#endif // FNAF_GAME_H
