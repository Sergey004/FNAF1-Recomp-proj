/**
 * Five Nights at Freddy's 1 — Recompilation
 * PowerSystem.h: Power management — drain, usage levels, power-out
 */

#ifndef FNAF_POWER_SYSTEM_H
#define FNAF_POWER_SYSTEM_H

#include "Types.h"

namespace fnaf {

class PowerSystem {
public:
    PowerSystem();

    // Reset for a new night
    void Reset();

    // Call every logic tick (1/60 second)
    // Returns true if power just ran out this tick.
    bool Tick(bool cameraUp, bool leftDoorClosed, bool rightDoorClosed,
              bool leftLightOn, bool rightLightOn);

    // Force-drain power (e.g., Foxy banging on door)
    // Returns true if this drain caused power to run out.
    bool DrainPower(f32 amount);

    // Get current power percentage [0.0, 100.0]
    f32 GetPower() const;

    // Get current usage level (1-5)
    i32 GetUsageLevel() const;

    // Is power depleted?
    bool IsPowerOut() const;

    // Foxy door-bang tracker — call when Foxy bangs on closed left door
    // Returns the amount of power drained.
    f32 OnFoxyDoorBang();

private:
    f32 m_power;         // Current power [0.0, 100.0]
    bool m_powerOut;     // Has power run out?
    i32  m_usageLevel;   // Current power usage (1-5)
    i32  m_foxyBangs;    // How many times Foxy has banged on the door this night

    void CalculateUsageLevel(bool cameraUp, bool leftDoorClosed,
                             bool rightDoorClosed, bool leftLightOn, bool rightLightOn);
};

} // namespace fnaf

#endif // FNAF_POWER_SYSTEM_H
