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
    // v2.48: ANIM_COUNT doubles as Golden Freddy's sentinel; everything above
    // it is a scare-only id (never index AI tables with these).
    ANIM_COUNT  = 4,
    ANIM_FREDDY_DARK = 5    // power-out kill: the dark face flicker (dark "freddy" frame)
};

// ============================================================
//  Foxy's cove stages
//  Foxy has a unique 6-stage state machine (0-5).
// ============================================================

enum FoxyStage {
    FOXY_STAGE_0 = 0, // Behind curtain — curtain fully closed
    FOXY_STAGE_1 = 1, // Peeking out — curtain partially open
    FOXY_STAGE_2 = 2, // Out of view — curtain wide open, Foxy gone from cove
    FOXY_STAGE_3 = 3, // Lurking — gone from cove, approaching (25 s or on sight)
    FOXY_STAGE_4 = 4, // Running — sprinting down the West Hall (1.67 s)
    FOXY_STAGE_5 = 5  // At the left door — bangs or attacks
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
    GAME_STATE_DISCLAIMER    = 8, // Boot warning screen (title frame String obj 0)
    GAME_STATE_INTRO_AD      = 9, // v2.7.13 "HELP WANTED" newspaper (frame "ad", New Game)
    GAME_STATE_CUSTOMIZE     = 10 // v2.46 Night 7 setup (frame "customize", groups 5-17)
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
    // Verified against the original event dump (docs/AI_MECHANICS.md):
    // CF2.5 frameRate = 60, minute counter >= 90 -> hour++, 6 hours per night.
    static const f64 TICK_RATE           = 60.0;  // Logic updates per second
    static const f64 HOUR_DURATION_SEC   = 90.0;  // Real seconds per in-game hour (group 264-265)
    // v2.53: the dump's minute counter RESETS TO 1 past 90 (group 265) —
    // hour 1 lasts 90 s, every later hour 89 s; the night is 90+89*5 = 535 s.
    static const f64 NIGHT_DURATION_SEC  = 535.0;
    static const f64 TICK_INTERVAL_SEC   = 1.0 / TICK_RATE; // ~0.0167s per tick
    
    // Movement opportunities: each animatronic has its OWN interval
    // (groups 188-191, Timer conditions 4970/4980/3020/5010 centiseconds).
    static const f64 BONNIE_MOVE_INTERVAL_SEC = 4.97;
    static const f64 CHICA_MOVE_INTERVAL_SEC  = 4.98;
    static const f64 FREDDY_MOVE_INTERVAL_SEC = 3.02;
    static const f64 FOXY_MOVE_INTERVAL_SEC   = 5.01;
    
    // Foxy timings (60 FPS frames, groups 316-320, 329, 313)
    static const i32 FOXY_RUN_TICKS          = 100;   // stage 4 -> at door (group 317: >100)
    static const i32 FOXY_LURK_TICKS         = 1500;  // stage 3 -> at door (group 320: >1500)
    static const i32 FOXY_COOLDOWN_MIN_TICKS = 50;    // tablet cooldown (group 329)
    static const i32 FOXY_COOLDOWN_RAND      = 1000;
    
    // Freddy: door kill chance 25%/s (group 406: Random(4)==1, Timer 1000)
    static const i32 FREDDY_DOOR_KILL_DENOM = 4;
    
    // Night start title card duration
    static const f64 NIGHT_START_DISPLAY_SEC = 3.0;
    
    // 6 AM celebration display duration
    static const f64 NIGHT_COMPLETE_DISPLAY_SEC = 5.0;
    
    // Power-out phase roll chances (groups 272/291: Random(5)+1==1)
    static const i32 POWER_OUT_ROLL_DENOM = 5;
    static const f64 POWER_OUT_PHASE_MAX_SEC = 20.0;
    static const f64 POWER_OUT_PHASE_ROLL_SEC = 5.0;   // phases 0/1: roll every 5 s
    // v2.49: after the jingle ends, Freddy's steps approach before the dark
    // kill (wiki detail, user-picked; the dump has a black gap there instead)
    static const f64 POWER_OUT_STEPS_SEC = 1.5;
}

// ============================================================
//  Power system constants
// ============================================================

namespace PowerConstants {
    // Verified against the original (groups 175-177, 342-345, 324):
    // "power left" is stored in TENTHS of a percent and starts at 999.
    // Every 1 second: power -= usage, where usage = 1..5 (1 + monitor +
    // 2 doors + 2 lights). Extra per-night drains: N2 -1/6s, N3 -1/5s,
    // N4 -1/4s, N5+ -1/3s.
    static const i32 POWER_START_TENTHS = 999;
    
    // Foxy door bang: (10 + 50*bangCount) tenths = 1% + 5% per previous bang
    // (group 324)
    static const i32 FOXY_BANG_BASE_TENTHS  = 10;
    static const i32 FOXY_BANG_SCALE_TENTHS = 50;
}

// ============================================================
//  AI system constants
// ============================================================

namespace AIConstants {
    // Roll: Random(20)+1 <= AI level (groups 188-191).
    static const i32 AI_LEVEL_MIN = 0;
    static const i32 AI_LEVEL_MAX = 20;
    static const i32 AI_ROLL_MAX  = 20;
    
    // Freddy movement delay after a successful opportunity:
    // counter must reach (1000 - AI*100) ticks with the monitor down
    // (groups 397-398).
    static const i32 FREDDY_DELAY_BASE = 1000;
    static const i32 FREDDY_DELAY_PER_AI = 100;
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

// v2.22: why a camera transition fired — lets the audio layer play the right
// 1:1 sound (flip-up whir vs. camera-switch blip vs. put-down).
enum CameraChangeReason {
    CAM_REASON_DOWN   = 0,   // monitor flipped down (or forced off)
    CAM_REASON_UP     = 1,   // monitor flipped up
    CAM_REASON_SWITCH = 2    // switched between cameras while up
};

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

    // v2.49: between the jingle and the power-out kill — Freddy's footsteps
    // are heard approaching (the "warn the player" cue).
    void (*onPowerOutSteps)();
    
    // Called when night is completed (6 AM reached).
    // Parameter: which night was completed.
    void (*onNightComplete)(i32 night);
    
    // Called when game over occurs.
    void (*onGameOver)();
    
    // Called when camera view changes.
    // Parameters: which camera is now active (CAM_OFF if monitor is down),
    // and the reason (CAM_REASON_*).
    void (*onCameraChange)(CameraId camera, int reason);
    
    // Called when an animatronic moves to a new room.
    // Parameters: animatronic ID, new room ID.
    void (*onAnimatronicMove)(AnimatronicId animatronic, RoomId room);
    
    // Called when Foxy advances his pirate cove stage.
    // Parameter: new stage (0-5, see FoxyStage).
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
    cb.onPowerOutSteps      = 0;
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
