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
AppModule* AppRegistry_Get(i32 index);   // NULL if out of range
AppModule* AppRegistry_Active();         // the module the core talks to
void       AppRegistry_SetActive(i32 index); // ignored if out of range

} // namespace fnaf

#endif // FNAF_APP_REGISTRY_H
