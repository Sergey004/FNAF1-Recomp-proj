/**
 * FNaFWorldModule.h: v2.90 — FNAF World AppModule wrapper.
 *
 * The AppModule contract (include/AppModule.h) for FNAF World: owns
 * FNaFWorldGame (logic) + FNaFWorldRender (drawing). Same shape as
 * SLModule; the ~437 MB pak streams (PrefersStreaming).
 */

#ifndef FNAF_FNAFWORLD_MODULE_H
#define FNAF_FNAFWORLD_MODULE_H

#include "AppModule.h"
#include "FNaFWorldGame.h"
#include "FNaFWorldRender.h"

namespace fnaf {

class FNaFWorldModule : public AppModule {
public:
    FNaFWorldModule() : m_wantsExit(false), m_time(0.0f),
                        m_prevScreen(-2), m_prevX(false) {}

    virtual const char* Name()   const { return "FNAFWORLD"; }
    virtual const char* Title() const { return "FNAF WORLD"; }
<<<<<<< HEAD
    // v2.134: the overworld and the flow are in, the battles are still
    // the labeled placeholder — the core draws this as 50%.
    virtual Readiness GetReadiness() const {
        Readiness r = { 1, 2, "the battles are placeholders" };
=======
    // v2.134: the splash/title, the file setup, the overworld and the
    // dialogue are in; the battles are still the labeled placeholder.
    virtual Readiness GetReadiness() const {
        Readiness r = { 7, 10, "the battles are placeholders" };
>>>>>>> d3d6b09 (The boot selector becomes a parts menu with a live status panel (v2.134))
        return r;
    }
    virtual const char* PakName() const { return "fnafworld.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload();
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }
    // the 437 MB pak cannot sit in the 512 MB UMA pool eager — streaming only
    virtual bool PrefersStreaming() const { return true; }

private:
    AppServices     m_services;
    FNaFWorldGame   m_game;
    FNaFWorldRender m_render;
    bool            m_wantsExit;
    f32             m_time;
    int             m_prevScreen;   // audio edge guard (title theme etc.)
    bool            m_prevX;        // X edge (the title hold-to-wipe)
};

} // namespace fnaf

#endif // FNAF_FNAFWORLD_MODULE_H