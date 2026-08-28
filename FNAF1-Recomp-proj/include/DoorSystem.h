/**
 * Five Nights at Freddy's 1 — Recompilation
 * DoorSystem.h: Office doors and hallway lights
 */

#ifndef FNAF_DOOR_SYSTEM_H
#define FNAF_DOOR_SYSTEM_H

#include "Types.h"

namespace fnaf {

class DoorSystem {
public:
    DoorSystem();

    // Reset for a new night
    void Reset();

    // Toggle a door (open ↔ closed)
    // Returns the new closed state.
    bool ToggleDoor(DoorSide side);

    // Set door state directly
    void SetDoor(DoorSide side, bool closed);

    // Is the specified door closed?
    bool IsDoorClosed(DoorSide side) const;

    // Both doors forced open (power out)
    void ForceDoorsOpen();

    // Toggle a light (on ↔ off)
    // Returns the new on state.
    bool ToggleLight(DoorSide side);

    // Set light state directly
    void SetLight(DoorSide side, bool on);

    // Is the specified light on?
    bool IsLightOn(DoorSide side) const;

    // All lights forced off (power out)
    void ForceLightsOff();

private:
    bool m_doorClosed[DOOR_COUNT];
    bool m_lightOn[DOOR_COUNT];
};

} // namespace fnaf

#endif // FNAF_DOOR_SYSTEM_H
