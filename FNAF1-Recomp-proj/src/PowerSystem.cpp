/**
 * Five Nights at Freddy's 1 — Recompilation
 * PowerSystem.cpp: Power drain implementation (v2.7.12)
 *
 * Verified against the original event program:
 *  - group 176: power left = 999 at night start
 *  - group 177: every 1 s, power -= usage (usage = 1 + monitor + doors + lights)
 *  - groups 342-345: N2 -1/6 s, N3 -1/5 s, N4 -1/4 s, N5+ -1/3 s
 *  - group 324: Foxy bang drains (10 + 50*bangs) tenths
 *  - groups 285/286: clamp at 0, power-out at <= 0
 */

#include "PowerSystem.h"

namespace fnaf {

PowerSystem::PowerSystem()
    : m_powerTenths(PowerConstants::POWER_START_TENTHS)
    , m_powerOut(false)
    , m_usageLevel(1)
    , m_bangCount(0)
    , m_nightNumber(1)
    , m_extraTicks(0)
    , m_extraAccum(0)
    , m_tick(0)
{
}

void PowerSystem::Reset(i32 nightNumber) {
    m_powerTenths = PowerConstants::POWER_START_TENTHS;
    m_powerOut = false;
    m_usageLevel = 1;
    m_bangCount = 0;
    m_nightNumber = nightNumber;
    m_tick = 0;
    m_extraAccum = 0;

    // Extra per-night drain intervals (groups 342-345). Night 1: none.
    i32 intervalSec = 0;
    switch (nightNumber) {
        case 2:  intervalSec = 6; break;
        case 3:  intervalSec = 5; break;
        case 4:  intervalSec = 4; break;
        default: intervalSec = (nightNumber >= 5) ? 3 : 0; break;
    }
    if (m_nightNumber < 2) intervalSec = 0;
    m_extraTicks = intervalSec * 60;
}

void PowerSystem::CalculateUsageLevel(bool cameraUp, bool leftDoorClosed,
                                       bool rightDoorClosed, bool leftLightOn,
                                       bool rightLightOn) {
    // Group 175: usage = 1 + monitor + left door + right door +
    // left light + right light (all five addends verified).
    i32 usage = 1;
    if (cameraUp)        usage++;
    if (leftDoorClosed)  usage++;
    if (rightDoorClosed) usage++;
    if (leftLightOn)     usage++;
    if (rightLightOn)    usage++;
    if (usage > 5) usage = 5;
    m_usageLevel = usage;
}

bool PowerSystem::Tick(bool cameraUp, bool leftDoorClosed, bool rightDoorClosed,
                        bool leftLightOn, bool rightLightOn) {
    if (m_powerOut) return false;

    CalculateUsageLevel(cameraUp, leftDoorClosed, rightDoorClosed,
                         leftLightOn, rightLightOn);

    // v2.7.12: discrete frame scheduling like the original's Timer chain
    // (drift-free). Group 177: drain once per 60 ticks.
    m_tick++;
    if (m_tick % 60 == 0) {
        m_powerTenths -= m_usageLevel;  // usage tenths per second
    }

    // Groups 342-345: extra night drain every interval
    if (m_extraTicks > 0) {
        m_extraAccum++;
        if (m_extraAccum >= m_extraTicks) {
            m_extraAccum = 0;
            m_powerTenths -= 1;
        }
    }

    // Groups 285/286: clamp and trigger power-out
    if (m_powerTenths <= 0) {
        m_powerTenths = 0;
        m_powerOut = true;
        return true;
    }
    return false;
}

void PowerSystem::ForceSecondTick(bool cameraUp, bool leftDoorClosed,
                                   bool rightDoorClosed, bool leftLightOn,
                                   bool rightLightOn) {
    m_tick += 59; // next Tick() hits the % 60 boundary
    Tick(cameraUp, leftDoorClosed, rightDoorClosed, leftLightOn, rightLightOn);
}

bool PowerSystem::DrainTenths(i32 tenths) {
    if (m_powerOut) return false;
    m_powerTenths -= tenths;
    if (m_powerTenths <= 0) {
        m_powerTenths = 0;
        m_powerOut = true;
        return true;
    }
    return false;
}

f32 PowerSystem::GetPower() const {
    return static_cast<f32>(m_powerTenths) / 10.0f;
}

i32 PowerSystem::GetPowerTenths() const {
    return m_powerTenths;
}

i32 PowerSystem::GetUsageLevel() const {
    return m_usageLevel;
}

bool PowerSystem::IsPowerOut() const {
    return m_powerOut;
}

i32 PowerSystem::OnFoxyDoorBangTenths() {
    // Group 324: drain = 10 + 50 * (bangs so far), then bang counter += 1
    const i32 drain = PowerConstants::FOXY_BANG_BASE_TENTHS
                    + PowerConstants::FOXY_BANG_SCALE_TENTHS * m_bangCount;
    m_bangCount++;
    DrainTenths(drain);
    return drain;
}

} // namespace fnaf
