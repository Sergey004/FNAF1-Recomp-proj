/**
 * Five Nights at Freddy's 1 — Recompilation
 * PowerSystem.h: Power management — drain, usage levels, power-out
 *
 * v2.7.12: matches the original exactly (groups 175-177, 342-345, 324):
 * power stored in TENTHS of a percent (starts 999), usage = 1..5
 * (1 + monitor + 2 doors + 2 lights), drain = usage tenths per second,
 * extra per-night drain, Foxy bang = 1% + 5% per previous bang.
 */

#ifndef FNAF_POWER_SYSTEM_H
#define FNAF_POWER_SYSTEM_H

#include "Types.h"

namespace fnaf {

class PowerSystem {
public:
    PowerSystem();

    // Reset for a new night (nightNumber selects the extra drain rate)
    void Reset(i32 nightNumber = 1);

    // Discrete-second counter (test hook)
    i64 GetSecondCount() const { return m_tick / 60; }

    // Call every logic tick (1/60 second)
    // Returns true if power just ran out this tick.
    bool Tick(bool cameraUp, bool leftDoorClosed, bool rightDoorClosed,
              bool leftLightOn, bool rightLightOn);

    // Force-drain power in tenths of a percent
    // Returns true if this drain caused power to run out.
    bool DrainTenths(i32 tenths);

    // Get current power in percent [0.0, 99.9]
    f32 GetPower() const;

    // Raw tenths (0-999) for HUD rendering
    i32 GetPowerTenths() const;

    // Get current usage level (1-5)
    i32 GetUsageLevel() const;

    // Is power depleted?
    bool IsPowerOut() const;

    // Foxy door-bang drain: (10 + 50*bangCount) tenths — group 324.
    // Returns the drained amount in TENTHS.
    i32 OnFoxyDoorBangTenths();

    // Seconds accumulator tick for tests
    void ForceSecondTick(bool cameraUp, bool leftDoorClosed,
                         bool rightDoorClosed, bool leftLightOn,
                         bool rightLightOn);

private:
    i32  m_powerTenths;  // 0..999 (tenths of a percent)
    bool m_powerOut;
    i32  m_usageLevel;   // 1..5
    i32  m_bangCount;    // Foxy bangs so far this night

    // Extra per-night drain (groups 342-345)
    i32  m_nightNumber;
    i32  m_extraTicks;   // interval in ticks (360/300/240/180), 0 = none
    i32  m_extraAccum;   // ticks since last extra drain

    i64  m_tick;         // discrete frame counter (drain every 60)

    void CalculateUsageLevel(bool cameraUp, bool leftDoorClosed,
                             bool rightDoorClosed, bool leftLightOn, bool rightLightOn);
};

} // namespace fnaf

#endif // FNAF_POWER_SYSTEM_H
