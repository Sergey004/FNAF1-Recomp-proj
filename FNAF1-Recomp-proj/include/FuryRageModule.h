/**
 * FuryRageModule.h: v2.108 — Fury's Rage AppModule wrapper.
 *
 * The AppModule contract (include/AppModule.h) for Fury's Rage: owns
 * FuryRageGame (logic) + FuryRageRender (drawing). Same shape as
 * FNaFWorldModule; the 543 MB pak streams (PrefersStreaming).
 */

#ifndef FNAF_FURYRAGE_MODULE_H
#define FNAF_FURYRAGE_MODULE_H

#include "AppModule.h"
#include "FuryRageGame.h"
#include "FuryRageRender.h"

namespace fnaf {

class FuryRageModule : public AppModule {
public:
    FuryRageModule() : m_wantsExit(false), m_time(0.0f) {}

    virtual const char* Name()   const { return "FURYS_RAGE"; }
    virtual const char* PakName() const { return "furysrage.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload();
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }
    // the 543 MB pak cannot sit in the 512 MB UMA pool eager
    virtual bool PrefersStreaming() const { return true; }

private:
    AppServices     m_services;
    FuryRageGame    m_game;
    FuryRageRender  m_render;
    bool            m_wantsExit;
    f32             m_time;
};

} // namespace fnaf

#endif // FNAF_FURYRAGE_MODULE_H
