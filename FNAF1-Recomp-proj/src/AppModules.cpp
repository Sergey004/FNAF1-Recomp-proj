/**
 * AppModules.cpp: v2.28 — implementations of the three game modules.
 * See include/AppModules.h and docs/ARCHITECTURE.md.
 */

#include "AppModules.h"
#include "AudioSystem.h"    // Play/Stop for the module ambience
#include "TextRenderer.h"   // stub screens call DrawText (full type needed)
#include "PakLoader.h"      // FindTexture for the module's own draws
#include "SpriteBatch.h"
#include "InputSystem.h"    // GameInput snapshot (office pan / screen switch)
#include "XdkCompat.h"      // Snprintf — XDK CRT predates C99 snprintf
#include <cstdio>

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
//  FNaF2Module — v2.31: game state lives in FNaF2Game, ALL drawing
//  lives in FNaF2Render (see include/FNaF2Render.h). The module only
//  translates the pad, ticks the game and forwards to the renderer.
// ============================================================

bool FNaF2Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;
    m_lastSwitchT = -1.0f;
    m_pan = 288.0f;   // center of the 576-px pan range
    m_prevAudioScreen = -1;
    m_callDone = false;
    m_callT = 0.0f;
    m_render.Init(services.pak, services.batch, services.text);
    return true;
}

void FNaF2Module::Tick(f32 dt) {
    m_time += dt;

    // ---- translate the pad into the game's inputs ----
    FNaF2Inputs in;
    in.aPressed     = m_services.input ? m_services.input->cameraToggle : false;
    in.upPressed    = m_services.input ? m_services.input->cameraUp     : false;
    in.downPressed  = m_services.input ? m_services.input->cameraDown   : false;
    in.leftPressed  = m_services.input ? m_services.input->cameraLeft   : false;
    in.rightPressed = m_services.input ? m_services.input->cameraRight  : false;
    in.lightHeld    = m_services.input ? m_services.input->leftShoulderHeld : false; // LB = hold flashlight
    // v2.33: LT = Freddy mask hold, RT = music-box wind (both analog
    // triggers are unused by FNAF2's other mechanics)
    in.maskHeld     = m_services.input ? (m_services.input->leftDoorAxis  > 0.5f) : false;
    in.windHeld     = m_services.input ? (m_services.input->rightDoorAxis > 0.5f) : false;
    in.lookDir      = m_services.input ? m_services.input->lookDir      : 0.0f;

    const i32 viewingBefore = m_game.GetViewing();
    m_game.Tick(dt, in);
    if (m_game.GetViewing() != viewingBefore)
        m_lastSwitchT = m_time;          // camera-switch interference burst

    // office pan follows the stick while in the office
    if (m_game.GetScreen() == FNaF2Game::SCR_OFFICE) {
        m_pan += in.lookDir * 480.0f * dt;          // ~480 px/s pan speed
        if (m_pan < 0.0f)   m_pan = 0.0f;
        if (m_pan > 576.0f) m_pan = 576.0f;

        // v2.33: the phone call — ~2 s into the office, one-shot per
        // night, sample "call <night>b" (night 6 = the garbled call). The
        // dump's night gates (play voice N counters) are collapsed into
        // <night> here; no mute in FNAF2.
        if (!m_callDone) {
            m_callT += dt;
            if (m_callT >= 2.0f && m_services.audio && m_services.pak) {
                char call[32];
                const i32 n = m_game.GetNight() < 1 ? 1 : (m_game.GetNight() > 6 ? 6 : m_game.GetNight());
                Snprintf(call, sizeof(call), "snd_call %db", n);
                m_services.audio->Play(m_services.pak, call, false, 1.0f);
                m_callDone = true;
            }
        }
    } else if (m_callDone) {
        m_callDone = false;   // leaving the office resets for the next night
        m_callT = 0.0f;
    }

    // v2.32: screen ambience — the title plays static2 + "In The Depths"
    // (the FNAF2 menu song); other screens are silent for now (the office
    // fan/room loops come with the office-audio stage).
    const int scr = (int)m_game.GetScreen();
    if (scr != m_prevAudioScreen && m_services.audio && m_services.pak) {
        if (scr == (int)FNaF2Game::SCR_TITLE) {
            // dump group 3: static2 (ch1 vol 50) + The_Sand_Temple_Loop_G
            // (ch2 vol 100) — the title drone (NOT "In The Depths")
            m_services.audio->Play(m_services.pak, "snd_static2", true, 0.5f);
            m_services.audio->Play(m_services.pak, "snd_The_Sand_Temple_Loop_G", true, 1.0f);
        } else {
            m_services.audio->Stop("snd_static2");
            m_services.audio->Stop("snd_In_The_Depths_C");
        }
        m_prevAudioScreen = scr;
    }
}

void FNaF2Module::Render() {
    switch (m_game.GetScreen()) {
        case FNaF2Game::SCR_DISCLAIMER: m_render.RenderDisclaimer(m_game); break;
        case FNaF2Game::SCR_TITLE:      m_render.RenderTitle(m_game, m_time); break;
        case FNaF2Game::SCR_NIGHTSTART: {
            // night card (frame "what day") — text card until its layout is ported
            if (m_services.text) {
                char buf[32];
                Snprintf(buf, sizeof(buf), "Night %d", m_game.GetNight());
                m_services.text->DrawText(560, 330, buf, 0xFFFFFFFF);
                m_services.text->DrawText(560, 360, "12 AM", 0xFFB0B0B0);
            }
            break;
        }
        case FNaF2Game::SCR_OFFICE:
            if (m_game.GetViewing() != 0) m_render.RenderMonitor(m_game, m_time, m_time - m_lastSwitchT, m_pan);
            else                          m_render.RenderOffice(m_game, m_time, m_pan);
            break;
        case FNaF2Game::SCR_6AM: {
            if (m_services.text) m_services.text->DrawText(580, 330, "6 AM", 0xFFFFFFFF);
            break;
        }
    }
}
// ============================================================
//  FNaF3Module — placeholder (same terms as FNaF2).
// ============================================================

bool FNaF3Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;
    m_pan = 488.0f;
    m_screen = -1;    // disclaimer first (frame 0 "Frame 17")
    m_cardT = 0.0f;
    m_prevA = false;
    m_render.Init(services.pak, services.batch, services.text);
    // v2.32: title ambience (titlemusic + static, per the FNAF3 title)
    if (services.audio && services.pak) {
        services.audio->Play(services.pak, "snd_titlemusic", true, 0.45f);
        services.audio->Play(services.pak, "snd_static_sound", true, 0.35f);
    }
    return true;
}

void FNaF3Module::Tick(f32 dt) {
    m_time += dt;
    if (m_screen == -1) {
        // own warning screen (~3.5 s or any key)
        m_cardT += dt;
        const bool anyKey = m_services.input &&
            (m_services.input->cameraToggle || m_services.input->cameraUp ||
             m_services.input->cameraDown);
        if (m_cardT >= 3.5f || anyKey) { m_screen = 0; m_cardT = 0.0f; }
        return;
    }
    const bool aNow = m_services.input ? m_services.input->cameraToggle : false;
    if (aNow && !m_prevA) {
        m_screen = (m_screen == 0) ? 1 : 0;
        if (m_services.audio) {
            if (m_screen == 0) {
                m_services.audio->Play(m_services.pak, "snd_titlemusic", true, 0.45f);
                m_services.audio->Play(m_services.pak, "snd_static_sound", true, 0.35f);
            } else {
                m_services.audio->Stop("snd_titlemusic");
                m_services.audio->Stop("snd_static_sound");
            }
        }
    }
    m_prevA = aNow;
    const f32 look = m_services.input ? m_services.input->lookDir : 0.0f;
    if (m_screen == 1) {
        m_pan += look * 480.0f * dt;
        if (m_pan < 0.0f)   m_pan = 0.0f;
        if (m_pan > 976.0f) m_pan = 976.0f;   // 2000-wide world
    }
}

void FNaF3Module::Render() {
    if (m_screen == -1) {
        if (m_services.text) {
            m_services.text->DrawText((int)(530.0f * 1.25f), (int)(313.0f * 0.9375f),
                                      "WARNING!", 0xFFFFFFFF);
            m_services.text->DrawText((int)(338.0f * 1.25f), (int)(360.0f * 0.9375f),
                                      "This game contains flashing lights, loud", 0xFFFFFFFF);
            m_services.text->DrawText((int)(390.0f * 1.25f), (int)(388.0f * 0.9375f),
                                      "noises, and lots of jumpscares!", 0xFFFFFFFF);
        }
        return;
    }
    if (m_screen == 1) m_render.RenderOffice(m_time, m_pan);
    else               m_render.RenderTitle(m_time);
}

// ============================================================
//  FNaF4Module — placeholder (same terms as FNaF2).
//  Dump: build/Dumps/Five Nights at Freddys 4 (office = "level"
//  1300x768; fnaf4.pak ~373 MB, eager Load fits).
// ============================================================

bool FNaF4Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;
    m_pan = 138.0f;
    m_screen = -1;    // disclaimer first (frame 0 "Frame 17")
    m_cardT = 0.0f;
    m_prevA = false;
    m_render.Init(services.pak, services.batch, services.text);
    return true;
}

void FNaF4Module::Tick(f32 dt) {
    m_time += dt;
    if (m_screen == -1) {
        m_cardT += dt;
        const bool anyKey = m_services.input &&
            (m_services.input->cameraToggle || m_services.input->cameraUp ||
             m_services.input->cameraDown);
        if (m_cardT >= 3.5f || anyKey) {
            m_screen = 0; m_cardT = 0.0f;
            // the title theme starts with the title (dump group 1)
            if (m_services.audio && m_services.pak)
                m_services.audio->Play(m_services.pak, "snd_title", true, 0.3f);
        }
        return;
    }
    const bool aNow = m_services.input ? m_services.input->cameraToggle : false;
    if (aNow && !m_prevA) m_screen = (m_screen == 0) ? 1 : 0;
    m_prevA = aNow;
    const f32 look = m_services.input ? m_services.input->lookDir : 0.0f;
    if (m_screen == 1) {
        m_pan += look * 480.0f * dt;
        if (m_pan < 0.0f)   m_pan = 0.0f;
        if (m_pan > 276.0f) m_pan = 276.0f;   // 1300-wide bedroom
    }
}

void FNaF4Module::Render() {
    if (m_screen == -1) {
        // FNAF4's warning is RED (frame 0 "Frame 17")
        if (m_services.text) {
            m_services.text->DrawText((int)(465.0f * 1.25f), (int)(290.0f * 0.9375f),
                                      "WARNING!", 0xFF2020E0);
            m_services.text->DrawText((int)(255.0f * 1.25f), (int)(365.0f * 0.9375f),
                                      "THIS GAME CONTAINS FLASHING LIGHTS, LOUD", 0xFF2020E0);
            m_services.text->DrawText((int)(298.0f * 1.25f), (int)(393.0f * 0.9375f),
                                      "NOISES, AND LOTS OF JUMPSCARES!", 0xFF2020E0);
        }
        return;
    }
    if (m_screen == 1) m_render.RenderOffice(m_time, m_pan);
    else               m_render.RenderTitle(m_time);
}

// ============================================================
//  SLModule — placeholder. When built (stage 9) it will boot via
//  PakLoader::LoadStreaming (sisterlocation.pak is ~1.5 GB) and
//  drive PreloadAsync from its room-to-room frame transitions.
// ============================================================

bool SLModule::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    return true;
}

void SLModule::Tick(f32 /*dt*/) {
    // No game yet.
}

void SLModule::Render() {
    if (m_services.text != 0)
        m_services.text->DrawText(420, 344, "Sister Location - foundation stub", 0xFF88FF88);
}

} // namespace fnaf
