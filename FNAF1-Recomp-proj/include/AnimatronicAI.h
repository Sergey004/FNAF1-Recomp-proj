/**
 * Five Nights at Freddy's 1 — Recompilation
 * AnimatronicAI.h: Full AI logic for all 4 animatronics
 *
 * This is the HEART of the FNAF 1 game logic.
 * Every movement decision, attack check, and Foxy behavior
 * is implemented here to match the original game.
 *
 * Reconstructed from observed FNAF 1 retail behavior (black-box, no engine
 * source or runtime reverse-engineering involved).
 * All values are matched against the original FNAF 1 behavior.
 */

#ifndef FNAF_ANIMATRONIC_AI_H
#define FNAF_ANIMATRONIC_AI_H

#include "Types.h"
#include "Animatronic.h"
#include "Room.h"
#include "DoorSystem.h"
#include "CameraSystem.h"
#include "PowerSystem.h"
#include "GameTimer.h"

namespace fnaf {

// ============================================================
//  Result of an AI tick — tells the game what happened.
// ============================================================

enum AIEvent {
    AI_EVENT_NONE            = 0,
    AI_EVENT_MOVED           = 1,
    AI_EVENT_FOXY_STAGE_UP   = 2,
    AI_EVENT_FOXY_RUNNING    = 3,
    AI_EVENT_FOXY_AT_DOOR    = 4,
    AI_EVENT_FOXY_BANG       = 5,
    AI_EVENT_AT_DOOR         = 6,
    AI_EVENT_ATTACK          = 7
};

struct AITickResult {
    AIEvent      event;
    AnimatronicId animatronic;
    RoomId      newRoom;
    FoxyStage   foxyStage;
    f32         powerDrained;
};

// ============================================================
//  AnimatronicAI
// ============================================================

class AnimatronicAI {
public:
    AnimatronicAI();

    void Reset();

    Animatronic& GetAnimatronic(AnimatronicId id);
    const Animatronic& GetAnimatronic(AnimatronicId id) const;

    void SetAILevel(AnimatronicId id, i32 level);
    void SetActive(AnimatronicId id, bool active);

    // Main AI tick — called every MOVEMENT_INTERVAL_TICKS (~5 seconds)
    i32 OnMovementOpportunity(const DoorSystem& doors,
                               const CameraSystem& cameras,
                               const GameTimer& timer,
                               AITickResult* results, i32 maxResults);

    // Per-tick updates (every logic tick, 60fps)
    i32 OnTick(const DoorSystem& doors,
               const CameraSystem& cameras,
               PowerSystem& power,
               const GameTimer& timer,
               AITickResult* results, i32 maxResults);

    bool IsAnimatronicOnCamera(AnimatronicId id, CameraId cam) const;
    bool IsAnimatronicAtDoor(AnimatronicId id, DoorSide side) const;
    bool IsAnyAnimatronicAtDoor(DoorSide side) const;

    AnimatronicId GetAttacker() const;
    bool HasAttackOccurred() const;
    void ClearAttack();

    void OnPirateCoveViewed();

private:
    Animatronic m_animatronics[ANIM_COUNT];
    AnimatronicId m_attacker;
    bool          m_attackOccurred;

    i32  m_rngState;
    i32  RandomRange(i32 min, i32 max);
    void SeedRNG(i32 seed);

    void UpdateFreddy(const DoorSystem& doors, const CameraSystem& cameras,
                      AITickResult* results, i32& resultCount, i32 maxResults);
    void UpdateBonnie(const DoorSystem& doors, AITickResult* results,
                      i32& resultCount, i32 maxResults);
    void UpdateChica(const DoorSystem& doors, AITickResult* results,
                     i32& resultCount, i32 maxResults);
    void UpdateFoxy(const DoorSystem& doors, const CameraSystem& cameras,
                    AITickResult* results, i32& resultCount, i32 maxResults);

    bool CheckAttack(AnimatronicId id, DoorSide side, const DoorSystem& doors);

    RoomId ChooseBonnieNextRoom();
    RoomId ChooseChicaNextRoom();
    RoomId GetFreddyNextRoom();

    // Freddy's strict movement path
    static const RoomId s_freddyPath[];
    static const i32    s_freddyPathLength;
    i32 m_freddyPathIndex;

    // Bonnie's reachable rooms
    static const RoomId s_bonnieRooms[];
    static const i32    s_bonnieRoomCount;

    // Chica's reachable rooms
    static const RoomId s_chicaRooms[];
    static const i32    s_chicaRoomCount;

    // Movement opportunity tick counter
    i32  m_movementTickCounter;

    // Pirate Cove view tracking for Foxy
    bool m_pirateCoveViewedThisInterval;

    // Helper: check if AI roll passes
    bool CheckAIRoll(i32 aiLevel);

    // Helper: add a result to the array
    void AddResult(AITickResult* results, i32& count, i32 maxResults,
                   AIEvent event, AnimatronicId anim, RoomId room = ROOM_NONE,
                   FoxyStage stage = FOXY_STAGE_0, f32 drain = 0.0f);
};

} // namespace fnaf

#endif // FNAF_ANIMATRONIC_AI_H
