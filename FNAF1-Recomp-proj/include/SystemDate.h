/**
 * SystemDate.h — the dashboard's system date (v2.74).
 *
 * The CFT games read a "system date" object for their date events: the
 * Halloween office dressing (month 10, day 31) in F1 (frame 3 groups
 * 431-433), F2 (groups 749-751) and F3 (groups 771-773).
 *
 * The Xbox 360 XDK has no XAPI clock, but the CRT clock IS the dashboard
 * time (the 360 keeps a synced clock).
 *
 * EPOCH WARNING: the Xbox 360's clock does NOT count from the Unix epoch —
 * it counts from 2005-11-22 00:00:00 UTC (the original Xbox from
 * 2001-11-15 00:00:00 UTC). The raw seconds are therefore shifted by that
 * offset before the calendar conversion. Both readings are tried and the
 * plausible one wins, so a CRT that already applies the offset cannot
 * double-shift the date.
 *
 * The date is printed once per process ("[sysdate] YYYY-MM-DD") so a
 * console run can be checked against the dashboard's own date.
 *
 * Header-only on purpose: the offline harness (tools/offline) does not link
 * the XDK-backed Progress.cpp.
 */
#ifndef FNaf1_SYSTEM_DATE_H
#define FNaf1_SYSTEM_DATE_H

#include "Types.h"
#include <ctime>
#include <cstdio>

namespace fnaf {

// The Xbox 360 clock epoch (2005-11-22 00:00:00 UTC) in Unix seconds.
static const time_t kXbox360EpochOffset = 1132617600;

// Converts seconds-since-Unix to a calendar date; false when the result is
// not a plausible date (year outside 2000..2100).
inline bool SystemDateFrom(time_t seconds, i32& year, i32& month, i32& day) {
    const struct tm* g = gmtime(&seconds);
    if (!g) return false;
    const i32 y = g->tm_year + 1900;
    if (y < 2000 || y > 2100) return false;
    year  = y;
    month = g->tm_mon + 1;
    day   = g->tm_mday;
    return true;
}

// month is 1..12, day 1..31. False when the clock is unavailable.
inline bool SystemDate(i32& year, i32& month, i32& day) {
    year = 0; month = 0; day = 0;
    const time_t raw = time(0);
    if (raw == (time_t)-1) return false;
    bool ok;
#ifdef _XBOX
    // The 360's CRT clock counts from 2005-11-22 00:00:00 UTC, NOT from the
    // Unix epoch, so the raw value only becomes a calendar date after the
    // offset is added. But if the raw value ALREADY reads as a plausible
    // date, this CRT counts from the Unix epoch and the offset must NOT be
    // applied — it would shift the date by ~35 years (and the October-31
    // events would never fire). The offset is therefore applied only when
    // the raw value is NOT a plausible date on its own.
    if (!SystemDateFrom(raw, year, month, day))
        ok = SystemDateFrom(raw + kXbox360EpochOffset, year, month, day);
    else
        ok = true;
#else
    ok = SystemDateFrom(raw, year, month, day);
#endif
    static bool reported = false;
    if (!reported) {
        reported = true;
        if (ok) printf("[sysdate] %04d-%02d-%02d (raw clock %d)\n",
                      year, month, day, (int)raw);
        else     printf("[sysdate] unavailable (raw clock value %d)\n", (int)raw);
    }
    return ok;
}

} // namespace fnaf

#endif // FNaf1_SYSTEM_DATE_H
