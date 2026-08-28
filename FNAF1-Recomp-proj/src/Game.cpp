/**
 * Five Nights at Freddy's 1 — Recompilation
 * Game.cpp: Main game state machine implementation
 */

#include "Game.h"
#include <cstring>

namespace fnaf {

Game::Game()
    : m_state(GAME_STATE_MENU)
    , m_currentNight(1)
    , m_nightConfig(0)
    , m_lastAIHour(-1)
    , m_ticksSinceLastMovement(0)
    , m_powerOutTimer(0.0f)
    , m_powerOutDuration(10.0f)
    , m_powerOutDurationSet(false)
    , m_nightStartTimer(0.0f)
    , m_jumpscareTimer(0.0f)
    , m_jumpscareTriggered(false)
    , m_jumpscareAnimatronic(ANIM_FREDDY)
    , m_nightCompleteTimer(0.0f)
    , m_simpleRNG(42)
{
    m_callbacks = MakeNullCallbacks();
}


Game::~Game() {
}


void Game::Init(i32 night) {
    m_currentNight = night;
    m_nightConfig = GetNightConfig(night);
    m_lastAIHour = -1;

    // Reset all subsystems
    m_timer.Reset();
    m_power.Reset();
    m_doors.Reset();
    m_cameras.Reset();
    m_ai.Reset();

    // Set starting AI levels from night config
    if (m_nightConfig) {
        for (i32 i = 0; i < ANIM_COUNT; ++i) {
            m_ai.SetAILevel(static_cast<AnimatronicId>(i),
                             m_nightConfig->startingLevels[i]);
        }
    }

    // Reset state
    m_ticksSinceLastMovement = 0;
    m_powerOutTimer = 0.0f;
    m_powerOutDurationSet = false;
    m_nightStartTimer = 0.0f;
    m_jumpscareTimer = 0.0f;
    m_jumpscareTriggered = false;
    m_nightCompleteTimer = 0.0f;

    // Start with night start display
    m_state = GAME_STATE_NIGHT_START;
}


GameState Game::Tick() {
    switch (m_state) {
        case GAME_STATE_NIGHT_START:
            ProcessNightStart();
            break;
        case GAME_STATE_PLAYING:
            ProcessPlaying();
            break;
        case GAME_STATE_POWER_OUT:
            ProcessPowerOut();
            break;
        case GAME_STATE_JUMPSCARE:
            ProcessJumpscare();
            break;
        case GAME_STATE_NIGHT_COMPLETE:
            ProcessNightComplete();
            break;
        default:
            break;
    }

    return m_state;
}


// ============================================================
//  Player Actions
// ============================================================

void Game::ToggleCamera() {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    bool nowUp = m_cameras.ToggleMonitor();

    if (m_callbacks.onCameraChange) {
        m_callbacks.onCameraChange(m_cameras.GetCurrentCamera());
    }
}

void Game::SetCameraUp(bool up) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    m_cameras.SetMonitorUp(up);

    if (m_callbacks.onCameraChange) {
        m_callbacks.onCameraChange(m_cameras.GetCurrentCamera());
    }
}

void Game::SwitchCamera(CameraId cam) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    CameraId prevCam = m_cameras.GetCurrentCamera();
    m_cameras.SwitchCamera(cam);

    // Check if switching to or from Pirate Cove (affects Foxy)
    if (cam == CAM_1C && prevCam != CAM_1C) {
        m_ai.OnPirateCoveViewed();
    }

    if (m_callbacks.onCameraChange) {
        m_callbacks.onCameraChange(m_cameras.GetCurrentCamera());
    }
}

void Game::ToggleDoor(DoorSide side) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    bool closed = m_doors.ToggleDoor(side);
    if (m_callbacks.onDoorChange) {
        m_callbacks.onDoorChange(side, closed);
    }
}

void Game::SetDoor(DoorSide side, bool closed) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) {
        m_doors.ForceDoorsOpen();
        return;
    }

    m_doors.SetDoor(side, closed);
    if (m_callbacks.onDoorChange) {
        m_callbacks.onDoorChange(side, closed);
    }
}

void Game::ToggleLight(DoorSide side) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    bool on = m_doors.ToggleLight(side);
    if (m_callbacks.onLightChange) {
        m_callbacks.onLightChange(side, on);
    }
}

void Game::SetLight(DoorSide side, bool on) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) {
        m_doors.ForceLightsOff();
        return;
    }

    m_doors.SetLight(side, on);
    if (m_callbacks.onLightChange) {
        m_callbacks.onLightChange(side, on);
    }
}


// ============================================================
//  State Queries
// ============================================================

GameState         Game::GetState() const { return m_state; }
i32               Game::GetCurrentNight() const { return m_currentNight; }
const GameTimer&  Game::GetTimer() const { return m_timer; }
const PowerSystem&Game::GetPower() const { return m_power; }
const DoorSystem& Game::GetDoors() const { return m_doors; }
const CameraSystem&Game::GetCameras() const { return m_cameras; }
const AnimatronicAI& Game::GetAI() const { return m_ai; }
bool Game::IsPowerOut() const { return m_state == GAME_STATE_POWER_OUT; }
f32  Game::GetPowerOutTimer() const { return m_powerOutTimer; }
AnimatronicId Game::GetJumpscareAnimatronic() const { return m_jumpscareAnimatronic; }
bool Game::HasJumpscareTriggered() const { return m_jumpscareTriggered; }
void Game::SetCallbacks(const GameCallbacks& cb) { m_callbacks = cb; }
const GameCallbacks& Game::GetCallbacks() const { return m_callbacks; }


// ============================================================
//  Process: Night Start ("12 AM" title card)
// ============================================================

void Game::ProcessNightStart() {
    m_nightStartTimer += static_cast<f32>(TimeConstants::TICK_INTERVAL_SEC);

    if (m_nightStartTimer >= TimeConstants::NIGHT_START_DISPLAY_SEC) {
        m_state = GAME_STATE_PLAYING;
        NotifyTimeUpdate();
        NotifyPowerUpdate();
    }
}


// ============================================================
//  Process: Main Gameplay
// ============================================================

void Game::ProcessPlaying() {
    // 1. Advance timer
    bool hourChanged = m_timer.Tick();

    // Apply AI level changes for the new hour
    if (hourChanged) {
        i32 newHour = m_timer.GetHour();
        ApplyAIChangesForHour(newHour);
        NotifyTimeUpdate();
    }

    // 2. Check for night completion
    if (m_timer.IsNightComplete()) {
        m_state = GAME_STATE_NIGHT_COMPLETE;
        m_nightCompleteTimer = 0.0f;
        if (m_callbacks.onNightComplete) {
            m_callbacks.onNightComplete(m_currentNight);
        }
        return;
    }

    // 3. Drain power
    bool cameraUp = m_cameras.IsMonitorUp();
    bool leftDoor = m_doors.IsDoorClosed(DOOR_LEFT);
    bool rightDoor = m_doors.IsDoorClosed(DOOR_RIGHT);
    bool leftLight = m_doors.IsLightOn(DOOR_LEFT);
    bool rightLight = m_doors.IsLightOn(DOOR_RIGHT);

    bool powerJustOut = m_power.Tick(cameraUp, leftDoor, rightDoor,
                                         leftLight, rightLight);
    NotifyPowerUpdate();

    if (powerJustOut) {
        // Power is out!
        m_state = GAME_STATE_POWER_OUT;
        m_powerOutTimer = 0.0f;
        m_powerOutDurationSet = false;

        // Force all systems off
        m_doors.ForceDoorsOpen();
        m_doors.ForceLightsOff();
        m_cameras.SetMonitorUp(false);

        if (m_callbacks.onPowerOut) {
            m_callbacks.onPowerOut();
        }
        if (m_callbacks.onMusicBoxStart) {
            m_callbacks.onMusicBoxStart();
        }

        if (m_callbacks.onDoorChange) {
            m_callbacks.onDoorChange(DOOR_LEFT, false);
            m_callbacks.onDoorChange(DOOR_RIGHT, false);
        }
        if (m_callbacks.onLightChange) {
            m_callbacks.onLightChange(DOOR_LEFT, false);
            m_callbacks.onLightChange(DOOR_RIGHT, false);
        }
        if (m_callbacks.onCameraChange) {
            m_callbacks.onCameraChange(CAM_OFF);
        }
        return;
    }

    // 4. Movement opportunity (every ~5 seconds)
    m_ticksSinceLastMovement++;
    if (m_ticksSinceLastMovement >= TimeConstants::MOVEMENT_INTERVAL_TICKS) {
        m_ticksSinceLastMovement = 0;

        AITickResult results[16];
        i32 count = m_ai.OnMovementOpportunity(
            m_doors, m_cameras, m_timer, results, 16);

        // Process results
        for (i32 i = 0; i < count; ++i) {
            if (results[i].event == AI_EVENT_ATTACK) {
                m_state = GAME_STATE_JUMPSCARE;
                m_jumpscareTimer = 0.0f;
                m_jumpscareTriggered = true;
                m_jumpscareAnimatronic = results[i].animatronic;

                if (m_callbacks.onJumpscare) {
                    m_callbacks.onJumpscare(results[i].animatronic);
                }
                return;
            }
            if (results[i].event == AI_EVENT_FOXY_BANG && m_callbacks.onFoxyDoorBang) {
                m_callbacks.onFoxyDoorBang(results[i].powerDrained);
            }
            if (results[i].event == AI_EVENT_MOVED && m_callbacks.onAnimatronicMove) {
                m_callbacks.onAnimatronicMove(results[i].animatronic,
                                                 results[i].newRoom);
            }
            if (results[i].event == AI_EVENT_FOXY_STAGE_UP && m_callbacks.onFoxyStageChange) {
                m_callbacks.onFoxyStageChange(results[i].foxyStage);
            }
        }
    }

    // 5. Per-tick AI updates (attack checks, Foxy running)
    {
        AITickResult results[16];
        i32 count = m_ai.OnTick(
            m_doors, m_cameras, m_power, m_timer, results, 16);

        for (i32 i = 0; i < count; ++i) {
            if (results[i].event == AI_EVENT_ATTACK) {
                m_state = GAME_STATE_JUMPSCARE;
                m_jumpscareTimer = 0.0f;
                m_jumpscareTriggered = true;
                m_jumpscareAnimatronic = results[i].animatronic;

                if (m_callbacks.onJumpscare) {
                    m_callbacks.onJumpscare(results[i].animatronic);
                }
                return;
            }
            if (results[i].event == AI_EVENT_FOXY_BANG && m_callbacks.onFoxyDoorBang) {
                m_callbacks.onFoxyDoorBang(results[i].powerDrained);
            }
            if (results[i].event == AI_EVENT_MOVED && m_callbacks.onAnimatronicMove) {
                m_callbacks.onAnimatronicMove(results[i].animatronic,
                                                 results[i].newRoom);
            }
        }
    }
}


// ============================================================
//  Process: Power Out (Freddy's music box)
// ============================================================

void Game::ProcessPowerOut() {
    // Advance timer
    m_timer.Tick();
    m_powerOutTimer += static_cast<f32>(TimeConstants::TICK_INTERVAL_SEC);

    // Set random duration for Freddy's attack
    if (!m_powerOutDurationSet) {
        m_simpleRNG = m_simpleRNG * 1664525 + 1013904223;
        u32 r = static_cast<u32>(m_simpleRNG);
        f32 range = static_cast<f32>(TimeConstants::POWER_OUT_MAX_SEC - TimeConstants::POWER_OUT_MIN_SEC);
        m_powerOutDuration = TimeConstants::POWER_OUT_MIN_SEC + (static_cast<f32>(r % 10000) / 10000.0f) * range;
        m_powerOutDurationSet = true;
    }

    // Check if 6 AM arrives first (SURVIVAL!)
    if (m_timer.IsNightComplete()) {
        if (m_callbacks.onMusicBoxStop) {
            m_callbacks.onMusicBoxStop();
        }
        m_state = GAME_STATE_NIGHT_COMPLETE;
        m_nightCompleteTimer = 0.0f;
        if (m_callbacks.onNightComplete) {
            m_callbacks.onNightComplete(m_currentNight);
        }
        return;
    }

    // Check if Freddy attacks
    if (m_powerOutTimer >= m_powerOutDuration) {
        if (m_callbacks.onMusicBoxStop) {
            m_callbacks.onMusicBoxStop();
        }
        m_state = GAME_STATE_JUMPSCARE;
        m_jumpscareTimer = 0.0f;
        m_jumpscareTriggered = true;
        m_jumpscareAnimatronic = ANIM_FREDDY;
        if (m_callbacks.onJumpscare) {
            m_callbacks.onJumpscare(ANIM_FREDDY);
        }
    }
}


// ============================================================
//  Process: Jump Scare
// ============================================================

void Game::ProcessJumpscare() {
    m_jumpscareTimer += static_cast<f32>(TimeConstants::TICK_INTERVAL_SEC);

    if (m_jumpscareTimer >= TimeConstants::JUMPSCARE_DURATION_SEC) {
        m_state = GAME_STATE_GAME_OVER;
        if (m_callbacks.onGameOver) {
            m_callbacks.onGameOver();
        }
    }
}


// ============================================================
//  Process: Night Complete (6 AM)
// ============================================================

void Game::ProcessNightComplete() {
    m_nightCompleteTimer += static_cast<f32>(TimeConstants::TICK_INTERVAL_SEC);

    if (m_nightCompleteTimer >= TimeConstants::NIGHT_COMPLETE_DISPLAY_SEC) {
        m_state = GAME_STATE_MENU;
    }
}


// ============================================================
//  Apply AI level changes for a specific hour
// ============================================================

void Game::ApplyAIChangesForHour(i32 hour) {
    if (!m_nightConfig) return;
    if (hour <= m_lastAIHour) return;

    for (i32 c = 0; c < m_nightConfig->changeCount; ++c) {
        const AIChange& change = m_nightConfig->changes[c];
        if (change.hour == hour) {
            for (i32 i = 0; i < ANIM_COUNT; ++i) {
                m_ai.SetAILevel(static_cast<AnimatronicId>(i), change.levels[i]);
            }
        }
    }

    m_lastAIHour = hour;
}


// ============================================================
//  Notification helpers
// ============================================================

void Game::NotifyTimeUpdate() {
    if (m_callbacks.onTimeUpdate) {
        m_callbacks.onTimeUpdate(m_timer.GetHour());
    }
}

void Game::NotifyPowerUpdate() {
    if (m_callbacks.onPowerUpdate) {
        m_callbacks.onPowerUpdate(m_power.GetPower());
    }
}


i32 Game::SimpleRandom(i32 min, i32 max) {
    m_simpleRNG = m_simpleRNG * 1664525 + 1013904223;
    u32 val = static_cast<u32>(m_simpleRNG);
    i32 range = max - min + 1;
    if (range <= 0) return min;
    return min + static_cast<i32>(val % static_cast<u32>(range));
}

} // namespace fnaf
