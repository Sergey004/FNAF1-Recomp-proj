/**
 * Five Nights at Freddy's 1 — Recompilation
 * Animatronic.h: Base animatronic state
 */

#ifndef FNAF_ANIMATRONIC_H
#define FNAF_ANIMATRONIC_H

#include "Types.h"

namespace fnaf {

// ============================================================
//  Animatronic — per-entity state for each of the 4 robots.
//  The actual movement logic is in AnimatronicAI.
//  This struct holds only the runtime state.
// ============================================================

struct Animatronic {
    AnimatronicId id;
    RoomId        currentRoom;
    i32           aiLevel;       // 0-20, set by NightConfig
    bool          active;        // Is this animatronic active this night?
    
    // Foxy-specific state (only used for ANIM_FOXY)
    FoxyStage     foxyStage;     // 0-3: behind curtain → peeking → gone → running
    i32           foxyLookCount; // How many times player has checked Pirate Cove
    i32           foxyRunTimer;  // Ticks since Foxy started running (stage 3)
    bool          foxyRunning;   // Is Foxy currently running down the hall?
    bool          foxyAtDoor;    // Is Foxy at the left door? (after running)
    
    // Tracking whether the animatronic is at the door and
    // waiting for an opportunity to enter the office.
    bool          atDoor;        // At left or right door position
    i32           doorWaitTicks; // Ticks spent waiting at the door
    
    // Has this animatronic already attacked/entered the office?
    bool          hasAttacked;
    
    void Reset(AnimatronicId animId, RoomId startRoom, i32 startAI, bool startActive) {
        id = animId;
        currentRoom = startRoom;
        aiLevel = startAI;
        active = startActive;
        foxyStage = FOXY_STAGE_0;
        foxyLookCount = 0;
        foxyRunTimer = 0;
        foxyRunning = false;
        foxyAtDoor = false;
        atDoor = false;
        doorWaitTicks = 0;
        hasAttacked = false;
    }
};

} // namespace fnaf

#endif // FNAF_ANIMATRONIC_H
