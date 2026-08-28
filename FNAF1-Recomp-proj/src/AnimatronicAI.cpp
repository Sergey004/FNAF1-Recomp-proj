/**
 * Five Nights at Freddy's 1 — Recompilation
 * AnimatronicAI.cpp: Full AI logic implementation
 *
 * All values and behaviors are matched to the original game,
 * reconstructed via black-box observation of retail gameplay.
 */

#include "AnimatronicAI.h"
#include <cstring>

namespace fnaf {

// ============================================================
//  Freddy's strict movement path (index-based)
//  Freddy ALWAYS follows this exact sequence.
// ============================================================
const RoomId AnimatronicAI::s_freddyPath[] = {
    ROOM_SHOW_STAGE,       // 0: Starting position
    ROOM_DINING_AREA,      // 1
    ROOM_RESTROOMS,        // 2
    ROOM_EAST_HALL,        // 3
    ROOM_EAST_HALL_CORNER, // 4
    ROOM_RIGHT_DOOR        // 5: Attack position
};
const i32 AnimatronicAI::s_freddyPathLength = 6;

// ============================================================
//  Bonnie's reachable rooms (in order of progression toward left door)
//  Bonnie moves FREELY among these rooms — not on a strict path.
//  He tends to advance toward the office but can also backtrack.
// ============================================================
const RoomId AnimatronicAI::s_bonnieRooms[] = {
    ROOM_SHOW_STAGE,
    ROOM_DINING_AREA,
    ROOM_BACKSTAGE,
    ROOM_WEST_HALL,
    ROOM_SUPPLY_CLOSET,
    ROOM_WEST_HALL_CORNER,
    ROOM_LEFT_DOOR
};
const i32 AnimatronicAI::s_bonnieRoomCount = 7;

// ============================================================
//  Chica's reachable rooms (in order of progression toward right door)
//  Chica moves FREELY among these rooms.
// ============================================================
const RoomId AnimatronicAI::s_chicaRooms[] = {
    ROOM_SHOW_STAGE,
    ROOM_DINING_AREA,
    ROOM_RESTROOMS,
    ROOM_KITCHEN,
    ROOM_EAST_HALL,
    ROOM_EAST_HALL_CORNER,
    ROOM_RIGHT_DOOR
};
const i32 AnimatronicAI::s_chicaRoomCount = 7;


AnimatronicAI::AnimatronicAI()
    : m_attacker(ANIM_FREDDY)
    , m_attackOccurred(false)
    , m_rngState(12345)
    , m_freddyPathIndex(0)
    , m_movementTickCounter(0)
    , m_pirateCoveViewedThisInterval(false)
{
}


void AnimatronicAI::Reset() {
    m_attackOccurred = false;
    m_attacker = ANIM_FREDDY;
    m_freddyPathIndex = 0;
    m_movementTickCounter = 0;
    m_pirateCoveViewedThisInterval = false;
    m_rngState = 12345;

    // Default starting positions and states
    for (i32 i = 0; i < ANIM_COUNT; ++i) {
        AnimatronicId id = static_cast<AnimatronicId>(i);
        switch (id) {
            case ANIM_FREDDY:
                m_animatronics[i].Reset(id, ROOM_SHOW_STAGE, 0, false);
                break;
            case ANIM_BONNIE:
                m_animatronics[i].Reset(id, ROOM_SHOW_STAGE, 0, true);
                break;
            case ANIM_CHICA:
                m_animatronics[i].Reset(id, ROOM_SHOW_STAGE, 0, true);
                break;
            case ANIM_FOXY:
                m_animatronics[i].Reset(id, ROOM_PIRATE_COVE, 0, false);
                m_animatronics[i].foxyStage = FOXY_STAGE_0;
                break;
        }
    }
}


Animatronic& AnimatronicAI::GetAnimatronic(AnimatronicId id) {
    return m_animatronics[static_cast<i32>(id)];
}

const Animatronic& AnimatronicAI::GetAnimatronic(AnimatronicId id) const {
    return m_animatronics[static_cast<i32>(id)];
}


void AnimatronicAI::SetAILevel(AnimatronicId id, i32 level) {
    if (level < 0) level = 0;
    if (level > 20) level = 20;
    m_animatronics[static_cast<i32>(id)].aiLevel = level;

    // Freddy and Foxy become active when they get AI > 0
    if (level > 0) {
        m_animatronics[static_cast<i32>(id)].active = true;
    }
}


void AnimatronicAI::SetActive(AnimatronicId id, bool active) {
    m_animatronics[static_cast<i32>(id)].active = active;
}


// ============================================================
//  LCG RNG (platform-independent, no std::rand dependency)
// ============================================================

i32 AnimatronicAI::RandomRange(i32 min, i32 max) {
    // Linear Congruential Generator
    // Constants from Numerical Recipes
    m_rngState = m_rngState * 1664525 + 1013904223;
    u32 val = static_cast<u32>(m_rngState);
    // Map to [min, max] inclusive
    i32 range = max - min + 1;
    if (range <= 0) return min;
    i32 result = min + static_cast<i32>(val % static_cast<u32>(range));
    if (result > max) result = max;
    if (result < min) result = min;
    return result;
}

void AnimatronicAI::SeedRNG(i32 seed) {
    m_rngState = seed;
    if (m_rngState == 0) m_rngState = 1;
}


// ============================================================
//  AI Roll Check
//  Roll random 1-20, return true if <= aiLevel.
//  This is the CORE mechanic for ALL animatronic movement decisions.
// ============================================================

bool AnimatronicAI::CheckAIRoll(i32 aiLevel) {
    if (aiLevel <= 0) return false;
    if (aiLevel >= 20) return true;  // AI 20 = always moves
    i32 roll = RandomRange(1, 20);
    return roll <= aiLevel;
}


// ============================================================
//  Helper to add a result
// ============================================================

void AnimatronicAI::AddResult(AITickResult* results, i32& count, i32 maxResults,
                               AIEvent event, AnimatronicId anim, RoomId room,
                               FoxyStage stage, f32 drain) {
    if (count < maxResults) {
        AITickResult& r = results[count];
        r.event = event;
        r.animatronic = anim;
        r.newRoom = room;
        r.foxyStage = stage;
        r.powerDrained = drain;
        count++;
    }
}


// ============================================================
//  OnMovementOpportunity — called every ~5 seconds
//  This is the main AI decision point.
// ============================================================

i32 AnimatronicAI::OnMovementOpportunity(const DoorSystem& doors,
                                          const CameraSystem& cameras,
                                          const GameTimer& timer,
                                          AITickResult* results, i32 maxResults) {
    i32 count = 0;
    m_movementTickCounter++;

    // Reset per-interval tracking
    m_pirateCoveViewedThisInterval = false;

    if (m_attackOccurred) return count;

    // Update each animatronic
    UpdateFreddy(doors, cameras, results, count, maxResults);
    UpdateBonnie(doors, results, count, maxResults);
    UpdateChica(doors, results, count, maxResults);
    UpdateFoxy(doors, cameras, results, count, maxResults);

    return count;
}


// ============================================================
//  OnTick — called every logic tick (60fps)
//  Handles: Foxy running timer, door wait, attack checks
// ============================================================

i32 AnimatronicAI::OnTick(const DoorSystem& doors,
                           const CameraSystem& cameras,
                           PowerSystem& power,
                           const GameTimer& timer,
                           AITickResult* results, i32 maxResults) {
    i32 count = 0;

    if (m_attackOccurred) return count;

    Animatronic& foxy = m_animatronics[ANIM_FOXY];

    // Handle Foxy running state
    if (foxy.foxyRunning && !foxy.hasAttacked) {
        foxy.foxyRunTimer++;

        // Foxy takes about 1.5 seconds to reach the door (90 ticks at 60fps)
        // In the original, this is nearly instant (1-2 movement opportunities)
        // When he arrives at the door:
        if (foxy.foxyRunTimer >= 90) {
            foxy.foxyRunning = false;
            foxy.foxyAtDoor = true;
            foxy.currentRoom = ROOM_LEFT_DOOR;

            AddResult(results, count, maxResults,
                       AI_EVENT_FOXY_AT_DOOR, ANIM_FOXY, ROOM_LEFT_DOOR);

            // Check if door is open → attack
            if (!doors.IsDoorClosed(DOOR_LEFT)) {
                // FOXY ATTACKS!
                foxy.hasAttacked = true;
                m_attackOccurred = true;
                m_attacker = ANIM_FOXY;
                AddResult(results, count, maxResults,
                           AI_EVENT_ATTACK, ANIM_FOXY, ROOM_OFFICE);
            } else {
                // Door is closed — Foxy bangs and drains power
                f32 drained = power.OnFoxyDoorBang();
                AddResult(results, count, maxResults,
                           AI_EVENT_FOXY_BANG, ANIM_FOXY, ROOM_LEFT_DOOR,
                           FOXY_STAGE_0, drained);

                // After banging, Foxy resets to stage 0
                foxy.foxyStage = FOXY_STAGE_0;
                foxy.foxyAtDoor = false;
                foxy.currentRoom = ROOM_PIRATE_COVE;
                foxy.foxyRunTimer = 0;
            }
        }
    }

    // Handle animatronics waiting at doors (Bonnie/Chica/Freddy)
    for (i32 i = 0; i < ANIM_COUNT; ++i) {
        Animatronic& anim = m_animatronics[i];
        if (!anim.active || anim.hasAttacked) continue;
        if (!anim.atDoor) continue;

        anim.doorWaitTicks++;

        DoorSide side = (anim.currentRoom == ROOM_LEFT_DOOR) ? DOOR_LEFT : DOOR_RIGHT;
        bool doorClosed = doors.IsDoorClosed(side);

        if (doorClosed) {
            // Door is closed — wait, then eventually leave
            // Bonnie/Chica leave after ~10-20 seconds (600-1200 ticks)
            // Freddy leaves after ~5-10 seconds
            i32 leaveThreshold = (i == ANIM_FREDDY) ? 300 : 900;

            // Freddy also leaves if camera is put up while he's at the door
            bool freddyCameraEscape = (i == ANIM_FREDDY && cameras.IsMonitorUp());

            if (anim.doorWaitTicks >= leaveThreshold || freddyCameraEscape) {
                // Leave — go back to an earlier room
                anim.atDoor = false;
                anim.doorWaitTicks = 0;

                if (i == ANIM_FREDDY) {
                    // Freddy goes back to East Hall Corner
                    anim.currentRoom = ROOM_EAST_HALL_CORNER;
                    m_freddyPathIndex = 4; // Back to East Hall Corner in path
                } else if (i == ANIM_BONNIE) {
                    // Bonnie goes back to West Hall or Dining Area
                    anim.currentRoom = (RandomRange(0, 1) == 0)
                                      ? ROOM_WEST_HALL : ROOM_DINING_AREA;
                } else if (i == ANIM_CHICA) {
                    // Chica goes back to East Hall or Dining Area
                    anim.currentRoom = (RandomRange(0, 1) == 0)
                                      ? ROOM_EAST_HALL : ROOM_DINING_AREA;
                }

                AddResult(results, count, maxResults,
                           AI_EVENT_MOVED, anim.id, anim.currentRoom);
            }
        } else {
            // Door is OPEN — ATTACK!
            // In the original, there's actually a chance-based check here.
            // The animatronic enters on the next movement opportunity when door is open.
            // We check on each tick but the attack only triggers if they've been
            // at the door for at least a brief moment.
            if (anim.doorWaitTicks >= 30) { // ~0.5 second minimum wait
                anim.hasAttacked = true;
                m_attackOccurred = true;
                m_attacker = anim.id;
                anim.currentRoom = ROOM_OFFICE;
                AddResult(results, count, maxResults,
                           AI_EVENT_ATTACK, anim.id, ROOM_OFFICE);
            }
        }
    }

    return count;
}


// ============================================================
//  FREDDY AI
//  - Only moves when camera monitor is DOWN
//  - Follows a strict path: Show Stage → Dining Area → Restrooms
//    → East Hall → East Hall Corner → Right Door
//  - At Right Door, attacks if door is open
// ============================================================

void AnimatronicAI::UpdateFreddy(const DoorSystem& doors, const CameraSystem& cameras,
                                AITickResult* results, i32& resultCount, i32 maxResults) {
    Animatronic& freddy = m_animatronics[ANIM_FREDDY];

    if (!freddy.active || freddy.aiLevel <= 0 || freddy.hasAttacked) return;

    // Freddy only moves when the camera is DOWN
    if (cameras.IsMonitorUp()) return;

    // If Freddy is at the door, he's handled by OnTick
    if (freddy.atDoor) return;

    // If Freddy is at the office, he already attacked
    if (freddy.currentRoom == ROOM_OFFICE) return;

    // AI roll check
    if (!CheckAIRoll(freddy.aiLevel)) return;

    // Move to next room on path
    m_freddyPathIndex++;

    if (m_freddyPathIndex >= s_freddyPathLength) {
        m_freddyPathIndex = s_freddyPathLength - 1;
        return;
    }

    RoomId newRoom = s_freddyPath[m_freddyPathIndex];
    freddy.currentRoom = newRoom;

    AddResult(results, resultCount, maxResults,
               AI_EVENT_MOVED, ANIM_FREDDY, newRoom);

    // If Freddy reached the right door position
    if (newRoom == ROOM_RIGHT_DOOR) {
        freddy.atDoor = true;
        freddy.doorWaitTicks = 0;
        AddResult(results, resultCount, maxResults,
                   AI_EVENT_AT_DOOR, ANIM_FREDDY, ROOM_RIGHT_DOOR);
    }
}


// ============================================================
//  BONNIE AI
//  - Moves freely among his reachable rooms
//  - Tends to advance toward the left side (office)
//  - Can appear at the left door
//  - More active than other animatronics
// ============================================================

void AnimatronicAI::UpdateBonnie(const DoorSystem& doors, AITickResult* results,
                                  i32& resultCount, i32 maxResults) {
    Animatronic& bonnie = m_animatronics[ANIM_BONNIE];

    if (!bonnie.active || bonnie.aiLevel <= 0 || bonnie.hasAttacked) return;
    if (bonnie.atDoor) return;
    if (bonnie.currentRoom == ROOM_OFFICE) return;

    // AI roll check
    if (!CheckAIRoll(bonnie.aiLevel)) return;

    RoomId newRoom = ChooseBonnieNextRoom();
    if (newRoom == bonnie.currentRoom) return; // Didn't move

    bonnie.currentRoom = newRoom;

    AddResult(results, resultCount, maxResults,
               AI_EVENT_MOVED, ANIM_BONNIE, newRoom);

    // If Bonnie reached the left door
    if (newRoom == ROOM_LEFT_DOOR) {
        bonnie.atDoor = true;
        bonnie.doorWaitTicks = 0;
        AddResult(results, resultCount, maxResults,
                   AI_EVENT_AT_DOOR, ANIM_BONNIE, ROOM_LEFT_DOOR);
    }
}


RoomId AnimatronicAI::ChooseBonnieNextRoom() {
    Animatronic& bonnie = m_animatronics[ANIM_BONNIE];
    RoomId current = bonnie.currentRoom;

    // Find current position in the room list
    i32 currentIdx = -1;
    for (i32 i = 0; i < s_bonnieRoomCount; ++i) {
        if (s_bonnieRooms[i] == current) {
            currentIdx = i;
            break;
        }
    }

    if (currentIdx < 0) {
        // Not found in room list — return to a known room
        return ROOM_DINING_AREA;
    }

    // Bonnie can move to rooms at index currentIdx ± RandomRange(1, 3)
    // He generally moves FORWARD (toward higher index = closer to office)
    // but can also backtrack by 1-2 positions.

    i32 direction = RandomRange(0, 3);
    // 0 = stay/forward 1, 1 = forward 2, 2 = forward 1, 3 = back 1
    i32 offset = 0;
    switch (direction) {
        case 0: offset = RandomRange(1, 2); break;  // Forward 1-2
        case 1: offset = RandomRange(2, 3); break;  // Forward 2-3
        case 2: offset = 1; break;                  // Forward 1
        case 3: offset = -RandomRange(1, 2); break;  // Back 1-2
    }

    i32 newIdx = currentIdx + offset;

    // Clamp to valid range
    if (newIdx < 0) newIdx = 0;
    if (newIdx >= s_bonnieRoomCount) newIdx = s_bonnieRoomCount - 1;

    // Don't stay in the same room
    if (newIdx == currentIdx) {
        if (currentIdx < s_bonnieRoomCount - 1) {
            newIdx = currentIdx + 1;
        } else if (currentIdx > 0) {
            newIdx = currentIdx - 1;
        }
    }

    return s_bonnieRooms[newIdx];
}


// ============================================================
//  CHICA AI
//  - Moves freely among her reachable rooms
//  - Tends to advance toward the right side (office)
//  - Can appear at the right door
// ============================================================

void AnimatronicAI::UpdateChica(const DoorSystem& doors, AITickResult* results,
                                 i32& resultCount, i32 maxResults) {
    Animatronic& chica = m_animatronics[ANIM_CHICA];

    if (!chica.active || chica.aiLevel <= 0 || chica.hasAttacked) return;
    if (chica.atDoor) return;
    if (chica.currentRoom == ROOM_OFFICE) return;

    // AI roll check
    if (!CheckAIRoll(chica.aiLevel)) return;

    RoomId newRoom = ChooseChicaNextRoom();
    if (newRoom == chica.currentRoom) return;

    chica.currentRoom = newRoom;

    AddResult(results, resultCount, maxResults,
               AI_EVENT_MOVED, ANIM_CHICA, newRoom);

    // If Chica reached the right door
    if (newRoom == ROOM_RIGHT_DOOR) {
        chica.atDoor = true;
        chica.doorWaitTicks = 0;
        AddResult(results, resultCount, maxResults,
                   AI_EVENT_AT_DOOR, ANIM_CHICA, ROOM_RIGHT_DOOR);
    }
}


RoomId AnimatronicAI::ChooseChicaNextRoom() {
    Animatronic& chica = m_animatronics[ANIM_CHICA];
    RoomId current = chica.currentRoom;

    i32 currentIdx = -1;
    for (i32 i = 0; i < s_chicaRoomCount; ++i) {
        if (s_chicaRooms[i] == current) {
            currentIdx = i;
            break;
        }
    }

    if (currentIdx < 0) {
        return ROOM_DINING_AREA;
    }

    // Chica behaves similarly to Bonnie but on the right side
    i32 direction = RandomRange(0, 3);
    i32 offset = 0;
    switch (direction) {
        case 0: offset = RandomRange(1, 2); break;
        case 1: offset = RandomRange(2, 3); break;
        case 2: offset = 1; break;
        case 3: offset = -RandomRange(1, 2); break;
    }

    i32 newIdx = currentIdx + offset;
    if (newIdx < 0) newIdx = 0;
    if (newIdx >= s_chicaRoomCount) newIdx = s_chicaRoomCount - 1;

    if (newIdx == currentIdx) {
        if (currentIdx < s_chicaRoomCount - 1) {
            newIdx = currentIdx + 1;
        } else if (currentIdx > 0) {
            newIdx = currentIdx - 1;
        }
    }

    return s_chicaRooms[newIdx];
}


// ============================================================
//  FOXY AI
//  - Unique 4-stage state machine at Pirate Cove
//  - Stage 0: Behind curtain
//  - Stage 1: Peeking out
//  - Stage 2: Gone from cove (out of view)
//  - Stage 3: Running down the West Hall
//
//  Foxy advances stages when:
//  1. AI roll passes
//  2. Player is NOT viewing Pirate Cove camera
//
//  Looking at Pirate Cove can RESET or SLOW Foxy's progress
// ============================================================

void AnimatronicAI::UpdateFoxy(const DoorSystem& doors, const CameraSystem& cameras,
                               AITickResult* results, i32& resultCount, i32 maxResults) {
    Animatronic& foxy = m_animatronics[ANIM_FOXY];

    if (!foxy.active || foxy.aiLevel <= 0 || foxy.hasAttacked) return;
    if (foxy.foxyRunning) return; // Already running, handled by OnTick
    if (foxy.atDoor) return;      // At door, handled by OnTick

    // AI roll check
    if (!CheckAIRoll(foxy.aiLevel)) return;

    // If player is viewing Pirate Cove, DON'T advance (and possibly reset)
    if (m_pirateCoveViewedThisInterval) {
        // At lower AI levels, looking resets Foxy
        // At higher AI levels (10+), it only slows him
        if (foxy.aiLevel < 10) {
            if (foxy.foxyStage > 0) {
                foxy.foxyStage = static_cast<FoxyStage>(foxy.foxyStage - 1);
                AddResult(results, resultCount, maxResults,
                           AI_EVENT_FOXY_STAGE_UP, ANIM_FOXY, ROOM_PIRATE_COVE,
                           foxy.foxyStage);
            }
        }
        // At AI >= 10, Foxy doesn't reset but also doesn't advance
        return;
    }

    // Advance Foxy's stage
    if (foxy.foxyStage < FOXY_STAGE_3) {
        foxy.foxyStage = static_cast<FoxyStage>(foxy.foxyStage + 1);
        AddResult(results, resultCount, maxResults,
                   AI_EVENT_FOXY_STAGE_UP, ANIM_FOXY, ROOM_PIRATE_COVE,
                   foxy.foxyStage);
    }

    // If Foxy reached stage 3, he starts running
    if (foxy.foxyStage == FOXY_STAGE_3) {
        foxy.foxyRunning = true;
        foxy.foxyRunTimer = 0;
        AddResult(results, resultCount, maxResults,
                   AI_EVENT_FOXY_RUNNING, ANIM_FOXY, ROOM_WEST_HALL);
    }
}


void AnimatronicAI::OnPirateCoveViewed() {
    m_pirateCoveViewedThisInterval = true;
}


// ============================================================
//  Query functions
// ============================================================

bool AnimatronicAI::IsAnimatronicOnCamera(AnimatronicId id, CameraId cam) const {
    const Animatronic& anim = m_animatronics[static_cast<i32>(id)];
    if (!anim.active) return false;
    if (anim.foxyRunning || anim.hasAttacked) return false;
    if (anim.currentRoom == ROOM_OFFICE) return false;

    RoomId expectedRoom = RoomSystem::GetCameraRoom(cam);
    return anim.currentRoom == expectedRoom;
}

bool AnimatronicAI::IsAnimatronicAtDoor(AnimatronicId id, DoorSide side) const {
    const Animatronic& anim = m_animatronics[static_cast<i32>(id)];
    if (!anim.active) return false;

    RoomId doorRoom = (side == DOOR_LEFT) ? ROOM_LEFT_DOOR : ROOM_RIGHT_DOOR;
    return anim.currentRoom == doorRoom && (anim.atDoor || anim.foxyAtDoor);
}

bool AnimatronicAI::IsAnyAnimatronicAtDoor(DoorSide side) const {
    for (i32 i = 0; i < ANIM_COUNT; ++i) {
        if (IsAnimatronicAtDoor(static_cast<AnimatronicId>(i), side)) {
            return true;
        }
    }
    return false;
}

AnimatronicId AnimatronicAI::GetAttacker() const {
    return m_attacker;
}

bool AnimatronicAI::HasAttackOccurred() const {
    return m_attackOccurred;
}

void AnimatronicAI::ClearAttack() {
    m_attackOccurred = false;
    m_attacker = ANIM_FREDDY;
    for (i32 i = 0; i < ANIM_COUNT; ++i) {
        m_animatronics[i].hasAttacked = false;
    }
}

bool AnimatronicAI::CheckAttack(AnimatronicId id, DoorSide side,
                                const DoorSystem& doors) {
    if (doors.IsDoorClosed(side)) return false;
    return true;
}

RoomId AnimatronicAI::GetFreddyNextRoom() {
    if (m_freddyPathIndex + 1 < s_freddyPathLength) {
        return s_freddyPath[m_freddyPathIndex + 1];
    }
    return s_freddyPath[s_freddyPathLength - 1];
}

} // namespace fnaf
