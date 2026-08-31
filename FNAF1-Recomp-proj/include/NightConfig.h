/**
 * Five Nights at Freddy's 1 — Recompilation
 * NightConfig.h: Night configuration — AI levels per night
 *
 * Verified against the original event dump (groups 305-311, 335-337):
 * starting levels + hourly DELTAS at 2/3/4 AM (Bonnie always +1;
 * Chica/Foxy +1 at 3 and 4 AM; Freddy never rises).
 */

#ifndef FNAF_NIGHT_CONFIG_H
#define FNAF_NIGHT_CONFIG_H

#include "Types.h"

namespace fnaf {

typedef i8 AILevels[ANIM_COUNT]; // [Freddy, Bonnie, Chica, Foxy]

struct NightConfig {
    i32      nightNumber;
    AILevels startingLevels;
    bool     freddyRandomStart; // Night 4: Freddy = 1 + Random(2) per run
};

// Hourly AI deltas (applied once when the hour is reached).
struct AIHourDelta {
    i32      hour;          // 2, 3, 4
    AILevels delta;         // added to the current levels
};

static const i32 NIGHT_HOUR_DELTA_COUNT = 3;
static const AIHourDelta s_nightHourDeltas[NIGHT_HOUR_DELTA_COUNT] = {
    { 2, { 0, 1, 0, 0 } },
    { 3, { 0, 1, 1, 1 } },
    { 4, { 0, 1, 1, 1 } }
};

const NightConfig* GetNightConfig(i32 night);
i32 GetNightCount();

// Applies the hourly delta for `hour` (if any) to `levels`.
void ApplyHourDelta(i32 hour, AILevels& levels);

} // namespace fnaf

#endif // FNAF_NIGHT_CONFIG_H
