/**
 * Five Nights at Freddy's 1 — Recompilation
 * PowerSystem.cpp: Power drain implementation
 */

#include "PowerSystem.h"

namespace fnaf {

PowerSystem::PowerSystem()
    : m_power(100.0f)
    , m_powerOut(false)
    , m_usageLevel(1)
    , m_foxyBangs(0)
{
}

void PowerSystem::Reset() {
    m_power = 100.0f;
    m_powerOut = false;
    m_usageLevel = 1;
    m_foxyBangs = 0;
}

void PowerSystem::CalculateUsageLevel(bool cameraUp, bool leftDoorClosed,
                                       bool rightDoorClosed, bool leftLightOn,
                                       bool rightLightOn) {
    // Base usage is always 1
    i32 usage = 1;
    
    if (cameraUp)         usage++;
    if (leftDoorClosed)   usage++;
    if (rightDoorClosed)  usage++;
    // In the original game, lights do NOT increase the usage bar indicator
    // but they DO consume power briefly. However, the usage level display
    // only shows camera + doors. For accuracy, we include lights in drain
    // but cap the displayed usage at a reasonable level.
    if (leftLightOn)      usage++;
    if (rightLightOn)     usage++;
    
    // Clamp to 5 (the maximum bars shown in the original UI)
    if (usage > 5) usage = 5;
    
    m_usageLevel = usage;
}

bool PowerSystem::Tick(bool cameraUp, bool leftDoorClosed, bool rightDoorClosed,
                        bool leftLightOn, bool rightLightOn) {
    if (m_powerOut) return false;
    
    CalculateUsageLevel(cameraUp, leftDoorClosed, rightDoorClosed,
                         leftLightOn, rightLightOn);
    
    // Drain: usage_level * base_drain_per_tick
    // This means at usage 1, power lasts the full night (~534s)
    // At usage 2, power lasts half the night, etc.
    f32 drain = static_cast<f32>(m_usageLevel) * PowerConstants::DRAIN_PER_TICK_USAGE1;
    m_power -= drain;
    
    if (m_power <= 0.0f) {
        m_power = 0.0f;
        m_powerOut = true;
        return true; // Power just ran out
    }
    
    return false;
}

bool PowerSystem::DrainPower(f32 amount) {
    if (m_powerOut) return false;
    
    m_power -= amount;
    if (m_power <= 0.0f) {
        m_power = 0.0f;
        m_powerOut = true;
        return true;
    }
    return false;
}

f32 PowerSystem::GetPower() const {
    return m_power;
}

i32 PowerSystem::GetUsageLevel() const {
    return m_usageLevel;
}

bool PowerSystem::IsPowerOut() const {
    return m_powerOut;
}

f32 PowerSystem::OnFoxyDoorBang() {
    m_foxyBangs++;
    f32 drainAmount = 0.0f;
    
    // Escalating drain: 1% → 5% → 10%+ for subsequent bangs
    switch (m_foxyBangs) {
        case 1:  drainAmount = PowerConstants::FOXY_DRAIN_1; break;
        case 2:  drainAmount = PowerConstants::FOXY_DRAIN_2; break;
        default: drainAmount = PowerConstants::FOXY_DRAIN_3; break;
    }
    
    DrainPower(drainAmount);
    return drainAmount;
}

} // namespace fnaf