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

    // Toggle a hallway light
    void ToggleLight(DoorSide side);
    void SetLight(DoorSide side, bool on);

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

    // Jump scare info
    AnimatronicId GetJumpscareAnimatronic() const;
    bool HasJumpscareTriggered() const;

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

    // Movement opportunity tracking
    i32 m_ticksSinceLastMovement; // Ticks since last AI movement opportunity

    // Power-out sub-state
    f32  m_powerOutTimer;       // Seconds since power went out
    f32  m_powerOutDuration;    // Random duration before Freddy attacks
    bool m_powerOutDurationSet;

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
    void ApplyAIChangesForHour(i32 hour);

    void NotifyTimeUpdate();
    void NotifyPowerUpdate();

    // Simple RNG for power-out duration
    i32 m_simpleRNG;
    i32 SimpleRandom(i32 min, i32 max);
};

} // namespace fnaf

#endif // FNAF_GAME_H
