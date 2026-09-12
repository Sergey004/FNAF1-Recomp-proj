/**
 * AppModule.h: v2.28 — the game-module contract.
 *
 * The app is split into two layers so FNAF 2 / FNAF 3 can reuse the same
 * Xbox 360 shell (see docs/ARCHITECTURE.md):
 *
 *   CORE ("app shell", main.cpp + the *System/*.Batch sources):
 *     D3D9 device + frame loop, SpriteBatch/render targets, TextRenderer,
 *     PakLoader, XAudio2 dB mixer, XInput polling, fades, Xbox message
 *     boxes, XContent save wrappers, achievements plumbing. Owns the
 *     single 60 Hz loop and the frame Begin/End.
 *
 *   MODULE (one per game — FNaF1Module, FNaF2Module, FNaF3Module):
 *     everything that belongs to that one game: its pak bundle, asset ids,
 *     logic (state machine/AI/cameras/doors), its renderer, its sounds and
 *     its save layout.
 *
 * The core NEVER includes game headers; it talks to a module only through
 * this interface. A module NEVER owns the device or the loop; it gets
 * AppServices and fills the core's frame.
 *
 * VS2010/C++03: no nullptr, no lambdas, no override keyword.
 */

#ifndef FNAF_APP_MODULE_H
#define FNAF_APP_MODULE_H

#include "Types.h"

namespace fnaf {

class AudioSystem;
class PakLoader;
class SpriteBatch;
class TextRenderer;

// Services the core hands to every module once, at boot. All owned by the
// core — a module must not free or re-create them.
struct AppServices {
    AudioSystem*  audio;   // XAudio2 dB mixer (channels, Play/PlayOnChannel)
    PakLoader*    pak;     // shared Clickteam-pak loader (game:\ reads)
    SpriteBatch*  batch;   // quad queue; core calls Begin() before module Render()
    TextRenderer* text;    // bitmap text (debug/HUD strings)

    AppServices() : audio(0), pak(0), batch(0), text(0) {}
};

class AppModule {
public:
    virtual ~AppModule() {}

    // Display name ("FNAF1") — boot banner and the future boot selector.
    virtual const char* Name() const = 0;

    // Asset bundle this module needs ("fnaf1.pak"). The core checks for it
    // and runs the shared "missing <pak>" message-box flow on failure.
    virtual const char* PakName() const = 0;

    // Load assets + per-game init. false = fatal (core shows the error UI).
    // Stage 1 note: FNAF1 still loads its pak from main.cpp directly, so
    // FNaF1Module::Load is a record-keeping no-op (see ARCHITECTURE.md).
    virtual bool Load(AppServices& services) = 0;

    // Release everything Load() acquired. Called before the core exits.
    virtual void Unload() = 0;

    // Fixed 60 Hz logic step. The core has already polled input this frame;
    // stage 2 will pass a snapshot instead of the raw poll (see docs).
    virtual void Tick(f32 dt) = 0;

    // Fill the current frame. Called between the core's batch Begin/End;
    // the module queues quads/text only — never touches the device.
    virtual void Render() = 0;

    // Module asks the core to shut the app down (Golden-Freddy-style
    // force close). Polled once per frame after Tick.
    virtual bool WantsExit() const = 0;
};

} // namespace fnaf

#endif // FNAF_APP_MODULE_H
