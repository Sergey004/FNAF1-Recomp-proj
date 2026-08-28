/**
 * Five Nights at Freddy's 1 — Recompilation
 * NightConfig.h: Night configuration — AI levels per hour per night
 */

#ifndef FNAF_NIGHT_CONFIG_H
#define FNAF_NIGHT_CONFIG_H

#include "Types.h"

namespace fnaf {

typedef i8 AILevels[ANIM_COUNT];

struct AIChange {
    i32      hour;
    AILevels levels;
};

struct NightConfig {
    i32      nightNumber;
    AILevels startingLevels;
    i32      changeCount;
    const AIChange* changes;
};

const NightConfig* GetNightConfig(i32 night);
i32 GetNightCount();

} // namespace fnaf

#endif // FNAF_NIGHT_CONFIG_H
