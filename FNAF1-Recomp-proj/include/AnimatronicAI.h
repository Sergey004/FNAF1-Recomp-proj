/**
 * Five Nights at Freddy's 1 — Recompilation
 * AnimatronicAI.h: Full AI logic for all 4 animatronics
 *
 * v2.7.12: every rule below was verified against the original game's
 * event program (docs/AI_MECHANICS.md — event group numbers cited there):
 *
 *  - Each animatronic has its OWN movement interval:
 *    Bonnie 4.97 s, Chica 4.98 s, Freddy 3.02 s, Foxy 5.01 s (groups 188-191)
 *  - Movement roll: Random(20)+1 <= AI level
 *  - Bonnie/Chica walk the original room graphs with coin-flip branch picks
 *  - Bonnie/Chica share the "move who?" channel (original quirk)
 *  - Freddy: opportunity -> delay (1000-AI*100 ticks, monitor down) -> one
 *    path step; 4B steps need the monitor UP; door kill 25%/s (group 406)
 *  - Foxy: 6-stage machine, tablet cooldown hold, instant run when the
 *    empty cove is seen, 25 s lurk timeout, Random(2) reset after bang
 *
 * Reconstructed from the original FNAF 1 event data (black-box clean-room).
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
    AI_EVENT_FOXY_STAGE_UP   = 2,   // cove stage 0->1->2->3
    AI_EVENT_FOXY_RUNNING    = 3,   // stage 4 (sprint down West Hall)
    AI_EVENT_FOXY_AT_DOOR    = 4,   // stage 5 (tablet must drop)
    AI_EVENT_FOXY_BANG       = 5,
    AI_EVENT_AT_DOOR         = 6,   // Bonnie/Chica/Freddy at a door zone
    AI_EVENT_ATTACK          = 7,
    AI_EVENT_FREDDY_IN_OFFICE = 8  // v2.46: Freddy stepped to "freddy got in"
                                   // (group 394) — kills the lights (406/408)
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
    i32  GetAILevel(AnimatronicId id) const;
    void AddAILevel(AnimatronicId id, i32 delta);
    void SetActive(AnimatronicId id, bool active);

    // Per-tick update (60 Hz). Internally schedules each animatronic's
    // movement opportunity, Freddy's delay chain, Foxy's timers and all
    // door/attack checks. Power-out freezes every animatronic.
    i32 OnTick(const DoorSystem& doors,
               const CameraSystem& cameras,
               PowerSystem& power,
               const GameTimer& timer,
               AITickResult* results, i32 maxResults);

    // Player just switched the monitor to Pirate Cove (group 40):
    // if Foxy is lurking (stage 3) he instantly starts his run.
    void OnCoveLooked();

    bool IsAnimatronicOnCamera(AnimatronicId id, CameraId cam) const;
    bool IsAnimatronicAtDoor(AnimatronicId id, DoorSide side) const;
    bool IsAnyAnimatronicAtDoor(DoorSide side) const;

    AnimatronicId GetAttacker() const;
    bool HasAttackOccurred() const;
    void ClearAttack();

    // Deterministic RNG for host tests (LCG, platform independent)
    void SeedRNG(i32 seed);
    i32  RandomRange(i32 min, i32 max);

private:
    Animatronic m_animatronics[ANIM_COUNT];
    AnimatronicId m_attacker;
    bool          m_attackOccurred;

    i32  m_rngState;

    // Per-animatronic opportunity accumulators (seconds).
    f64  m_oppAccum[ANIM_COUNT];

    // Bonnie/Chica shared "move who?" channel: 0 none, 1 Bonnie, 2 Chica
    i32  m_moveWho;

    // Freddy state machine (mirrors alterable 12/13 on "freddy bear")
    bool m_freddyPending;      // opportunity passed, waiting out the delay
    i32  m_freddyDelayTicks;   // alterable[13]
    bool m_freddyGo;           // alterable[12] == 2 -> take one step now
    bool m_freddyAtDoorZone;   // standing at "freddy got in" (right door)
    i32  m_freddyPathIndex;    // 0..5 into s_freddyPath
    i32  m_freddyDoorRollAccum;// 1 s accumulator for the 25 % door kill

    // Foxy state (mirrors fox progress + chica alterables 5/6/12/15)
    FoxyStage m_foxyStage;
    i32  m_foxyRunTimer;       // stage 4 counter (100 ticks)
    i32  m_foxyLurkTimer;      // stage 3 counter (1500 ticks)
    i32  m_foxyTabletCooldown; // alterable[12]: refreshed while tablet up
    i32  m_foxyCooldownTick;   // 0.1 s refresh accumulator

    // Per-tick helpers
    void RunOpportunity(AnimatronicId id, f64 interval,
                        const CameraSystem& cameras,
                        AITickResult* results, i32& count, i32 max);
    bool CheckAIRoll(i32 aiLevel);
    bool ViewingCove(const CameraSystem& cameras) const;

    void MoveBonnie(const DoorSystem& doors, AITickResult* results,
                    i32& count, i32 max);
    void MoveChica(const DoorSystem& doors, AITickResult* results,
                   i32& count, i32 max);
    void UpdateFreddy(const DoorSystem& doors, const CameraSystem& cameras,
                      AITickResult* results, i32& count, i32 max);
    void UpdateFoxy(const DoorSystem& doors, const CameraSystem& cameras,
                    PowerSystem& power, AITickResult* results,
                    i32& count, i32 max);

    void RegisterAttack(AnimatronicId id, AITickResult* results,
                        i32& count, i32 max);
    void AddResult(AITickResult* results, i32& count, i32 maxResults,
                   AIEvent event, AnimatronicId anim, RoomId room,
                   FoxyStage stage = FOXY_STAGE_0, f32 drain = 0.0f);

    static const RoomId s_freddyPath[];
    static const i32    s_freddyPathLength;
};

} // namespace fnaf

#endif // FNAF_ANIMATRONIC_AI_H
