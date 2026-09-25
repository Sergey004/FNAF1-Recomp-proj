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

    // v2.21 analog-door test: continuous position (0 = open, 1 = closed).
    // Drives the door's RENDER frame; the logical "closed" flag (AI/entry)
    // follows as amount >= 0.5, so gameplay rules stay intact.
    void SetDoorAmount(DoorSide side, float amount);
    float GetDoorAmount(DoorSide side) const;

    // v2.22: animate the visual door slide. ToggleDoor/SetDoor flip the
    // logical "closed" (AI) instantly but slide the visual amount toward the
    // target over DOOR_SLIDE_SEC (the original 16-frame @30 FPS slide), so
    // the non-analog door animates instead of snapping. Analog mode calls
    // SetDoorAmount every frame, which disables the slide for that door.
    void Tick(f32 dt);

    // Is the specified door closed?
    bool IsDoorClosed(DoorSide side) const;

    // v2.53: closed AND the slide has landed (the dump's usage read-after-settle)
    bool IsDoorSettledClosed(DoorSide side) const;

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
    float m_doorAmount[DOOR_COUNT];   // v2.21 analog position 0(open)..1(closed)
    float m_doorTarget[DOOR_COUNT];   // v2.22 visual slide target (0 open / 1 closed)
    bool  m_doorAnimating[DOOR_COUNT];// v2.22 slide in progress (analog clears it)
    bool m_lightOn[DOOR_COUNT];
};

} // namespace fnaf

#endif // FNAF_DOOR_SYSTEM_H
