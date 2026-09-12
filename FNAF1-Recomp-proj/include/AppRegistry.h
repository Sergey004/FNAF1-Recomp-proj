/**
 * AppRegistry.h: v2.28 — the static table of game modules.
 *
 * Stage 1: FNAF1 is always active and still drives main.cpp directly.
 * The registry exists so the core already addresses "the game" through
 * AppModule; stage 5 adds a boot selector (hold-a-button menu) that calls
 * AppRegistry_SetActive before any asset is loaded.
 */

#ifndef FNAF_APP_REGISTRY_H
#define FNAF_APP_REGISTRY_H

#include "Types.h"

namespace fnaf {

class AppModule;

i32        AppRegistry_Count();          // 3 (FNAF1, FNAF2, FNAF3)
AppModule* AppRegistry_Get(i32 index);   // 0 if out of range
AppModule* AppRegistry_Active();         // the module the core talks to
void       AppRegistry_SetActive(i32 index); // ignored if out of range

// v2.29: SOFT pak scan (call once at boot, before any pak load). Checks
// every module's PakName() at the canonical location (game:\<pak>), fills
// found[] per module and moves the active module onto one whose pak
// EXISTS — FNAF1 has priority, then the first module found. Never fails:
// with no paks at all FNAF1 stays active and the core's missing-pak
// message box reports it as before.
void       AppRegistry_ScanPaks(bool* found, i32 maxCount);

} // namespace fnaf

#endif // FNAF_APP_REGISTRY_H
