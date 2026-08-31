/**
 * Five Nights at Freddy's 1 — Recompilation
 * GameTimer.h: Night timer — tracks 12 AM to 6 AM
 */

#ifndef FNAF_GAME_TIMER_H
#define FNAF_GAME_TIMER_H

#include "Types.h"

namespace fnaf {

class GameTimer {
public:
    GameTimer();

    // Reset timer for a new night
    void Reset();

    // Advance the timer by one logic tick (1/60 second)
    // Returns true if the hour changed this tick.
    bool Tick();

    // Advance by a specific delta time in seconds
    // Returns true if the hour changed.
    bool Update(f64 deltaTimeSec);

    // Get current in-game hour (0 = 12 AM, 1 = 1 AM, ..., 5 = 5 AM, 6 = 6 AM)
    i32 GetHour() const;

    // Get elapsed time in seconds since night start
    f64 GetElapsedSeconds() const;

    // Get total night duration in seconds
    f64 GetNightDuration() const;

    // Get remaining time in seconds
    f64 GetRemainingSeconds() const;

    // Get progress through the night [0.0, 1.0]
    f32 GetProgress() const;

    // Check if the night is complete (6 AM reached)
    bool IsNightComplete() const;

    // Get the hour as a display string: "12 AM", "1 AM", ..., "6 AM"
    const char* GetHourString() const;

    // Get the number of ticks elapsed
    i64 GetTickCounter() const;

private:
    f64  m_elapsedSeconds;  // Real seconds elapsed
    i32  m_currentHour;     // Current in-game hour (0-6)
    i64  m_tickCounter;     // Total logic ticks processed

    static const char* s_hourStrings[7]; // "12 AM" through "6 AM"
};

} // namespace fnaf

#endif // FNAF_GAME_TIMER_H
