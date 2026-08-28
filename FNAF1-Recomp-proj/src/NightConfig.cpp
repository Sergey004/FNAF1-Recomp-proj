/**
 * Five Nights at Freddy's 1 — Recompilation
 * NightConfig.cpp: Night configuration data (reconstructed from published night-by-night AI tables)
 */

#include "NightConfig.h"

namespace fnaf {

// Night 1
static const AIChange s_night1Changes[] = {
    { 2, { 0, 0, 0, 1 } },
    { 3, { 0, 3, 1, 1 } }
};
static const NightConfig s_night1 = { 1, {0,0,0,0}, 2, s_night1Changes };

// Night 2
static const AIChange s_night2Changes[] = {
    { 2, { 1, 3, 1, 1 } },
    { 3, { 1, 3, 4, 2 } },
    { 4, { 2, 3, 4, 2 } }
};
static const NightConfig s_night2 = { 2, {0,3,1,1}, 3, s_night2Changes };

// Night 3
static const AIChange s_night3Changes[] = {
    { 1, { 1, 3, 2, 1 } },
    { 2, { 2, 4, 3, 2 } },
    { 3, { 2, 5, 4, 3 } },
    { 4, { 3, 5, 5, 4 } }
};
static const NightConfig s_night3 = { 3, {1,0,1,1}, 4, s_night3Changes };

// Night 4
static const AIChange s_night4Changes[] = {
    { 1, { 2, 3, 2, 2 } },
    { 2, { 3, 4, 4, 4 } },
    { 3, { 4, 6, 6, 5 } },
    { 4, { 5, 8, 8, 6 } }
};
static const NightConfig s_night4 = { 4, {1,2,1,1}, 4, s_night4Changes };

// Night 5
static const AIChange s_night5Changes[] = {
    { 1, { 3, 6, 4, 5 } },
    { 2, { 4, 8, 7, 6 } },
    { 3, { 5, 10, 9, 7 } },
    { 4, { 6, 12, 12, 8 } }
};
static const NightConfig s_night5 = { 5, {3,5,2,5}, 4, s_night5Changes };

// Night 6 (20/20/20/20)
static const AIChange s_night6Changes[] = {
    { 0, { 20, 20, 20, 20 } }
};
static const NightConfig s_night6 = { 6, {20,20,20,20}, 1, s_night6Changes };

// Night 7
static const AIChange s_night7Changes[] = {
    { 0, { 20, 20, 20, 20 } }
};
static const NightConfig s_night7 = { 7, {20,20,20,20}, 1, s_night7Changes };

static const NightConfig* s_allNights[] = {
    &s_night1, &s_night2, &s_night3, &s_night4,
    &s_night5, &s_night6, &s_night7
};
static const i32 s_nightCount = 7;

const NightConfig* GetNightConfig(i32 night) {
    if (night < 1 || night > s_nightCount) return 0;
    return s_allNights[night - 1];
}

i32 GetNightCount() {
    return s_nightCount;
}

} // namespace fnaf
