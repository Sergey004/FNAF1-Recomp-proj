/**
 * AppRegistry.cpp: v2.28 — the static module table (see AppRegistry.h).
 */

#include "AppRegistry.h"
#include "AppModules.h"

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

} // namespace fnaf
