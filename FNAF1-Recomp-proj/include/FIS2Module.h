/**
 * FIS2Module.h: v2.108 — Freddy in Space 2 AppModule wrapper.
 *
 * The AppModule contract (include/AppModule.h) for Freddy in Space 2: owns
 * FIS2Game (logic) + FIS2Render (drawing). Same shape as FNaFWorldModule;
 * the 390 MB pak streams (PrefersStreaming).
 */

#ifndef FNAF_FIS2_MODULE_H
#define FNAF_FIS2_MODULE_H

#include "AppModule.h"
#include "FIS2Game.h"
#include "FIS2Render.h"

namespace fnaf {

class FIS2Module : public AppModule {
public:
    FIS2Module() : m_wantsExit(false), m_time(0.0f) {}

    virtual const char* Name()   const { return "FIS2"; }
    virtual const char* Title() const { return "FREDDY IN SPACE 2"; }
    // v2.134: the flow is in, the shooter level is "step 1" of two.
    virtual Readiness GetReadiness() const {
        Readiness r = { 1, 2, "the level is step 1 of 2" };
        return r;
    }
    virtual const char* PakName() const { return "fis2.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload();
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }
    // the 390 MB pak cannot sit in the 512 MB UMA pool eager
    virtual bool PrefersStreaming() const { return true; }

private:
    AppServices   m_services;
    FIS2Game      m_game;
    FIS2Render    m_render;
    bool          m_wantsExit;
    f32           m_time;
};

} // namespace fnaf

#endif // FNAF_FIS2_MODULE_H
