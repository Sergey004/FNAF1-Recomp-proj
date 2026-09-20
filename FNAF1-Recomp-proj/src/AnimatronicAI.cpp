/**
 * Five Nights at Freddy's 1 — Recompilation
 * AnimatronicAI.cpp: AI logic implementation (v2.7.12)
 *
 * Every rule is matched to the original event program; see
 * docs/AI_MECHANICS.md for the per-group evidence.
 */

#include "AnimatronicAI.h"
#include <cstring>

namespace fnaf {

// ============================================================
//  Freddy's strict movement path (group path 1A->1B->7->6->4A->4B)
//  Verified from groups 389-395: Show Stage -> Dining -> Bathrooms
//  -> Kitchen -> East Hall -> East Hall Corner -> door zone.
// ============================================================
const RoomId AnimatronicAI::s_freddyPath[] = {
    ROOM_SHOW_STAGE,       // 0: Starting position
    ROOM_DINING_AREA,      // 1
    ROOM_RESTROOMS,        // 2  (cam 7)
    ROOM_KITCHEN,          // 3  (cam 6)  <- v2.7.12: real path includes Kitchen
    ROOM_EAST_HALL,        // 4  (cam 4A)
    ROOM_EAST_HALL_CORNER, // 5  (cam 4B)
    ROOM_RIGHT_DOOR        // 6: door zone ("freddy got in")
};
const i32 AnimatronicAI::s_freddyPathLength = 7;


AnimatronicAI::AnimatronicAI()
    : m_attacker(ANIM_FREDDY)
    , m_attackOccurred(false)
    , m_rngState(12345)
    , m_moveWho(0)
    , m_freddyPending(false)
    , m_freddyDelayTicks(0)
    , m_freddyGo(false)
    , m_freddyAtDoorZone(false)
    , m_freddyPathIndex(0)
    , m_freddyDoorRollAccum(0)
    , m_foxyStage(FOXY_STAGE_0)
    , m_foxyRunTimer(0)
    , m_foxyLurkTimer(0)
    , m_foxyTabletCooldown(0)
    , m_foxyCooldownTick(0)
{
    for (i32 i = 0; i < ANIM_COUNT; ++i) m_oppAccum[i] = 0.0;
}


void AnimatronicAI::Reset() {
    m_attackOccurred = false;
    m_attacker = ANIM_FREDDY;
    m_rngState = 12345;
    m_moveWho = 0;
    m_freddyPending = false;
    m_freddyDelayTicks = 0;
    m_freddyGo = false;
    m_freddyAtDoorZone = false;
    m_freddyPathIndex = 0;
    m_freddyDoorRollAccum = 0;
    m_foxyStage = FOXY_STAGE_0;
    m_foxyRunTimer = 0;
    m_foxyLurkTimer = 0;
    m_foxyTabletCooldown = 0;
    m_foxyCooldownTick = 0;
    for (i32 i = 0; i < ANIM_COUNT; ++i) m_oppAccum[i] = 0.0;

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
    if (level < AIConstants::AI_LEVEL_MIN) level = AIConstants::AI_LEVEL_MIN;
    if (level > AIConstants::AI_LEVEL_MAX) level = AIConstants::AI_LEVEL_MAX;
    m_animatronics[static_cast<i32>(id)].aiLevel = level;

    // Freddy and Foxy become active when they get AI > 0
    if (level > 0) {
        m_animatronics[static_cast<i32>(id)].active = true;
    }
}

i32 AnimatronicAI::GetAILevel(AnimatronicId id) const {
    return m_animatronics[static_cast<i32>(id)].aiLevel;
}

void AnimatronicAI::AddAILevel(AnimatronicId id, i32 delta) {
    SetAILevel(id, GetAILevel(id) + delta);
}

void AnimatronicAI::SetActive(AnimatronicId id, bool active) {
    m_animatronics[static_cast<i32>(id)].active = active;
}


// ============================================================
//  LCG RNG (platform-independent, no std::rand dependency)
// ============================================================

i32 AnimatronicAI::RandomRange(i32 min, i32 max) {
    m_rngState = m_rngState * 1664525 + 1013904223;
    u32 val = static_cast<u32>(m_rngState);
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
//  AI Roll Check — Random(20)+1 <= aiLevel (groups 188-191)
// ============================================================

bool AnimatronicAI::CheckAIRoll(i32 aiLevel) {
    if (aiLevel <= 0) return false;
    if (aiLevel >= 20) return true;  // AI 20 = always moves
    i32 roll = RandomRange(1, 20);
    return roll <= aiLevel;
}


bool AnimatronicAI::ViewingCove(const CameraSystem& cameras) const {
    return cameras.IsMonitorUp() && cameras.GetCurrentCamera() == CAM_1C;
}


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
//  OnTick — called every logic tick (60 fps).
//  Power-out freezes everyone (groups 315-320 gate on power down == 0;
//  the dark-office sequence owns the night from there).
// ============================================================

i32 AnimatronicAI::OnTick(const DoorSystem& doors,
                           const CameraSystem& cameras,
                           PowerSystem& power,
                           const GameTimer& timer,
                           AITickResult* results, i32 maxResults) {
    i32 count = 0;

    if (m_attackOccurred) return count;
    if (power.IsPowerOut()) return count;

    const f64 dt = TimeConstants::TICK_INTERVAL_SEC;
    const bool monitorUp = cameras.IsMonitorUp();

    // ---- Foxy tablet cooldown (chica alterable[12], groups 313/329) ----
    // While any camera is up it is re-armed every 0.1 s; otherwise decays.
    m_foxyCooldownTick++;
    if (monitorUp) {
        if (m_foxyCooldownTick >= 6) {           // Timer 100 = 0.1 s
            m_foxyCooldownTick = 0;
            m_foxyTabletCooldown = TimeConstants::FOXY_COOLDOWN_MIN_TICKS
                                 + RandomRange(0, TimeConstants::FOXY_COOLDOWN_RAND - 1);
        }
    } else {
        m_foxyCooldownTick = 0;
        if (m_foxyTabletCooldown > 0) m_foxyTabletCooldown--;
    }

    // ---- Movement opportunities (own interval per animatronic) ----
    // Order matters: Bonnie before Chica replicates the "move who?"
    // overwrite quirk when both succeed on the same tick.
    RunOpportunity(ANIM_BONNIE, TimeConstants::BONNIE_MOVE_INTERVAL_SEC,
                   cameras, results, count, maxResults);
    RunOpportunity(ANIM_CHICA, TimeConstants::CHICA_MOVE_INTERVAL_SEC,
                   cameras, results, count, maxResults);
    RunOpportunity(ANIM_FREDDY, TimeConstants::FREDDY_MOVE_INTERVAL_SEC,
                   cameras, results, count, maxResults);
    RunOpportunity(ANIM_FOXY, TimeConstants::FOXY_MOVE_INTERVAL_SEC,
                   cameras, results, count, maxResults);

    // ---- Execute the channel moves + everything frame-based ----
    MoveBonnie(doors, results, count, maxResults);
    MoveChica(doors, results, count, maxResults);
    UpdateFreddy(doors, cameras, results, count, maxResults);
    UpdateFoxy(doors, cameras, power, results, count, maxResults);

    (void)timer;
    return count;
}


// ============================================================
//  Opportunity scheduler — fires CheckAIRoll per animatronic
//  on its own interval, then dispatches the per-character move.
// ============================================================

void AnimatronicAI::RunOpportunity(AnimatronicId id, f64 interval,
                                    const CameraSystem& cameras,
                                    AITickResult* results, i32& count, i32 max) {
    Animatronic& anim = m_animatronics[static_cast<i32>(id)];
    if (!anim.active || anim.aiLevel <= 0 || anim.hasAttacked) return;

    m_oppAccum[id] += TimeConstants::TICK_INTERVAL_SEC;
    if (m_oppAccum[id] < interval) return;
    m_oppAccum[id] -= interval;

    if (id == ANIM_BONNIE) {
        // Group 188: roll -> claim the shared "move who?" channel (1)
        if (CheckAIRoll(anim.aiLevel)) m_moveWho = 1;
    } else if (id == ANIM_CHICA) {
        // Group 189: roll -> claim the channel (2); overwrites Bonnie's 1
        if (CheckAIRoll(anim.aiLevel)) m_moveWho = 2;
    } else if (id == ANIM_FREDDY) {
        // Group 190: monitor must be DOWN, roll -> pending
        if (!m_freddyAtDoorZone && !cameras.IsMonitorUp() &&
            CheckAIRoll(anim.aiLevel)) {
            m_freddyPending = true;
        }
    } else if (id == ANIM_FOXY) {
        // Group 191: not watching the Cove, tablet cooldown spent,
        // stage < 3, roll -> advance one cove stage.
        Animatronic& foxy = m_animatronics[ANIM_FOXY];
        if (!foxy.foxyRunning && !foxy.foxyAtDoor &&
            m_foxyStage < FOXY_STAGE_3 &&
            m_foxyTabletCooldown == 0 &&
            !ViewingCove(cameras) &&
            CheckAIRoll(anim.aiLevel)) {
            m_foxyStage = static_cast<FoxyStage>(m_foxyStage + 1);
            foxy.foxyStage = m_foxyStage;
            AddResult(results, count, max, AI_EVENT_FOXY_STAGE_UP,
                      ANIM_FOXY, ROOM_PIRATE_COVE, m_foxyStage);
        }
    }
}


// ============================================================
//  BONNIE — room graph (groups 199-215)
//  Branch choice: Random(2)+1 in {1,2} (group 201 re-rolls every
//  second; statistically a coin flip taken at move time).
// ============================================================

void AnimatronicAI::MoveBonnie(const DoorSystem& doors, AITickResult* results,
                                i32& count, i32 max) {
    Animatronic& bonnie = m_animatronics[ANIM_BONNIE];
    if (m_moveWho != 1) return;
    if (!bonnie.active || bonnie.hasAttacked) { m_moveWho = 0; return; }

    RoomId dest = bonnie.currentRoom;
    bool move = false;

    if (bonnie.atDoor) {
        // Groups 214/215: at "ready to attack left". The door state must be
        // SETTLED: enter only fully open (alterable[0]==0), retreat only
        // fully closed (==2); mid-slide (1/4) holds the check — move who?
        // stays set and the groups re-run every tick until the door lands.
        const f32 amt = doors.GetDoorAmount(DOOR_LEFT);
        if (amt > 0.0f && amt < 1.0f) return;   // door mid-transition: wait
        if (!doors.IsDoorClosed(DOOR_LEFT)) {
            dest = ROOM_OFFICE;                 // "got you left" -> kill
            move = true;
        } else {
            dest = ROOM_DINING_AREA;            // retreat to 1B
            move = true;
        }
        bonnie.atDoor = false;
        bonnie.doorWaitTicks = 0;
    } else {
        const i32 choice = RandomRange(1, 2);
        switch (bonnie.currentRoom) {
            case ROOM_SHOW_STAGE:       dest = (choice == 1) ? ROOM_BACKSTAGE : ROOM_DINING_AREA; move = true; break;
            case ROOM_BACKSTAGE:        dest = (choice == 1) ? ROOM_DINING_AREA : ROOM_WEST_HALL; move = true; break;
            case ROOM_DINING_AREA:      dest = (choice == 1) ? ROOM_BACKSTAGE : ROOM_WEST_HALL;  move = true; break;
            case ROOM_WEST_HALL:        dest = (choice == 1) ? ROOM_SUPPLY_CLOSET : ROOM_WEST_HALL_CORNER; move = true; break;
            case ROOM_WEST_HALL_CORNER: dest = (choice == 1) ? ROOM_SUPPLY_CLOSET : ROOM_LEFT_DOOR; move = true; break;
            case ROOM_SUPPLY_CLOSET:    dest = (choice == 1) ? ROOM_LEFT_DOOR : ROOM_WEST_HALL; move = true; break;
            default: break; // unknown room: stay
        }
    }

    m_moveWho = 0; // channel released (groups do this on every branch)
    if (!move || dest == bonnie.currentRoom) return;

    bonnie.currentRoom = dest;
    AddResult(results, count, max, AI_EVENT_MOVED, ANIM_BONNIE, dest);

    if (dest == ROOM_LEFT_DOOR) {
        bonnie.atDoor = true;
        bonnie.doorWaitTicks = 0;
        AddResult(results, count, max, AI_EVENT_AT_DOOR, ANIM_BONNIE, dest);
    } else if (dest == ROOM_OFFICE) {
        RegisterAttack(ANIM_BONNIE, results, count, max);
    }
}


// ============================================================
//  CHICA — room graph (groups 232-244)
// ============================================================

void AnimatronicAI::MoveChica(const DoorSystem& doors, AITickResult* results,
                               i32& count, i32 max) {
    Animatronic& chica = m_animatronics[ANIM_CHICA];
    if (m_moveWho != 2) return;
    if (!chica.active || chica.hasAttacked) { m_moveWho = 0; return; }

    RoomId dest = chica.currentRoom;
    bool move = false;

    if (chica.atDoor) {
        // Groups 243/244: at "ready to attack right" — same settle rule as
        // Bonnie's side: hold while the door is mid-transition.
        const f32 amt = doors.GetDoorAmount(DOOR_RIGHT);
        if (amt > 0.0f && amt < 1.0f) return;   // door mid-transition: wait
        if (!doors.IsDoorClosed(DOOR_RIGHT)) {
            dest = ROOM_OFFICE;                 // "got you right" -> kill
            move = true;
        } else {
            dest = ROOM_EAST_HALL;              // retreat to 4A
            move = true;
        }
        chica.atDoor = false;
        chica.doorWaitTicks = 0;
    } else {
        const i32 choice = RandomRange(1, 2);
        switch (chica.currentRoom) {
            case ROOM_SHOW_STAGE:       dest = ROOM_DINING_AREA; move = true; break; // only exit (group 232)
            case ROOM_DINING_AREA:      dest = (choice == 1) ? ROOM_RESTROOMS : ROOM_KITCHEN; move = true; break;
            case ROOM_KITCHEN:          dest = (choice == 1) ? ROOM_RESTROOMS : ROOM_EAST_HALL; move = true; break;
            case ROOM_RESTROOMS:        dest = (choice == 1) ? ROOM_KITCHEN : ROOM_EAST_HALL; move = true; break;
            case ROOM_EAST_HALL:        dest = (choice == 1) ? ROOM_DINING_AREA : ROOM_EAST_HALL_CORNER; move = true; break;
            case ROOM_EAST_HALL_CORNER: dest = (choice == 1) ? ROOM_EAST_HALL : ROOM_RIGHT_DOOR; move = true; break;
            default: break;
        }
    }

    m_moveWho = 0;
    if (!move || dest == chica.currentRoom) return;

    chica.currentRoom = dest;
    AddResult(results, count, max, AI_EVENT_MOVED, ANIM_CHICA, dest);

    if (dest == ROOM_RIGHT_DOOR) {
        chica.atDoor = true;
        chica.doorWaitTicks = 0;
        AddResult(results, count, max, AI_EVENT_AT_DOOR, ANIM_CHICA, dest);
    } else if (dest == ROOM_OFFICE) {
        RegisterAttack(ANIM_CHICA, results, count, max);
    }
}


// ============================================================
//  FREDDY (groups 190, 397-398, 389-395, 406)
//  pending -> delay (1000 - AI*100 ticks, monitor down) -> ONE step
//  At 4B: monitor UP (not viewing CAM 4B) decides door entry vs retreat
//  to 4A; the retreat also needs CAM 4A off screen (groups 394/395).
//  At door zone: 25% per second kill with monitor down (group 406).
// ============================================================

void AnimatronicAI::UpdateFreddy(const DoorSystem& doors, const CameraSystem& cameras,
                                  AITickResult* results, i32& count, i32 max) {
    Animatronic& freddy = m_animatronics[ANIM_FREDDY];
    if (!freddy.active || freddy.aiLevel <= 0 || freddy.hasAttacked) return;

    // ---- Door zone: 25%/s kill with the monitor down (group 406) ----
    // Group 406 also gates on fox progress < 5: while Foxy is sprinting or
    // at the door (stage >= 5) his kill owns the night and Freddy holds.
    if (m_freddyAtDoorZone) {
        if (m_foxyStage >= FOXY_STAGE_5) {
            m_freddyDoorRollAccum = 0;
            return;
        }
        if (!cameras.IsMonitorUp() && doors.IsDoorClosed(DOOR_RIGHT) == false) {
            m_freddyDoorRollAccum++;
            if (m_freddyDoorRollAccum >= 60) {      // Timer 1000 = 1 s
                m_freddyDoorRollAccum = 0;
                if (RandomRange(0, TimeConstants::FREDDY_DOOR_KILL_DENOM - 1) == 1) {
                    RegisterAttack(ANIM_FREDDY, results, count, max);
                }
            }
        } else {
            m_freddyDoorRollAccum = 0;
        }
        return;
    }

    // ---- 4B decision, fired by the monitor going UP (groups 394/395) ----
    // Both groups exclude viewing==42 (CAM 4B): watching 4B holds Freddy.
    // The retreat additionally excludes viewing==4 (CAM 4A), so with the
    // door CLOSED, watching 4A parks him at the corner too.
    if (freddy.currentRoom == ROOM_EAST_HALL_CORNER && m_freddyGo) {
        const CameraId viewed = cameras.GetCurrentCamera();
        if (cameras.IsMonitorUp() && viewed != CAM_4B) {
            m_freddyGo = false;
            m_freddyPending = false;
            m_freddyDelayTicks = 0;
            if (!doors.IsDoorClosed(DOOR_RIGHT)) {
                freddy.currentRoom = ROOM_RIGHT_DOOR;
                m_freddyAtDoorZone = true;
                AddResult(results, count, max, AI_EVENT_MOVED, ANIM_FREDDY, ROOM_RIGHT_DOOR);
                AddResult(results, count, max, AI_EVENT_AT_DOOR, ANIM_FREDDY, ROOM_RIGHT_DOOR);
                // v2.46: groups 406/408-412 — inside the office he kills the
                // lights (both light states := 0); Game zeroes the DoorSystem.
                AddResult(results, count, max, AI_EVENT_FREDDY_IN_OFFICE,
                          ANIM_FREDDY, ROOM_RIGHT_DOOR);
            } else if (viewed != CAM_4A) {
                freddy.currentRoom = ROOM_EAST_HALL;   // retreat to 4A
                m_freddyPathIndex = 4;
                AddResult(results, count, max, AI_EVENT_MOVED, ANIM_FREDDY, ROOM_EAST_HALL);
            }
        }
        return; // wait at 4B until the tablet is raised
    }

    // ---- Delay counter (groups 397/398/401) ----
    // [13] ticks up every frame (397, unconditional); watching Freddy's
    // current cam RESETS it to 0 (401); the move fires only with the
    // monitor down (398's viewing == 0).
    if (m_freddyPending) {
        if (cameras.IsMonitorUp() &&
            RoomSystem::GetCameraRoom(cameras.GetCurrentCamera()) == freddy.currentRoom) {
            m_freddyDelayTicks = 0;              // group 401
        } else {
            m_freddyDelayTicks++;                // group 397
        }
        if (!cameras.IsMonitorUp() &&
            m_freddyDelayTicks >= AIConstants::FREDDY_DELAY_BASE
                                - freddy.aiLevel * AIConstants::FREDDY_DELAY_PER_AI) {
            m_freddyPending = false;
            m_freddyDelayTicks = 0;
            m_freddyGo = true;                   // alterable[12] = 2
        }
    }

    if (!m_freddyGo) return;

    // ---- Take ONE path step (groups 389-392: no monitor condition) ----
    m_freddyGo = false;

    if (m_freddyPathIndex >= s_freddyPathLength - 2) {
        // Should not happen (4B handled above) — clamp for safety
        m_freddyPathIndex = s_freddyPathLength - 2;
    }

    // Step 1A -> 1B is blocked while Bonnie or Chica stand on the stage (group 389)
    if (freddy.currentRoom == ROOM_SHOW_STAGE) {
        const Animatronic& bonnie = m_animatronics[ANIM_BONNIE];
        const Animatronic& chica  = m_animatronics[ANIM_CHICA];
        if (bonnie.currentRoom == ROOM_SHOW_STAGE ||
            chica.currentRoom  == ROOM_SHOW_STAGE) {
            return; // stays pending-free; next opportunity re-arms
        }
    }

    m_freddyPathIndex++;
    const RoomId newRoom = s_freddyPath[m_freddyPathIndex];
    freddy.currentRoom = newRoom;
    AddResult(results, count, max, AI_EVENT_MOVED, ANIM_FREDDY, newRoom);
}


// ============================================================
//  FOXY (groups 315-324, 40)
//  Stage 3 lurk: 25 s timeout -> door; seen empty cove -> run NOW.
//  Stage 4 run: 100 ticks -> door. Stage 5: bang or kill.
// ============================================================

void AnimatronicAI::UpdateFoxy(const DoorSystem& doors, const CameraSystem& cameras,
                                PowerSystem& power, AITickResult* results,
                                i32& count, i32 max) {
    Animatronic& foxy = m_animatronics[ANIM_FOXY];
    if (!foxy.active || foxy.aiLevel <= 0 || foxy.hasAttacked) return;

    // v2.46 (group 40): the sprint starts when the player watches CAM 2A —
    // the west hall he runs down — while progress == 3 (either he reached 3
    // while 2A was open, or 2A is opened after). The old trigger watched the
    // COVE; at stage 3 the Cove just renders the empty garage (groups 64/65).
    if (m_foxyStage == FOXY_STAGE_3 &&
        cameras.IsMonitorUp() && cameras.GetCurrentCamera() == CAM_2A) {
        m_foxyStage = FOXY_STAGE_4;
        foxy.foxyStage = m_foxyStage;
        foxy.foxyRunning = true;
        foxy.foxyRunTimer = 0;
        m_foxyRunTimer = 0;
        m_foxyLurkTimer = 0;
        AddResult(results, count, max, AI_EVENT_FOXY_RUNNING, ANIM_FOXY,
                  ROOM_WEST_HALL, FOXY_STAGE_4);
        return;
    }

    if (m_foxyStage == FOXY_STAGE_3) {
        // Lurk timeout: 1500 ticks = 25 s straight to the door (group 320)
        m_foxyLurkTimer++;
        if (m_foxyLurkTimer > TimeConstants::FOXY_LURK_TICKS) {
            m_foxyStage = FOXY_STAGE_5;
            foxy.foxyStage = m_foxyStage;
            foxy.foxyRunning = false;
            foxy.foxyAtDoor = true;
            foxy.currentRoom = ROOM_LEFT_DOOR;
            AddResult(results, count, max, AI_EVENT_FOXY_AT_DOOR, ANIM_FOXY,
                      ROOM_LEFT_DOOR, FOXY_STAGE_5);
        }
        return;
    }

    if (m_foxyStage == FOXY_STAGE_4) {
        // Run: 100 ticks = 1.67 s down the hall (groups 315-317)
        m_foxyRunTimer++;
        foxy.foxyRunTimer = m_foxyRunTimer;
        if (m_foxyRunTimer > TimeConstants::FOXY_RUN_TICKS) {
            m_foxyStage = FOXY_STAGE_5;
            foxy.foxyStage = m_foxyStage;
            foxy.foxyRunning = false;
            foxy.foxyAtDoor = true;
            foxy.currentRoom = ROOM_LEFT_DOOR;
            AddResult(results, count, max, AI_EVENT_FOXY_AT_DOOR, ANIM_FOXY,
                      ROOM_LEFT_DOOR, FOXY_STAGE_5);
        }
        return;
    }

    if (m_foxyStage == FOXY_STAGE_5) {
        // At the door: the game force-drops the tablet (groups 321/322),
        // then group 323/324 decide bang vs kill.
        if (cameras.IsMonitorUp()) return;  // waits for the put-down
        if (!doors.IsDoorClosed(DOOR_LEFT)) {
            RegisterAttack(ANIM_FOXY, results, count, max);   // XSCREAM (group 323)
        } else {
            // Bang (group 324): knock, drain 10+50*bangs tenths, reset roll
            const f32 drainedTenths = static_cast<f32>(power.OnFoxyDoorBangTenths());
            m_foxyStage = static_cast<FoxyStage>(RandomRange(0, 1));  // Random(2)
            foxy.foxyStage = m_foxyStage;
            foxy.foxyAtDoor = false;
            foxy.currentRoom = ROOM_PIRATE_COVE;
            foxy.foxyRunTimer = 0;
            m_foxyRunTimer = 0;
            m_foxyLurkTimer = 0;
            AddResult(results, count, max, AI_EVENT_FOXY_BANG, ANIM_FOXY,
                      ROOM_LEFT_DOOR, m_foxyStage, drainedTenths / 10.0f);
        }
    }
}


void AnimatronicAI::OnCoveLooked() {
    // Handled inside UpdateFoxy via ViewingCove(); kept for API compat.
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
    return anim.currentRoom == doorRoom &&
           (anim.atDoor || anim.foxyAtDoor || anim.currentRoom == doorRoom);
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


void AnimatronicAI::RegisterAttack(AnimatronicId id, AITickResult* results,
                                    i32& count, i32 max) {
    Animatronic& anim = m_animatronics[static_cast<i32>(id)];
    anim.hasAttacked = true;
    anim.currentRoom = ROOM_OFFICE;
    m_attackOccurred = true;
    m_attacker = id;
    AddResult(results, count, max, AI_EVENT_ATTACK, id, ROOM_OFFICE);
}

} // namespace fnaf
