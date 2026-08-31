/**
 * Five Nights at Freddy's 1 — Recompilation
 * NightConfig.cpp: Night configuration data
 *
 * All values verified against the original event dump
 * (docs/AI_MECHANICS.md §3, event groups 305-311 + 335-337).
 */

#include "NightConfig.h"
#include <cstring>

namespace fnaf {

// Starting levels [Freddy, Bonnie, Chica, Foxy] — groups 305-311.
// Night 6 is 4/10/12/6, NOT 20/20/20/20 (a widespread myth).
// Night 7 in the original reads the customize-menu sliders; the port
// presets the classic "20/20/20/20" nightmare unless wired otherwise.
static const NightConfig s_nights[7] = {
    { 1, { 0,  0,  0,  0 }, false }, // Night 1
    { 2, { 0,  3,  1,  1 }, false }, // Night 2
    { 3, { 1,  0,  5,  2 }, false }, // Night 3
    { 4, { 1,  2,  4,  6 }, true  }, // Night 4: Freddy = 1 + Random(2) (group 308)
    { 5, { 3,  5,  7,  5 }, false }, // Night 5
    { 6, { 4, 10, 12,  6 }, false }, // Night 6
    { 7, {20, 20, 20, 20 }, false }  // Night 7 (custom in the original)
};

const NightConfig* GetNightConfig(i32 night) {
    if (night < 1 || night > 7) return 0;
    return &s_nights[night - 1];
}

i32 GetNightCount() {
    return 7;
}

void ApplyHourDelta(i32 hour, AILevels& levels) {
    for (i32 c = 0; c < NIGHT_HOUR_DELTA_COUNT; ++c) {
        if (s_nightHourDeltas[c].hour != hour) continue;
        for (i32 i = 0; i < ANIM_COUNT; ++i) {
            i32 v = levels[i] + s_nightHourDeltas[c].delta[i];
            if (v < AIConstants::AI_LEVEL_MIN) v = AIConstants::AI_LEVEL_MIN;
            if (v > AIConstants::AI_LEVEL_MAX) v = AIConstants::AI_LEVEL_MAX;
            levels[i] = static_cast<i8>(v);
        }
    }
}

} // namespace fnaf
