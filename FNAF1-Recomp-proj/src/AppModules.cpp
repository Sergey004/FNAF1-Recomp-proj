/**
 * AppModules.cpp: v2.28 — implementations of the three game modules.
 * See include/AppModules.h and docs/ARCHITECTURE.md.
 */

#include "AppModules.h"
#include "TextRenderer.h"   // stub screens call DrawText (full type needed)

namespace fnaf {

// ============================================================
//  FNaF1Module — stage 1 adapter.
//  The game itself still lives in main.cpp (state machine,
//  Game/GameRender, audio callbacks, XContent save). This module
//  exists so the core can address "the game" through the contract;
//  stage 2+ moves main.cpp's per-state dispatch behind Tick/Render
//  piece by piece without changing behavior (ARCHITECTURE.md).
// ============================================================

FNaF1Module::FNaF1Module() : m_wantsExit(false) {}

bool FNaF1Module::Load(AppServices& services) {
    // Record the services; the actual fnaf1.pak load stays in main.cpp
    // until stage 2 (it is interleaved with the boot error UI today).
    m_services = services;
    m_wantsExit = false;
    return true;
}

void FNaF1Module::Unload() {
    // Nothing yet — main.cpp owns every asset at stage 1.
}

void FNaF1Module::Tick(f32 /*dt*/) {
    // Stage 1: main.cpp ticks the game directly. Nothing to do here.
}

void FNaF1Module::Render() {
    // Stage 1: main.cpp renders the game directly. Nothing to do here.
}

void FNaF1Module::RequestExit() {
    // Golden-Freddy-style force close will call this instead of exit(0)
    // so the core can shut D3D/XAudio down cleanly (stage 2).
    m_wantsExit = true;
}

// ============================================================
//  FNaF2Module — placeholder.
//  Real work starts with its own dump (docs/ARCHITECTURE.md):
//  FNAF2's pak/audio/asset ids and its very different rules
//  (flashlight, no office panning, vents, music box, 10 cameras).
// ============================================================

bool FNaF2Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    // Stage 1: never activated by the registry; when it becomes active
    // the core's missing-pak flow will refuse to boot without fnaf2.pak.
    return true;
}

void FNaF2Module::Tick(f32 /*dt*/) {
    // No game yet. B exits the module so a stray activation cannot soft-lock.
    // (Input reaches modules via the core in stage 2+; at stage 1 this
    // module is never ticked.)
}

void FNaF2Module::Render() {
    // No game yet — placeholder card, queued through the core's batch.
    if (m_services.text != 0)
        m_services.text->DrawText(470, 344, "FNAF2 - foundation stub", 0xFF88FF88);
}

// ============================================================
//  FNaF3Module — placeholder (same terms as FNaF2).
// ============================================================

bool FNaF3Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    return true;
}

void FNaF3Module::Tick(f32 /*dt*/) {
    // No game yet.
}

void FNaF3Module::Render() {
    if (m_services.text != 0)
        m_services.text->DrawText(470, 344, "FNAF3 - foundation stub", 0xFF88FF88);
}

} // namespace fnaf
