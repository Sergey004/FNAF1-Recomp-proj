/**
 * Five Nights at Freddy's 1 — Recompilation
 * Types.h: Core type definitions, enums, and constants
 * 
 * Compatible with C++11 and Xbox 360 XDK.
 * This file contains ALL fundamental game types used throughout the engine.
 */

#ifndef FNAF_TYPES_H
#define FNAF_TYPES_H

#include <cstdint>

namespace fnaf {

// ============================================================
//  Integer types (platform-independent)
// ============================================================

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

// ============================================================
//  Room identifiers
//  These correspond to the original game's map rooms.
// ============================================================

enum RoomId {
    ROOM_SHOW_STAGE      = 0,  // CAM 1A — Starting position for Freddy, Bonnie, Chica
    ROOM_DINING_AREA     = 1,  // CAM 1B — Central hub connecting many rooms
    ROOM_PIRATE_COVE     = 2,  // CAM 1C — Foxy's starting position
    ROOM_WEST_HALL       = 3,  // CAM 2A — Leads to W. Hall Corner
    ROOM_WEST_HALL_CORNER= 4,  // CAM 2B — Just outside left office door
    ROOM_SUPPLY_CLOSET   = 5,  // CAM 3  — Small room off West Hall
    ROOM_EAST_HALL       = 6,  // CAM 4A — Leads to E. Hall Corner
    ROOM_EAST_HALL_CORNER= 7,  // CAM 4B — Just outside right office door
    ROOM_BACKSTAGE       = 8,  // CAM 5  — Contains spare animatronic heads
    ROOM_KITCHEN         = 9,  // CAM 6  — Audio only (no visual feed)
    ROOM_RESTROOMS       = 10, // CAM 7  — Restroom area
    ROOM_LEFT_DOOR       = 11, // Just outside the left office door (Bonnie/Foxy attack position)
    ROOM_RIGHT_DOOR      = 12, // Just outside the right office door (Chica/Freddy attack position)
    ROOM_OFFICE          = 13, // Inside the office — game over if animatronic enters
    ROOM_NONE            = 255 // Animatronic is inactive / not on the map
};

// ============================================================
//  Camera identifiers
//  11 cameras total, matching the original game.
// ============================================================

enum CameraId {
    CAM_1A = 0,  // Show Stage
    CAM_1B = 1,  // Dining Area
    CAM_1C = 2,  // Pirate Cove
    CAM_2A = 3,  // West Hall
    CAM_2B = 4,  // W. Hall Corner
    CAM_3  = 5,  // Supply Closet
    CAM_4A = 6,  // East Hall
    CAM_4B = 7,  // E. Hall Corner
    CAM_5  = 8,  // Backstage
    CAM_6  = 9,  // Kitchen (audio only)
    CAM_7  = 10, // Restrooms
    CAM_OFF = 255 // Camera monitor is down
};

// ============================================================
//  Animatronic identifiers
// ============================================================

enum AnimatronicId {
    ANIM_FREDDY = 0,
    ANIM_BONNIE = 1,
    ANIM_CHICA  = 2,
    ANIM_FOXY   = 3,
    ANIM_COUNT  = 4
};

// ============================================================
//  Foxy's pirate cove stages
//  Foxy has a unique 4-stage state machine.
// ============================================================

enum FoxyStage {
    FOXY_STAGE_0 = 0, // Behind curtain — curtain fully closed
    FOXY_STAGE_1 = 1, // Peeking out — curtain partially open, one eye visible
    FOXY_STAGE_2 = 2, // Out of view — curtain wide open, Foxy gone from cove
    FOXY_STAGE_3 = 3  // Running — Foxy is sprinting down the West Hall
};

// ============================================================
//  Game state machine
// ============================================================

enum GameState {
    GAME_STATE_MENU          = 0,
    GAME_STATE_NIGHT_START   = 1, // Brief "12 AM" title card
    GAME_STATE_PLAYING       = 2, // Main gameplay
    GAME_STATE_POWER_OUT     = 3, // Power depleted, Freddy's music box
    GAME_STATE_JUMPSCARE     = 4, // Animatronic jump scare
    GAME_STATE_NIGHT_COMPLETE= 5, // "6 AM" victory screen
    GAME_STATE_GAME_OVER     = 6, // "Game Over" screen
    GAME_STATE_STATIC        = 7, // Camera static / transition
    GAME_STATE_DISCLAIMER    = 8  // Boot warning screen (title frame String obj 0)
};

// ============================================================
//  Door and light states
// ============================================================

enum DoorSide {
    DOOR_LEFT  = 0,
    DOOR_RIGHT = 1,
    DOOR_COUNT = 2
};

// ============================================================
//  Game timing constants (matching original FNAF 1)
// ============================================================

namespace TimeConstants {
    // Original game runs at approximately 60 FPS.
    // One in-game "hour" lasts about 89 seconds of real time.
    // Total night duration: 6 hours x 89s = 534 seconds (~8 min 54 sec).
    
    static const f64 TICK_RATE           = 60.0;  // Logic updates per second
    static const f64 HOUR_DURATION_SEC   = 89.0;  // Real seconds per in-game hour
    static const f64 NIGHT_DURATION_SEC  = 6.0 * HOUR_DURATION_SEC; // 534s total
    static const f64 TICK_INTERVAL_SEC   = 1.0 / TICK_RATE; // ~0.0167s per tick
    
    // Movement opportunity occurs every ~5 seconds (300 frames at 60 FPS).
    // This is the interval at which each animatronic gets a chance to move.
    static const f64 MOVEMENT_INTERVAL_SEC  = 4.96;  // ~5 seconds
    static const i32  MOVEMENT_INTERVAL_TICKS = 298;   // ~300 ticks
    
    // Power-out music box duration (randomized)
    static const f64 POWER_OUT_MIN_SEC = 5.0;
    static const f64 POWER_OUT_MAX_SEC = 20.0;
    
    // Night start title card duration
    static const f64 NIGHT_START_DISPLAY_SEC = 3.0;
    
    // Jump scare duration
    static const f64 JUMPSCARE_DURATION_SEC = 1.5;
    
    // 6 AM celebration display duration
    static const f64 NIGHT_COMPLETE_DISPLAY_SEC = 5.0;
}

// ============================================================
//  Power system constants
// ============================================================

namespace PowerConstants {
    // Power starts at 100% on all nights.
    // Drain rate depends on the number of active systems.
    // Each "usage bar" represents a different drain multiplier.
    
    // Base drain: 1 usage level (always active)
    // Camera up: +1 usage
    // Each door closed: +1 usage
    // Each light on: +1 usage (lights drain very briefly while held)
    
    static const f32 BASE_DRAIN_RATE = 1.0f;  // 1 bar per base interval
    
    // Power drain per second at usage level 1 (base only, nothing else on)
    // Calibrated so power lasts ~534 seconds at usage 1.
    // Drain per second = 100% / (534 * usage_factor)
    // At usage 1: 100/534 ≈ 0.187% per second
    // At usage 2: 100/(534/2) ≈ 0.374% per second (drains twice as fast)
    // At usage 5: drains 5x faster
    
    // The original game uses: power -= usage_level per tick (scaled)
    // Total ticks at 60fps for full night: 534 * 60 = 32040 ticks
    // Power percent = 100, so drain per tick at usage 1 = 100/32040 ≈ 0.00312%
    
    static const f32 TOTAL_NIGHT_TICKS = 32040.0f; // 534s * 60fps
    static const f32 DRAIN_PER_TICK_USAGE1 = 100.0f / TOTAL_NIGHT_TICKS; // ~0.00312%
    
    // Foxy power drain when he bangs on a closed door
    // First bang: 1%, second: 5%, third: 10%
    static const f32 FOXY_DRAIN_1 = 1.0f;
    static const f32 FOXY_DRAIN_2 = 5.0f;
    static const f32 FOXY_DRAIN_3 = 10.0f;
}

// ============================================================
//  AI system constants
// ============================================================

namespace AIConstants {
    // Each animatronic has an AI level from 0 to 20.
    // On each movement opportunity, a random integer in [1, 20] is generated.
    // If random <= AI_level, the animatronic moves.
    // At AI 0, the animatronic never moves (0% chance).
    // At AI 20, the animatronic always moves (100% chance).
    // At AI 1,  1/20 = 5% chance to move.
    // At AI 10, 10/20 = 50% chance to move.
    
    static const i32 AI_LEVEL_MIN = 0;
    static const i32 AI_LEVEL_MAX = 20;
    static const i32 AI_ROLL_MAX  = 20; // Random roll range is [1, 20]
    
    // Foxy-specific: how many movement opportunities of NOT being checked
    // before Foxy advances a stage. Lower AI = more opportunities needed.
    // At AI 1: Foxy needs ~10 unchecked opportunities to reach stage 1
    // At AI 20: Foxy advances very quickly
    static const i32 FOXY_STAGE_ADVANCE_BASE = 3; // Base opportunities needed at AI 20
}

// ============================================================
//  Night configuration
// ============================================================

static const i32 NIGHT_COUNT = 7; // Nights 1-7

// ============================================================
//  Event callbacks (for platform integration)
//  These allow the game logic to notify the engine/renderer
//  about events without coupling to any specific platform.
// ============================================================

struct GameCallbacks {
    // Called when the player should see a jump scare.
    // Parameter: which animatronic is jump-scaring.
    void (*onJumpscare)(AnimatronicId animatronic);
    
    // Called when power runs out.
    void (*onPowerOut)();
    
    // Called when Freddy's music box should play during power-out.
    void (*onMusicBoxStart)();
    
    // Called when Freddy's music box stops (either 6 AM or jump scare).
    void (*onMusicBoxStop)();
    
    // Called when night is completed (6 AM reached).
    // Parameter: which night was completed.
    void (*onNightComplete)(i32 night);
    
    // Called when game over occurs.
    void (*onGameOver)();
    
    // Called when camera view changes.
    // Parameter: which camera is now active (CAM_OFF if monitor is down).
    void (*onCameraChange)(CameraId camera);
    
    // Called when an animatronic moves to a new room.
    // Parameters: animatronic ID, new room ID.
    void (*onAnimatronicMove)(AnimatronicId animatronic, RoomId room);
    
    // Called when Foxy advances his pirate cove stage.
    // Parameter: new stage (0-3).
    void (*onFoxyStageChange)(FoxyStage stage);
    
    // Called when Foxy bangs on the left door.
    // Parameter: power drained.
    void (*onFoxyDoorBang)(f32 powerDrained);
    
    // Called when the office time display should update.
    // Parameter: current hour (0=12AM, 1=1AM, ..., 5=5AM, 6=6AM).
    void (*onTimeUpdate)(i32 hour);
    
    // Called when power percentage changes.
    // Parameter: current power (0.0 - 100.0).
    void (*onPowerUpdate)(f32 power);
    
    // Called when a door state changes.
    // Parameters: door side, is_closed.
    void (*onDoorChange)(DoorSide side, bool closed);
    
    // Called when a light state changes.
    // Parameters: door side, is_on.
    void (*onLightChange)(DoorSide side, bool on);
    
    // Optional: RNG function. If null, a default LCG is used.
    // Should return a random integer in [min, max] inclusive.
    int (*randomRange)(int min, int max);
    
    // Optional: get current time in seconds (for delta time).
    // If null, tick-based timing is used.
    double (*getCurrentTime)();
};

// Default null callbacks struct for initialization.
inline GameCallbacks MakeNullCallbacks() {
    GameCallbacks cb;
    cb.onJumpscare          = 0;
    cb.onPowerOut           = 0;
    cb.onMusicBoxStart      = 0;
    cb.onMusicBoxStop       = 0;
    cb.onNightComplete      = 0;
    cb.onGameOver           = 0;
    cb.onCameraChange       = 0;
    cb.onAnimatronicMove    = 0;
    cb.onFoxyStageChange    = 0;
    cb.onFoxyDoorBang       = 0;
    cb.onTimeUpdate         = 0;
    cb.onPowerUpdate        = 0;
    cb.onDoorChange         = 0;
    cb.onLightChange        = 0;
    cb.randomRange          = 0;
    cb.getCurrentTime       = 0;
    return cb;
}

} // namespace fnaf

#endif // FNAF_TYPES_H
