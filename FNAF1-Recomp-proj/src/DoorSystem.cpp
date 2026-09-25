/**
 * Five Nights at Freddy's 1 — Recompilation
 * DoorSystem.cpp: Door and light control implementation
 */

#include "DoorSystem.h"
#include <cstring>

namespace fnaf {

// v2.22: the original door slide is 16 frames @ 30 FPS (speed 50), i.e.
// 16/30 = 0.5333 s end-to-end (see the removed DOOR_FRAME_T / DOOR_L/R_OPEN).
static const f32 DOOR_SLIDE_SEC = 16.0f / 30.0f;

DoorSystem::DoorSystem() {
    Reset();
}

void DoorSystem::Reset() {
    memset(m_doorClosed, 0, sizeof(m_doorClosed));
    memset(m_doorAmount, 0, sizeof(m_doorAmount));
    memset(m_doorTarget, 0, sizeof(m_doorTarget));
    memset(m_doorAnimating, 0, sizeof(m_doorAnimating));
    memset(m_lightOn, 0, sizeof(m_lightOn));
}

bool DoorSystem::ToggleDoor(DoorSide side) {
    if (side < 0 || side >= DOOR_COUNT) return false;
    // v2.53: dead zone mid-slide — a second click while the door travels is
    // ignored (the dump has no group that matches the anim-in-flight states).
    if (m_doorAnimating[side]) return m_doorClosed[side];
    m_doorClosed[side] = !m_doorClosed[side];
    // v2.22: logical state flips instantly (AI reads it); the visual slide
    // is driven by Tick() toward this target.
    m_doorTarget[side] = m_doorClosed[side] ? 1.0f : 0.0f;
    m_doorAnimating[side] = true;
    return m_doorClosed[side];
}

void DoorSystem::SetDoor(DoorSide side, bool closed) {
    if (side >= 0 && side < DOOR_COUNT) {
        m_doorClosed[side] = closed;
        m_doorTarget[side] = closed ? 1.0f : 0.0f;
        m_doorAnimating[side] = true;
    }
}

void DoorSystem::SetDoorAmount(DoorSide side, float amount) {
    if (side < 0 || side >= DOOR_COUNT) return;
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    m_doorAmount[side] = amount;
    m_doorTarget[side] = amount;      // analog drives the position directly
    m_doorAnimating[side] = false;    // ...so no slide for this door
    // Logical "closed" (enemy entry/block) follows at the half-way point, so
    // the AI rules stay exactly the original: closed when mostly closed.
    m_doorClosed[side] = (amount >= 0.5f);
}

// v2.22: advance any in-flight door slide. Moved to the logic tick (60 Hz) so
// the amount animates smoothly; the renderer keeps sampling GetDoorAmount().
void DoorSystem::Tick(f32 dt) {
    for (int side = 0; side < DOOR_COUNT; ++side) {
        if (!m_doorAnimating[side]) continue;
        const f32 target = m_doorTarget[side];
        f32 cur = m_doorAmount[side];
        if (cur < target) {
            cur += dt / DOOR_SLIDE_SEC;
            if (cur >= target) { cur = target; m_doorAnimating[side] = false; }
        } else if (cur > target) {
            cur -= dt / DOOR_SLIDE_SEC;
            if (cur <= target) { cur = target; m_doorAnimating[side] = false; }
        } else {
            m_doorAnimating[side] = false;
        }
        m_doorAmount[side] = cur;
    }
}

float DoorSystem::GetDoorAmount(DoorSide side) const {
    if (side < 0 || side >= DOOR_COUNT) return 0.0f;
    return m_doorAmount[side];
}

bool DoorSystem::IsDoorClosed(DoorSide side) const {
    if (side < 0 || side >= DOOR_COUNT) return false;
    return m_doorClosed[side];
}

bool DoorSystem::IsDoorSettledClosed(DoorSide side) const {
    if (side < 0 || side >= DOOR_COUNT) return false;
    return m_doorClosed[side] && !m_doorAnimating[side];
}

void DoorSystem::ForceDoorsOpen() {
    for (int side = 0; side < DOOR_COUNT; ++side) {
        // v2.53 (dump g103/105): power-out force-open goes through the
        // animated slide + the motor sound (was an instant snap in the port).
        m_doorClosed[side] = false;
        m_doorTarget[side] = 0.0f;
        m_doorAnimating[side] = true;
    }
}

bool DoorSystem::ToggleLight(DoorSide side) {
    if (side < 0 || side >= DOOR_COUNT) return false;
    m_lightOn[side] = !m_lightOn[side];
    // v2.53 (dump g106/108): the two lights are mutually exclusive — turning
    // one on kills the other.
    if (m_lightOn[side]) m_lightOn[side == DOOR_LEFT ? DOOR_RIGHT : DOOR_LEFT] = false;
    return m_lightOn[side];
}

void DoorSystem::SetLight(DoorSide side, bool on) {
    if (side >= 0 && side < DOOR_COUNT) {
        m_lightOn[side] = on;
    }
}

bool DoorSystem::IsLightOn(DoorSide side) const {
    if (side < 0 || side >= DOOR_COUNT) return false;
    return m_lightOn[side];
}

void DoorSystem::ForceLightsOff() {
    m_lightOn[DOOR_LEFT]  = false;
    m_lightOn[DOOR_RIGHT] = false;
}

} // namespace fnaf