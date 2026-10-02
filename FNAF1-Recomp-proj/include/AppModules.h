/**
 * AppModules.h: v2.28/2.29 — the game modules on the AppModule contract.
 *
 *   FNaF1Module — FNAF 1. Stage 1: the game still runs directly from
 *     main.cpp (its state machine, Game/GameRender, sounds and save are
 *     all live there); this module records the contract and migrates
 *     piece by piece (docs/ARCHITECTURE.md, "Migration").
 *   FNaF2Module / FNaF3Module / FNaF4Module / SLModule — placeholders.
 *     They know their bundle names (the boot pak scan reports/probes
 *     them) and render stub screens if ever activated; no game code yet.
 *
 * Activation: AppRegistry (src/AppRegistry.cpp). FNAF1 has priority;
 * a boot selector comes later (ARCHITECTURE.md, stage 5).
 */

#ifndef FNAF_APP_MODULES_H
#define FNAF_APP_MODULES_H

#include "AppModule.h"
#include "FNaF2Game.h"
#include "FNaF2Render.h"
#include "FNaF3Game.h"
#include "FNaF3Render.h"
#include "FNaF4Game.h"
#include "FNaF4Render.h"

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
//  FNAF 2 (placeholder — v2.30 renders its TITLE screen from the
//  dump; everything below the title is still stubbed).
//  Dump: build/Dumps/Five Nights at Freddys 2, frame 1 "title"
//  1024x768. Coordinates map to the 720p screen as a pillarbox:
//  scale 0.9375, centered (160 px bars).
// ------------------------------------------------------------
class FNaF2Module : public AppModule {
public:
    FNaF2Module() : m_wantsExit(false), m_time(0.0f) {}

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
    f32         m_time;        // seconds since activation (drives static/glitch)
    f32         m_lastSwitchT; // camera-switch blip timer (monitor static)
    f32         m_pan;         // office pan: 0..(1600-1024) world px (stick)
    int         m_prevAudioScreen; // v2.32: ambience switching (-1 none)
    bool        m_callDone;    // v2.33: phone call played this night
    f32         m_callT;       // v2.33: seconds in the office before the call
    i32         m_sceneValue;  // v2.59: dump "Active 16" scene value (0 = keep)
    i32         m_lastSceneValue;
    FNaF2Game   m_game;        // v2.31: the dump-mirrored game state
    FNaF2Render m_render;      // v2.31: the per-game renderer (no GameRender)
};

// ------------------------------------------------------------
//  FNAF 3 (v2.61 — the night loop lives in FNaF3Game; the module
//  translates the pad, owns the screen ambience and forwards draw)
// ------------------------------------------------------------
class FNaF3Module : public AppModule {
public:
    FNaF3Module() : m_wantsExit(false), m_time(0.0f),
                    m_prevAudioScreen(-2), m_prevX(false) {}

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
    f32         m_time;            // anim clock (static bursts)
    int         m_prevAudioScreen; // ambience switching (-2 = none yet)
    bool        m_prevX;           // X button edge (lure / seal)
    FNaF3Game   m_game;
    FNaF3Render m_render;
};

// ------------------------------------------------------------
//  FNAF 4 (v2.61 — the bedroom night loop lives in FNaF4Game).
//  Pad map (NO cursor): D-pad walks (left door / center / closet /
//  right door; up = to bed, down/B = out); A (hold) = flashlight
//  peek; X (hold) = shut the door; listening = stand at a door
//  holding nothing. The 373 MB pak streams (PrefersStreaming).
// ------------------------------------------------------------
class FNaF4Module : public AppModule {
public:
    FNaF4Module() : m_wantsExit(false), m_time(0.0f), m_pan(138.0f),
                    m_prevAudioScreen(-2) {}

    virtual const char* Name()   const { return "FNAF4"; }
    virtual const char* PakName() const { return "fnaf4.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload() {}
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }
    // 373 MB pak + 1281 eager textures = over the 512 MB UMA pool (real-HW
    // console crash) — the streaming loader is mandatory for FNAF4.
    virtual bool PrefersStreaming() const { return true; }

private:
    AppServices m_services;
    bool        m_wantsExit;
    f32         m_time;            // anim clock
    f32         m_pan;             // bedroom pan 0..276 (1300-wide world)
    int         m_prevAudioScreen; // ambience switching (-2 = none yet)
    FNaF4Game   m_game;
    FNaF4Render m_render;
};

// ------------------------------------------------------------
//  Sister Location (placeholder) — will boot through the
//  streaming pak loader (its pak is ~1.5 GB).
// ------------------------------------------------------------
class SLModule : public AppModule {
public:
    SLModule() : m_wantsExit(false) {}

    virtual const char* Name()   const { return "SL"; }
    virtual const char* PakName() const { return "sisterlocation.pak"; }

    virtual bool Load(AppServices& services);
    virtual void Unload() {}
    virtual void Tick(f32 dt);
    virtual void Render();
    virtual bool WantsExit() const { return m_wantsExit; }
    // 1.5 GB pak — streaming is the only option for SL (stage 9).
    virtual bool PrefersStreaming() const { return true; }

private:
    AppServices m_services;
    bool        m_wantsExit;
};

} // namespace fnaf

#endif // FNAF_APP_MODULES_H
