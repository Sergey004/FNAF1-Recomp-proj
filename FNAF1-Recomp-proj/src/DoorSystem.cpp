/**
 * Five Nights at Freddy's 1 — Recompilation
 * DoorSystem.cpp: Door and light control implementation
 */

#include "DoorSystem.h"
#include <cstring>

namespace fnaf {

DoorSystem::DoorSystem() {
    Reset();
}

void DoorSystem::Reset() {
    memset(m_doorClosed, 0, sizeof(m_doorClosed));
    memset(m_doorAmount, 0, sizeof(m_doorAmount));
    memset(m_lightOn, 0, sizeof(m_lightOn));
}

bool DoorSystem::ToggleDoor(DoorSide side) {
    if (side < 0 || side >= DOOR_COUNT) return false;
    m_doorClosed[side] = !m_doorClosed[side];
    m_doorAmount[side] = m_doorClosed[side] ? 1.0f : 0.0f;
    return m_doorClosed[side];
}

void DoorSystem::SetDoor(DoorSide side, bool closed) {
    if (side >= 0 && side < DOOR_COUNT) {
        m_doorClosed[side] = closed;
        m_doorAmount[side] = closed ? 1.0f : 0.0f;
    }
}

void DoorSystem::SetDoorAmount(DoorSide side, float amount) {
    if (side < 0 || side >= DOOR_COUNT) return;
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    m_doorAmount[side] = amount;
    // Logical "closed" (enemy entry/block) follows at the half-way point, so
    // the AI rules stay exactly the original: closed when mostly closed.
    m_doorClosed[side] = (amount >= 0.5f);
}

float DoorSystem::GetDoorAmount(DoorSide side) const {
    if (side < 0 || side >= DOOR_COUNT) return 0.0f;
    return m_doorAmount[side];
}

bool DoorSystem::IsDoorClosed(DoorSide side) const {
    if (side < 0 || side >= DOOR_COUNT) return false;
    return m_doorClosed[side];
}

void DoorSystem::ForceDoorsOpen() {
    m_doorClosed[DOOR_LEFT]  = false;
    m_doorClosed[DOOR_RIGHT] = false;
    m_doorAmount[DOOR_LEFT]  = 0.0f;
    m_doorAmount[DOOR_RIGHT] = 0.0f;
}

bool DoorSystem::ToggleLight(DoorSide side) {
    if (side < 0 || side >= DOOR_COUNT) return false;
    m_lightOn[side] = !m_lightOn[side];
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