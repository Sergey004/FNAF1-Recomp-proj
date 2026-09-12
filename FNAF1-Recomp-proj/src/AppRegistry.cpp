/**
 * AppRegistry.cpp: v2.28 — the static module table (see AppRegistry.h).
 */

#include "AppRegistry.h"
#include "AppModules.h"
#include "XdkCompat.h"   // Snprintf — XDK CRT predates C99 snprintf
#include <cstdio>

namespace fnaf {

static FNaF1Module s_fnaf1;
static FNaF2Module s_fnaf2;
static FNaF3Module s_fnaf3;

static AppModule* const s_modules[3] = { &s_fnaf1, &s_fnaf2, &s_fnaf3 };
static i32 s_active = 0;   // stage 1: FNAF1 always

i32 AppRegistry_Count() { return 3; }

AppModule* AppRegistry_Get(i32 index) {
    if (index < 0 || index >= 3) return 0;
    return s_modules[index];
}

AppModule* AppRegistry_Active() {
    return s_modules[s_active];
}

void AppRegistry_SetActive(i32 index) {
    if (index >= 0 && index < 3) s_active = index;
}

// v2.29: soft pak scan. Only reads are performed (fopen/fclose probe) —
// a missing pak is a report, never an error.
void AppRegistry_ScanPaks(bool* found, i32 maxCount) {
    const i32 n = AppRegistry_Count() < maxCount ? AppRegistry_Count() : maxCount;

    for (i32 i = 0; i < n; ++i) {
        char path[128];
        Snprintf(path, sizeof(path), "game:\\%s", s_modules[i]->PakName());
        FILE* f = fopen(path, "rb");
        found[i] = (f != 0);
        if (f) fclose(f);
    }

    // Pick the active module: FNAF1 when its pak exists, otherwise the
    // first module whose pak was found; with nothing present FNAF1 stays
    // active so the core's missing-pak flow reports the usual way.
    i32 pick = 0;
    if (n > 0 && !found[0]) {
        pick = -1;
        for (i32 i = 1; i < n; ++i) {
            if (found[i]) { pick = i; break; }
        }
        if (pick < 0) pick = 0;
    }
    s_active = pick;
}

} // namespace fnaf
