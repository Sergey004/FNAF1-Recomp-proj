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
    , m_powerOutTimer(0.0f)
    , m_powerOutPhase(0)
    , m_powerOutPhaseTimer(0.0)
    , m_powerOutRollTimer(0.0)
    , m_freddyFaceLit(false)
    , m_faceLitTimer(0.0)
    , m_facePhase(0)
    , m_powerOutBlinkOn(false)
    , m_musicBoxPlaying(false)
    , m_debugGodMode(false)
    , m_feedStaticTicks(0)
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
    m_power.Reset(night);
    m_doors.Reset();
    m_cameras.Reset();
    m_ai.Reset();

    // Set starting AI levels from night config
    if (m_nightConfig) {
        for (i32 i = 0; i < ANIM_COUNT; ++i) {
            m_ai.SetAILevel(static_cast<AnimatronicId>(i),
                             m_nightConfig->startingLevels[i]);
        }
        // Night 4: Freddy starts at 1 + Random(2) (group 308)
        if (m_nightConfig->freddyRandomStart) {
            m_ai.SetAILevel(ANIM_FREDDY, 1 + SimpleRandom(1, 2));
        }
    }

    // Reset state
    m_powerOutTimer = 0.0f;
    m_powerOutPhase = 0;
    m_powerOutPhaseTimer = 0.0;
    m_powerOutRollTimer = 0.0;
    m_freddyFaceLit = false;
    m_faceLitTimer = 0.0;
    m_facePhase = 0;
    m_powerOutBlinkOn = false;
    m_musicBoxPlaying = false;
    m_feedStaticTicks = 0;
    m_nightStartTimer = 0.0f;
    m_jumpscareTimer = 0.0f;
    m_jumpscareTriggered = false;
    m_nightCompleteTimer = 0.0f;

    // Start with night start display
    m_state = GAME_STATE_NIGHT_START;
}


// v2.46: Night 7 reads the Customize screen's four levels (frame 3 group
// 311 copies the global AI counters 141-144 into the activity counters).
void Game::InitCustomNight(const i32 levels[4]) {
    Init(7);
    m_ai.SetAILevel(ANIM_FREDDY, levels[0]);
    m_ai.SetAILevel(ANIM_BONNIE, levels[1]);
    m_ai.SetAILevel(ANIM_CHICA,  levels[2]);
    m_ai.SetAILevel(ANIM_FOXY,   levels[3]);
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
        m_callbacks.onCameraChange(m_cameras.GetCurrentCamera(),
                                   nowUp ? CAM_REASON_UP : CAM_REASON_DOWN);
    }
}

void Game::SetCameraUp(bool up) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    m_cameras.SetMonitorUp(up);

    if (m_callbacks.onCameraChange) {
        m_callbacks.onCameraChange(m_cameras.GetCurrentCamera(),
                                   up ? CAM_REASON_UP : CAM_REASON_DOWN);
    }
}

void Game::SwitchCamera(CameraId cam) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    CameraId prevCam = m_cameras.GetCurrentCamera();
    m_cameras.SwitchCamera(cam);

    // Check if switching to or from Pirate Cove (affects Foxy)
    if (cam == CAM_1C && prevCam != CAM_1C) {
        m_ai.OnCoveLooked();
    }

    if (m_callbacks.onCameraChange) {
        m_callbacks.onCameraChange(m_cameras.GetCurrentCamera(), CAM_REASON_SWITCH);
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

void Game::SetDoorAmount(DoorSide side, float amount) {
    if (m_state != GAME_STATE_PLAYING) return;
    if (m_power.IsPowerOut()) return;

    const bool wasClosed = m_doors.IsDoorClosed(side);
    m_doors.SetDoorAmount(side, amount);
    if (m_callbacks.onDoorChange && wasClosed != m_doors.IsDoorClosed(side)) {
        m_callbacks.onDoorChange(side, m_doors.IsDoorClosed(side));
    }
}

void Game::TickDoors(f32 dt) {
    // v2.22: visual slide only; the AI reads IsDoorClosed() (flipped instantly
    // on toggle), so advancing the position here can't affect gameplay.
    m_doors.Tick(dt);
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
i32  Game::GetPowerOutPhase() const { return m_powerOutPhase; }
bool Game::IsFreddyFaceLit() const { return m_freddyFaceLit; }
i32  Game::GetPowerOutFaceState() const { return m_facePhase; }
bool Game::IsPowerOutBlinkOn() const { return m_powerOutBlinkOn; }
AnimatronicId Game::GetJumpscareAnimatronic() const { return m_jumpscareAnimatronic; }
bool Game::HasJumpscareTriggered() const { return m_jumpscareTriggered; }

// Per-animatronic scare length: the real animation (docs/AI_MECHANICS.md §9)
// plus a short hold on the last frame before the death screen.
f64 Game::GetJumpscareDurationSec() const {
    switch (m_jumpscareAnimatronic) {
        case ANIM_FREDDY: return 2.7;   // 31 frames @ 30 FPS, repeat 1 + hold
        case ANIM_FOXY:   return 2.3;   // 25 frames @ 30 FPS, repeat 1 + hold
        case ANIM_BONNIE: return 1.1;   // 11 frames @ 45 FPS + hold
        case ANIM_CHICA:  return 1.1;   // 16 frames @ 60 FPS + hold
        case ANIM_COUNT:  return 2.5;   // v2.22: Golden Freddy creepy start
        default:          return 1.5;
    }
}

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
        ApplyHourDeltas(newHour);
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

    bool powerJustOut = false;
    if (!m_debugGodMode) {
        powerJustOut = m_power.Tick(cameraUp, leftDoor, rightDoor,
                                    leftLight, rightLight);
        NotifyPowerUpdate();
    }

    if (powerJustOut) {
        // Power is out! The 4-phase dark-office sequence begins;
        // the music box starts with phase 1 (group 272).
        m_state = GAME_STATE_POWER_OUT;
        m_powerOutTimer = 0.0f;
        m_powerOutPhase = 0;
        m_powerOutPhaseTimer = 0.0;
        m_powerOutRollTimer = 0.0;
        m_freddyFaceLit = false;
        m_faceLitTimer = 0.0;
        m_powerOutBlinkOn = false;
        m_musicBoxPlaying = false;

        // Force all systems off
        m_doors.ForceDoorsOpen();
        m_doors.ForceLightsOff();
        m_cameras.SetMonitorUp(false);

        if (m_callbacks.onPowerOut) {
            m_callbacks.onPowerOut();
        }

        if (m_callbacks.onDoorChange) {
            m_callbacks.onDoorChange(DOOR_LEFT, false);
            m_callbacks.onDoorChange(DOOR_RIGHT, false);
        }
        if (m_callbacks.onLightChange) {
            m_callbacks.onLightChange(DOOR_LEFT, false);
            m_callbacks.onLightChange(DOOR_RIGHT, false);
        }
        // v2.22: no onCameraChange(CAM_OFF) here — power-out is a FORCED drop
        // (not a player flip-down), so it must not fire the "put down" sound.
        // OnPowerOut() already stops the static loop, which is all the old
        // CAM_OFF callback did for this path.
        return;
    }

    // 4. Per-tick AI updates — the AI schedules every animatronic's own
    //    movement opportunity internally (Bonnie 4.97s / Chica 4.98s /
    //    Freddy 3.02s / Foxy 5.01s, groups 188-191).
    {
        if (m_feedStaticTicks > 0) m_feedStaticTicks--;
        AITickResult results[16];
        i32 count = m_ai.OnTick(
            m_doors, m_cameras, m_power, m_timer, results, 16);

        for (i32 i = 0; i < count; ++i) {
            if (results[i].event == AI_EVENT_ATTACK) {
                if (m_debugGodMode) continue;   // god mode: ignore the kill (they keep moving)
                // They pull the monitor down on entry (groups 321/322 and
                // the got-you handlers); kill renders with the office view.
                m_cameras.SetMonitorUp(false);
                m_state = GAME_STATE_JUMPSCARE;
                m_jumpscareTimer = 0.0f;
                m_jumpscareTriggered = true;
                m_jumpscareAnimatronic = results[i].animatronic;

                if (m_callbacks.onJumpscare) {
                    m_callbacks.onJumpscare(results[i].animatronic);
                }
                return;
            }
            if (results[i].event == AI_EVENT_FOXY_AT_DOOR) {
                // Foxy force-drops the tablet (groups 321/322)
                m_cameras.SetMonitorUp(false);
            }
            if (results[i].event == AI_EVENT_FREDDY_IN_OFFICE) {
                // v2.46 (groups 406/408-412): Freddy inside the office kills
                // both door lights — his dark-office kill owns the room now.
                m_doors.SetLight(DOOR_LEFT, false);
                m_doors.SetLight(DOOR_RIGHT, false);
            }
            if (results[i].event == AI_EVENT_FOXY_BANG && m_callbacks.onFoxyDoorBang) {
                m_callbacks.onFoxyDoorBang(results[i].powerDrained);
            }
            // v2.46 (groups 194-198): an animatronic moving while its room
            // is on screen glitches the monitor — 300-tick static-out, the
            // feed is hidden ('Active 3' hidden by group 197) and a garble
            // sample plays (groups 219-222, sound hooked in main.cpp).
            if (results[i].event == AI_EVENT_MOVED && m_cameras.IsMonitorUp() &&
                (results[i].animatronic == ANIM_BONNIE ||
                 results[i].animatronic == ANIM_CHICA) &&
                results[i].newRoom != ROOM_LEFT_DOOR &&
                results[i].newRoom != ROOM_RIGHT_DOOR &&
                RoomSystem::GetCameraRoom(m_cameras.GetCurrentCamera()) ==
                    results[i].newRoom) {
                m_feedStaticTicks = 300;
            }
            if (results[i].event == AI_EVENT_MOVED && m_callbacks.onAnimatronicMove) {
                m_callbacks.onAnimatronicMove(results[i].animatronic,
                                                 results[i].newRoom);
            }
            if (results[i].event == AI_EVENT_FOXY_STAGE_UP && m_callbacks.onFoxyStageChange) {
                m_callbacks.onFoxyStageChange(results[i].foxyStage);
            }
            if (results[i].event == AI_EVENT_FOXY_RUNNING && m_callbacks.onFoxyStageChange) {
                m_callbacks.onFoxyStageChange(FOXY_STAGE_4);
            }
        }
    }
}


// ============================================================
//  Process: Power Out — the 4-phase dark office sequence
//  (docs/AI_MECHANICS.md §8; groups 272-302)
//
//   phase 0: dark office; 20%/5s (forced 20 s) -> phase 1
//   phase 1: music box; face flicker 25%/0.5s; 20%/5s -> phase 2
//   phase 2: 20-tick buzz blink -> phase 3
//   phase 3: black; 20%/2s (forced 20 s) -> Freddy kill
//   6 AM saves at any point (the clock keeps running).
// ============================================================

void Game::ProcessPowerOut() {
    // Advance timer — the clock keeps running
    bool hourChanged = m_timer.Tick();
    if (hourChanged) {
        ApplyHourDeltas(m_timer.GetHour());
        NotifyTimeUpdate();
    }
    m_powerOutTimer += static_cast<f32>(TimeConstants::TICK_INTERVAL_SEC);

    // Survival first: 6 AM beats Freddy
    if (m_timer.IsNightComplete()) {
        if (m_musicBoxPlaying && m_callbacks.onMusicBoxStop) {
            m_callbacks.onMusicBoxStop();
        }
        m_musicBoxPlaying = false;
        m_state = GAME_STATE_NIGHT_COMPLETE;
        m_nightCompleteTimer = 0.0f;
        if (m_callbacks.onNightComplete) {
            m_callbacks.onNightComplete(m_currentNight);
        }
        return;
    }

    const f64 dt = TimeConstants::TICK_INTERVAL_SEC;
    m_powerOutPhaseTimer += dt;

    switch (m_powerOutPhase) {
        case 0:
        case 1: {
            // Phase 1: Freddy face flicker — re-roll every 0.5 s, 25 % lit
            if (m_powerOutPhase == 1) {
                m_faceLitTimer += dt;
                if (m_faceLitTimer >= 0.5) {
                    m_faceLitTimer -= 0.5;
                    bool lit = (SimpleRandom(1, 4) == 1);
                    // v2.22: advance the "Active 2" face state on each NEW flash
                    // so the four garble/digital sounds cycle one-per-flash
                    // (groups 219-222).
                    if (lit && !m_freddyFaceLit) m_facePhase = (m_facePhase % 4) + 1;
                    m_freddyFaceLit = lit;
                }
            }

            // 20 % per 5 s (Random(5)+1 == 1, groups 272/291)
            m_powerOutRollTimer += dt;
            bool advance = false;
            if (m_powerOutRollTimer >= TimeConstants::POWER_OUT_PHASE_ROLL_SEC) {
                m_powerOutRollTimer -= TimeConstants::POWER_OUT_PHASE_ROLL_SEC;
                if (SimpleRandom(1, TimeConstants::POWER_OUT_ROLL_DENOM) == 1) advance = true;
            }
            if (m_powerOutPhaseTimer >= TimeConstants::POWER_OUT_PHASE_MAX_SEC) advance = true;

            if (advance) {
                m_powerOutPhase++;
                m_powerOutPhaseTimer = 0.0;
                m_powerOutRollTimer = 0.0;
                m_freddyFaceLit = false;
                m_faceLitTimer = 0.0;

                if (m_powerOutPhase == 1 && !m_musicBoxPlaying) {
                    // Group 272: the music box starts with phase 1
                    m_musicBoxPlaying = true;
                    if (m_callbacks.onMusicBoxStart) m_callbacks.onMusicBoxStart();
                }
                if (m_powerOutPhase == 2) {
                    // Group 293: alterable[7] = Random(2)+1 rolled at entry;
                    // 1 = office stays visible with the buzz, 2 = hidden
                    m_powerOutBlinkOn = (SimpleRandom(1, 2) == 1);
                }
            }
            break;
        }
        case 2: {
            // Phase 2: fixed 20-tick buzz blink (groups 297/298)
            if (m_powerOutPhaseTimer >= TimeConstants::POWER_OUT_BUZZ_TICKS
                                      * TimeConstants::TICK_INTERVAL_SEC) {
                m_powerOutPhase = 3;
                m_powerOutPhaseTimer = 0.0;
                m_powerOutRollTimer = 0.0;
                if (m_musicBoxPlaying && m_callbacks.onMusicBoxStop) {
                    m_callbacks.onMusicBoxStop();
                }
                m_musicBoxPlaying = false;
            }
            break;
        }
        case 3: {
            // Phase 3: 20 % per 2 s (group 301), forced at 20 s (group 302)
            m_powerOutRollTimer += dt;
            bool kill = false;
            if (m_powerOutRollTimer >= TimeConstants::POWER_OUT_FINAL_ROLL_SEC) {
                m_powerOutRollTimer -= TimeConstants::POWER_OUT_FINAL_ROLL_SEC;
                if (SimpleRandom(1, TimeConstants::POWER_OUT_ROLL_DENOM) == 1) kill = true;
            }
            if (m_powerOutPhaseTimer >= TimeConstants::POWER_OUT_PHASE_MAX_SEC) kill = true;

            if (kill) {
                if (m_musicBoxPlaying && m_callbacks.onMusicBoxStop) {
                    m_callbacks.onMusicBoxStop();
                }
                m_musicBoxPlaying = false;
                m_state = GAME_STATE_JUMPSCARE;
                m_jumpscareTimer = 0.0f;
                m_jumpscareTriggered = true;
                m_jumpscareAnimatronic = ANIM_FREDDY;
                if (m_callbacks.onJumpscare) {
                    m_callbacks.onJumpscare(ANIM_FREDDY);
                }
            }
            break;
        }
        default: break;
    }
}


// ============================================================
//  Process: Jump Scare
// ============================================================

void Game::ProcessJumpscare() {
    m_jumpscareTimer += static_cast<f32>(TimeConstants::TICK_INTERVAL_SEC);

    if (m_jumpscareTimer >= GetJumpscareDurationSec()) {
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

// v2.17 DEV: force the current night to finish as if 6 AM just hit. Fires the
// onNightComplete callback so chimes/progress/achievements all happen normally.
void Game::DebugForceNightComplete() {
    m_state = GAME_STATE_NIGHT_COMPLETE;
    m_nightCompleteTimer = 0.0f;
    if (m_callbacks.onNightComplete) {
        m_callbacks.onNightComplete(m_currentNight);
    }
}

void Game::SetDebugGodMode(bool on) {
    m_debugGodMode = on;
    if (on) m_power.Reset(m_currentNight);   // top up power so the night can't end
}

void Game::DebugTriggerPowerOut() {
    m_state = GAME_STATE_POWER_OUT;
    m_powerOutTimer = 0.0f;
    m_powerOutPhase = 0;
    m_powerOutPhaseTimer = 0.0;
    m_powerOutRollTimer = 0.0;
    m_freddyFaceLit = false;
    m_faceLitTimer = 0.0;
    m_facePhase = 0;
    m_powerOutBlinkOn = false;
    m_musicBoxPlaying = false;
    m_doors.ForceDoorsOpen();
    m_doors.ForceLightsOff();
    m_cameras.SetMonitorUp(false);
    if (m_callbacks.onPowerOut) m_callbacks.onPowerOut();
}

void Game::DebugTriggerJumpscare(AnimatronicId anim) {
    m_cameras.SetMonitorUp(false);
    m_state = GAME_STATE_JUMPSCARE;
    m_jumpscareTimer = 0.0f;
    m_jumpscareTriggered = true;
    m_jumpscareAnimatronic = anim;
    if (m_callbacks.onJumpscare) m_callbacks.onJumpscare(anim);
}

void Game::DebugTriggerGoldenFreddy() {
    // v2.22: Golden Freddy's kill reuses the jumpscare state with ANIM_COUNT
    // as the sentinel. The renderer draws img 571 (flat full-screen) for it.
    m_cameras.SetMonitorUp(false);
    m_state = GAME_STATE_JUMPSCARE;
    m_jumpscareTimer = 0.0f;
    m_jumpscareTriggered = true;
    m_jumpscareAnimatronic = ANIM_COUNT;
    if (m_callbacks.onJumpscare) m_callbacks.onJumpscare(ANIM_COUNT);
}


// ============================================================
//  Apply hourly AI deltas (2/3/4 AM — groups 335-337):
//  Bonnie +1 at 2 AM; Bonnie/Chica/Foxy +1 at 3 and 4 AM.
// ============================================================

void Game::ApplyHourDeltas(i32 hour) {
    if (hour <= m_lastAIHour) return;

    for (i32 h = m_lastAIHour + 1; h <= hour; ++h) {
        AILevels cur;
        for (i32 k = 0; k < ANIM_COUNT; ++k) {
            cur[k] = static_cast<i8>(m_ai.GetAILevel(static_cast<AnimatronicId>(k)));
        }
        ApplyHourDelta(h, cur);
        for (i32 k = 0; k < ANIM_COUNT; ++k) {
            m_ai.SetAILevel(static_cast<AnimatronicId>(k), cur[k]);
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
