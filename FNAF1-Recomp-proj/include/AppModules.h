/**
 * AppModules.h: v2.28 — the three game modules on the AppModule contract.
 *
 *   FNaF1Module — FNAF 1. Stage 1: the game still runs directly from
 *     main.cpp (its state machine, Game/GameRender, sounds and save are
 *     all live there); this module records the contract and migrates
 *     piece by piece (docs/ARCHITECTURE.md, "Migration").
 *   FNaF2Module / FNaF3Module — placeholders. They know their bundle name
 *     and render a stub screen if ever activated; no game code yet.
 *
 * Activation: AppRegistry (src/AppRegistry.cpp). Only FNAF1 is active;
 * a boot selector comes later (ARCHITECTURE.md, stage 5).
 */

#ifndef FNAF_APP_MODULES_H
#define FNAF_APP_MODULES_H

#include "AppModule.h"

namespace fnaf {

// ------------------------------------------------------------
//  FNAF 1 (active)
// ------------------------------------------------------------
class FNaF1Module : public AppModule {
public:
    FNaF1Module();

    virtual const char* Name()   const { return "FNAF1"; }
    virtual const char* PakName() const { return "fnaf1.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload();
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }

    // Stage-2 migration hooks (main.cpp will start routing through these;
    // kept out of the AppModule interface on purpose — per-game extras):
    void RequestExit();                    // replaces the raw exit(0) calls

private:
    AppServices m_services;
    bool        m_wantsExit;
};

// ------------------------------------------------------------
//  FNAF 2 (placeholder)
// ------------------------------------------------------------
class FNaF2Module : public AppModule {
public:
    FNaF2Module() : m_wantsExit(false) {}

    virtual const char* Name()   const { return "FNAF2"; }
    virtual const char* PakName() const { return "fnaf2.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload() {}
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }

private:
    AppServices m_services;
    bool        m_wantsExit;
};

// ------------------------------------------------------------
//  FNAF 3 (placeholder)
// ------------------------------------------------------------
class FNaF3Module : public AppModule {
public:
    FNaF3Module() : m_wantsExit(false) {}

    virtual const char* Name()   const { return "FNAF3"; }
    virtual const char* PakName() const { return "fnaf3.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload() {}
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }

private:
    AppServices m_services;
    bool        m_wantsExit;
};

} // namespace fnaf

#endif // FNAF_APP_MODULES_H
