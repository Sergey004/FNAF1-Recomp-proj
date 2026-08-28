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
    memset(m_lightOn, 0, sizeof(m_lightOn));
}

bool DoorSystem::ToggleDoor(DoorSide side) {
    if (side < 0 || side >= DOOR_COUNT) return false;
    m_doorClosed[side] = !m_doorClosed[side];
    return m_doorClosed[side];
}

void DoorSystem::SetDoor(DoorSide side, bool closed) {
    if (side >= 0 && side < DOOR_COUNT) {
        m_doorClosed[side] = closed;
    }
}

bool DoorSystem::IsDoorClosed(DoorSide side) const {
    if (side < 0 || side >= DOOR_COUNT) return false;
    return m_doorClosed[side];
}

void DoorSystem::ForceDoorsOpen() {
    m_doorClosed[DOOR_LEFT]  = false;
    m_doorClosed[DOOR_RIGHT] = false;
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