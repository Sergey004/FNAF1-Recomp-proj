/**
 * Five Nights at Freddy's 1 — Recompilation
 * GameTimer.cpp: Night timer implementation
 */

#include "GameTimer.h"
#include <cmath>

namespace fnaf {

const char* GameTimer::s_hourStrings[7] = {
    "12 AM",
    "1 AM",
    "2 AM",
    "3 AM",
    "4 AM",
    "5 AM",
    "6 AM"
};

GameTimer::GameTimer()
    : m_elapsedSeconds(0.0)
    , m_currentHour(0)
    , m_tickCounter(0)
{
}

void GameTimer::Reset() {
    m_elapsedSeconds = 0.0;
    m_currentHour = 0;
    m_tickCounter = 0;
}

bool GameTimer::Tick() {
    return Update(TimeConstants::TICK_INTERVAL_SEC);
}

bool GameTimer::Update(f64 deltaTimeSec) {
    if (m_currentHour >= 6) return false;

    m_elapsedSeconds += deltaTimeSec;
    m_tickCounter++;

    // v2.7.12: discrete frame counting like the original (minute counter
    // >= 90 x 1-second timers at 60 FPS => 5400 ticks per hour). Float
    // accumulation drifts; the original's Timer chain does not.
    const i64 hourTicks = (i64)(TimeConstants::HOUR_DURATION_SEC * TimeConstants::TICK_RATE);
    i32 newHour = static_cast<i32>(m_tickCounter / hourTicks);
    if (newHour > 6) newHour = 6;

    bool hourChanged = (newHour != m_currentHour);
    m_currentHour = newHour;

    return hourChanged;
}

i32 GameTimer::GetHour() const {
    return m_currentHour;
}

f64 GameTimer::GetElapsedSeconds() const {
    return m_elapsedSeconds;
}

f64 GameTimer::GetNightDuration() const {
    return TimeConstants::NIGHT_DURATION_SEC;
}

f64 GameTimer::GetRemainingSeconds() const {
    f64 remaining = TimeConstants::NIGHT_DURATION_SEC - m_elapsedSeconds;
    if (remaining < 0.0) remaining = 0.0;
    return remaining;
}

f32 GameTimer::GetProgress() const {
    f32 progress = static_cast<f32>(m_elapsedSeconds / TimeConstants::NIGHT_DURATION_SEC);
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;
    return progress;
}

bool GameTimer::IsNightComplete() const {
    return m_currentHour >= 6;
}

const char* GameTimer::GetHourString() const {
    if (m_currentHour >= 0 && m_currentHour < 7) {
        return s_hourStrings[m_currentHour];
    }
    return "6 AM";
}

i64 GameTimer::GetTickCounter() const {
    return m_tickCounter;
}

} // namespace fnaf