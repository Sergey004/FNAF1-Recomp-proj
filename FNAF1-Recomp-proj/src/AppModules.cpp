/**
 * AppModules.cpp: v2.28 — implementations of the three game modules.
 * See include/AppModules.h and docs/ARCHITECTURE.md.
 */

#include "AppModules.h"
#include "AudioSystem.h"    // Play/Stop for the module ambience
#include "Achievements.h"   // v2.62: the FNAF2 add-on id writes
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

// v2.59: audio trampolines for FNaF2Game's hooks (dump channels 1..31 map
// onto the AudioSystem's generic 0..31 mixer; volumes are Clickteam 0..100).
static AudioSystem* s_fn2Audio = 0;
static PakLoader*   s_fn2Pak   = 0;
static void FNaF2SfxPlay(const char* name, bool loop, i32 channel, i32 volume) {
    if (!s_fn2Audio || !s_fn2Pak || !name) return;
    s_fn2Audio->SetChannelVolume(channel, CFVolumeToDb(volume));
    s_fn2Audio->PlayOnChannel(s_fn2Pak, name, loop, channel);
}
static void FNaF2SfxStop(const char* name) {
    if (s_fn2Audio && name) s_fn2Audio->Stop(name);
}
static void FNaF2ChVol(i32 channel, i32 volume) {
    if (s_fn2Audio) s_fn2Audio->SetChannelVolume(channel, CFVolumeToDb(volume));
}
// v2.62: the add-on achievement trampoline (the core's Achievements owner)
static Achievements* s_fn2Ach = 0;
static void FNaF2AchUnlock(i32 slot) {
    if (s_fn2Ach) s_fn2Ach->UnlockFnaf2(slot);
}

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
    m_sceneValue = 0;
    m_lastSceneValue = 0;
    m_xHoldT = 0.0f;
    m_prevRB = false;
    // v2.62: the game speaks in dump channels through these hooks; the module
    // owns the AudioSystem wiring (PlayOnChannel + CFVolume dB conversion).
    s_fn2Audio = services.audio;
    s_fn2Pak   = services.pak;
    s_fn2Ach   = services.ach;
    m_game.audio.play           = FNaF2SfxPlay;
    m_game.audio.stop           = FNaF2SfxStop;
    m_game.audio.channelVolume  = FNaF2ChVol;
    m_game.audio.unlockAch      = FNaF2AchUnlock;
    m_game.ResetToTitle();
    // v2.62: load the dump's own save ("freddy2") into the session
    Progress::PrimeStorage();
    Progress::GameProgressF2 p2;
    if (Progress::LoadF2(p2)) m_game.ApplyProgressF2(p2);
    return true;
}

void FNaF2Module::Tick(f32 dt) {
    m_time += dt;

    // ---- translate the pad into the game's inputs ----
    FNaF2Inputs in;
    in.aPressed     = m_services.input ? m_services.input->cameraToggle : false;
    in.bPressed     = m_services.input ? m_services.input->back         : false;
    in.upPressed    = m_services.input ? m_services.input->cameraUp     : false;
    in.downPressed  = m_services.input ? m_services.input->cameraDown   : false;
    in.leftPressed  = m_services.input ? m_services.input->cameraLeft   : false;
    in.rightPressed = m_services.input ? m_services.input->cameraRight  : false;
    in.lightHeld    = m_services.input ? m_services.input->leftShoulderHeld : false; // LB = hold flashlight
    // v2.59: RB = mask, X = music-box wind (cam 11), LT/RT = vent lights
    in.maskHeld        = m_services.input ? m_services.input->rightShoulderHeld : false;
    in.windHeld        = m_services.input ? m_services.input->xHeld : false;
    in.ventLightLHeld  = m_services.input ? (m_services.input->leftDoorAxis  > 0.5f) : false;
    in.ventLightRHeld  = m_services.input ? (m_services.input->rightDoorAxis > 0.5f) : false;
    in.lookDir      = m_services.input ? m_services.input->lookDir      : 0.0f;
    // v2.62: the minigames' held directions (D-pad OR the left stick) and the
    // customize mode-cycle edges
    if (m_services.input) {
        in.mgUp    = m_services.input->cameraUp    || m_services.input->lookDirY >  0.5f;
        in.mgDown  = m_services.input->cameraDown  || m_services.input->lookDirY < -0.5f;
        in.mgLeft  = m_services.input->cameraLeft  || m_services.input->lookDir   < -0.5f;
        in.mgRight = m_services.input->cameraRight || m_services.input->lookDir   >  0.5f;
        const bool rb = m_services.input->rightShoulderHeld;
        in.rbPressed = rb && !m_prevRB;
        m_prevRB = rb;
        const bool lb = m_services.input->leftShoulderHeld;
        in.lbPressed = lb && !m_prevLB;
        m_prevLB = lb;
        in.lookDirY = m_services.input->lookDirY;
    }

    // v2.62: the title X-hold wipes the freddy2 save (the dump's hold-delete)
    if (m_game.GetScreen() == FNaF2Game::SCR_TITLE && m_services.input &&
        m_services.input->xHeld) {
        m_xHoldT += dt;
        if (m_xHoldT >= 5.0f) {
            m_xHoldT = 0.0f;
            Progress::WipeF2();
            Progress::GameProgressF2 p2;
            Progress::ResetF2(p2);
            m_game.ApplyProgressF2(p2);
            printf("FNAF2 SAVE WIPED (X hold)\n");
        }
    } else {
        m_xHoldT = 0.0f;
    }

    const i32 viewingBefore = m_game.GetViewing();
    m_game.Tick(dt, in);
    if (m_game.GetViewing() != viewingBefore)
        m_lastSwitchT = m_time;          // camera-switch interference burst

    // v2.62: the save bridge — the game flips the dirty bit on the dump's
    // write beats (6 AM, endings, errors, the minigame rotation)
    if (m_game.ConsumeSaveDirty()) {
        Progress::GameProgressF2 p2;
        m_game.FillProgressF2(p2);
        if (Progress::SaveF2(p2))
            printf("FNAF2 SAVE: level=%d beatgame=%d beat6=%d\n",
                   p2.level, p2.beatgame ? 1 : 0, p2.beat6 ? 1 : 0);
    }
    // the dump's End application (the rare post-night loader) -> the boot
    if (m_game.ExitRequested()) m_wantsExit = true;

    // v2.59: the dump scene selector runs once per tick; a 0 = "no matching
    // view" keeps the previous image (the dump's own stick-behavior)
    m_lastSceneValue = m_sceneValue;
    m_sceneValue = m_game.ComputeSceneValue();

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
            m_services.audio->Stop("snd_The_Sand_Temple_Loop_G");
        }
        // v2.59: the office loop set — dump channels with their entry volumes
        // (g31 + g52-55 etc.): In_The_Depths 50, fansound 40, buzzlight/CMPTR/
        // deepbreaths/stare/melody/garble/jackinthebox/popstatic/With_S2 live
        // at 0 until the game's volume hooks move them.
        if (scr == (int)FNaF2Game::SCR_OFFICE) {
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_In_The_Depths_C",   true, 1);  s_fn2Audio->SetChannelVolume(1,  CFVolumeToDb(50));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_buzzlight",         true, 2);  s_fn2Audio->SetChannelVolume(2,  CFVolumeToDb(0));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_CMPTR_Low_Tech_Stat", true, 3); s_fn2Audio->SetChannelVolume(3,  CFVolumeToDb(0));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_fansound",          true, 6);  s_fn2Audio->SetChannelVolume(6,  CFVolumeToDb(40));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_deepbreaths",       true, 8);  s_fn2Audio->SetChannelVolume(8,  CFVolumeToDb(0));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_stare",             true, 9);  s_fn2Audio->SetChannelVolume(9,  CFVolumeToDb(0));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_Music_Box_Melody_Playful", true, 13); s_fn2Audio->SetChannelVolume(13, CFVolumeToDb(0));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_elec garble",       true, 16); s_fn2Audio->SetChannelVolume(16, CFVolumeToDb(0));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_jackinthebox",      true, 18); s_fn2Audio->SetChannelVolume(18, CFVolumeToDb(75));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_popstatic",         true, 30); s_fn2Audio->SetChannelVolume(30, CFVolumeToDb(0));
            s_fn2Audio->PlayOnChannel(m_services.pak, "snd_With_S2",           true, 31); s_fn2Audio->SetChannelVolume(31, CFVolumeToDb(0));
        } else if (m_prevAudioScreen == (int)FNaF2Game::SCR_OFFICE) {
            s_fn2Audio->Stop("snd_In_The_Depths_C");   s_fn2Audio->Stop("snd_buzzlight");
            s_fn2Audio->Stop("snd_CMPTR_Low_Tech_Stat"); s_fn2Audio->Stop("snd_fansound");
            s_fn2Audio->Stop("snd_deepbreaths");       s_fn2Audio->Stop("snd_stare");
            s_fn2Audio->Stop("snd_Music_Box_Melody_Playful"); s_fn2Audio->Stop("snd_elec garble");
            s_fn2Audio->Stop("snd_jackinthebox");      s_fn2Audio->Stop("snd_popstatic");
            s_fn2Audio->Stop("snd_With_S2");
        }
        m_prevAudioScreen = scr;
    }
}

void FNaF2Module::Render() {
    switch (m_game.GetScreen()) {
        case FNaF2Game::SCR_DISCLAIMER: m_render.RenderDisclaimer(m_game); break;
        case FNaF2Game::SCR_TITLE:      m_render.RenderTitle(m_game, m_time); break;
        case FNaF2Game::SCR_AD:         m_render.RenderAd(); break;
        case FNaF2Game::SCR_NIGHTSTART: m_render.RenderCard(m_game); break;
        case FNaF2Game::SCR_OFFICE:
            if (m_game.GetViewing() != 0) m_render.RenderMonitor(m_game, m_time, m_time - m_lastSwitchT, m_pan, m_sceneValue, m_lastSceneValue);
            else                          m_render.RenderOffice(m_game, m_time, m_pan, m_sceneValue);
            // v2.59: the jumpscare overlays everything
            if (m_game.GetScareTimer() >= 0.0f) m_render.DrawAttack(m_game);
            break;
        case FNaF2Game::SCR_STATIC:     m_render.RenderStatic(); break;
        case FNaF2Game::SCR_NEXTDAY:    m_render.RenderNextDay(m_game); break;
        case FNaF2Game::SCR_DREAM:      m_render.RenderDream(m_game); break;
        case FNaF2Game::SCR_ERROR:      m_render.RenderError(false); break;
        case FNaF2Game::SCR_ERROR2:     m_render.RenderError(true); break;
        case FNaF2Game::SCR_END5:       m_render.RenderEnd(5); break;
        case FNaF2Game::SCR_END6:       m_render.RenderEnd(6); break;
        case FNaF2Game::SCR_END7:       m_render.RenderEnd(7); break;
        case FNaF2Game::SCR_CUSTOMIZE:  m_render.RenderCustomize(m_game); break;
        case FNaF2Game::SCR_RARE1:      m_render.RenderRare(1); break;
        case FNaF2Game::SCR_RARE2:      m_render.RenderRare(2); break;
        case FNaF2Game::SCR_RARE3:      m_render.RenderRare(3); break;
        case FNaF2Game::SCR_GAMEOVER:   m_render.RenderGameOver(); break;
        case FNaF2Game::SCR_EIGHTBIT:   m_render.RenderEightBit(m_game); break;
        case FNaF2Game::SCR_MGLOAD:     m_render.RenderMgLoad(); break;
        case FNaF2Game::SCR_MG1:
        case FNaF2Game::SCR_MG2:
        case FNaF2Game::SCR_MG3:        m_render.RenderMinigame(m_game); break;
        case FNaF2Game::SCR_ENDBARS:    m_render.RenderEndBars(); break;
        case FNaF2Game::SCR_RAREEXIT:   m_render.RenderEndBars(); break;
    }
}
// ============================================================
//  FNaF3Module — v2.61: the FNAF3 night loop (FNaF3Game) + renderer.
//  Pad map (NO cursor): LS/LT/RT pan the office; LB flips the
//  monitor; on the map D-pad moves the highlight, A = cam,
//  X = lure (room map) / seal (vent map), RB = room<->vent map,
//  Y = maintenance panel (up/down + A), B drops the monitor.
// ============================================================

static AudioSystem* s_fn3Audio = 0;
static PakLoader*   s_fn3Pak   = 0;
static void FNaF3SfxPlay(const char* name, bool loop, i32 channel, i32 volume) {
    if (!s_fn3Audio || !s_fn3Pak || !name) return;
    s_fn3Audio->SetChannelVolume(channel, CFVolumeToDb(volume));
    s_fn3Audio->PlayOnChannel(s_fn3Pak, name, loop, channel);
}
static void FNaF3SfxStop(const char* name) {
    if (s_fn3Audio && name) s_fn3Audio->Stop(name);
}
static void FNaF3ChVol(i32 channel, i32 volume) {
    if (s_fn3Audio) s_fn3Audio->SetChannelVolume(channel, CFVolumeToDb(volume));
}

bool FNaF3Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;
    m_prevAudioScreen = -2;
    m_prevX = false;
    m_render.Init(services.pak, services.batch, services.text);
    s_fn3Audio = services.audio;
    s_fn3Pak   = services.pak;
    m_game.audio.play           = FNaF3SfxPlay;
    m_game.audio.stop           = FNaF3SfxStop;
    m_game.audio.channelVolume  = FNaF3ChVol;
    m_game.ResetToTitle();
    // v2.62: load the dump's own save ("freddy3")
    Progress::GameProgressF3 p3;
    if (Progress::LoadF3(p3)) m_game.ApplyProgressF3(p3);
    return true;
}

void FNaF3Module::Tick(f32 dt) {
    m_time += dt;

    // ---- translate the pad ----
    FNaF3Inputs in;
    const bool xNow = m_services.input ? m_services.input->xHeld : false;
    in.aPressed     = m_services.input ? m_services.input->cameraToggle : false;
    in.bPressed     = m_services.input ? m_services.input->back         : false;
    in.xPressed     = xNow && !m_prevX;
    in.yPressed     = m_services.input ? m_services.input->yToggle      : false;
    in.lbPressed    = m_services.input ? m_services.input->leftLightToggle  : false;
    in.rbPressed    = m_services.input ? m_services.input->rightLightToggle : false;
    in.upPressed    = m_services.input ? m_services.input->cameraUp     : false;
    in.downPressed  = m_services.input ? m_services.input->cameraDown   : false;
    in.leftPressed  = m_services.input ? m_services.input->cameraLeft   : false;
    in.rightPressed = m_services.input ? m_services.input->cameraRight  : false;
    if (m_services.input) {
        in.lookDir = m_services.input->lookDir;
        // the triggers pan too (FNAF2 terms)
        if (m_services.input->leftDoorAxis  > 0.5f) in.lookDir -= 1.0f;
        if (m_services.input->rightDoorAxis > 0.5f) in.lookDir += 1.0f;
    }
    m_prevX = xNow;

    m_game.Tick(dt, in);

    // v2.62: the freddy3 save bridge (the next-day screen flips the bit)
    if (m_game.ConsumeSaveDirty()) {
        Progress::GameProgressF3 p3;
        m_game.FillProgressF3(p3);
        if (Progress::SaveF3(p3))
            printf("FNAF3 SAVE: level=%d\n", p3.level);
    }

    // ---- screen ambience (the office loop set + the night voice) ----
    const int scr = (int)m_game.GetScreen();
    if (scr != m_prevAudioScreen && m_services.audio && m_services.pak) {
        if (scr == (int)FNaF3Game::SCR_TITLE) {
            m_services.audio->Play(m_services.pak, "snd_titlemusic", true, 0.45f);
            m_services.audio->Play(m_services.pak, "snd_static_sound", true, 0.35f);
        } else {
            m_services.audio->Stop("snd_titlemusic");
            m_services.audio->Stop("snd_static_sound");
        }
        if ((scr == (int)FNaF3Game::SCR_OFFICE) &&
            (m_prevAudioScreen == (int)FNaF3Game::SCR_NIGHTSTART ||
             m_prevAudioScreen == (int)FNaF3Game::SCR_WAIT)) {
            // office entry: the fan + the day-start + the night voice
            s_fn3Audio->PlayOnChannel(m_services.pak, "snd_tablefan", true, 1);
            s_fn3Audio->SetChannelVolume(1, CFVolumeToDb(50));
            s_fn3Audio->PlayOnChannel(m_services.pak, "snd_startday", false, 2);
            s_fn3Audio->SetChannelVolume(2, CFVolumeToDb(100));
            static const char* const kVoice[7] = {
                0, "snd_night1final", "snd_night2final2", "snd_night3final",
                "snd_night4final", "snd_night5final", "snd_night6final"
            };
            const i32 n = m_game.GetNight();
            if (n >= 1 && n <= 6 && kVoice[n])
                s_fn3Audio->PlayOnChannel(m_services.pak, kVoice[n], false, 18);
            s_fn3Audio->SetChannelVolume(18, CFVolumeToDb(100));
        } else if (m_prevAudioScreen == (int)FNaF3Game::SCR_OFFICE &&
                   scr != (int)FNaF3Game::SCR_OFFICE) {
            s_fn3Audio->Stop("snd_tablefan");
        }
        m_prevAudioScreen = scr;
    }
}

void FNaF3Module::Render() {
    switch (m_game.GetScreen()) {
        case FNaF3Game::SCR_DISCLAIMER:
            if (m_services.text) {
                m_services.text->DrawText((int)(530.0f * 1.25f), (int)(313.0f * 0.9375f),
                                          "WARNING!", 0xFFFFFFFF);
                m_services.text->DrawText((int)(338.0f * 1.25f), (int)(360.0f * 0.9375f),
                                          "This game contains flashing lights, loud", 0xFFFFFFFF);
                m_services.text->DrawText((int)(390.0f * 1.25f), (int)(388.0f * 0.9375f),
                                          "noises, and lots of jumpscares!", 0xFFFFFFFF);
            }
            break;
        case FNaF3Game::SCR_TITLE:      m_render.RenderTitle(m_time, m_game.GetOptionSelected()); break;
        case FNaF3Game::SCR_NIGHTSTART: m_render.RenderNightStart(m_game.GetNight()); break;
        case FNaF3Game::SCR_OFFICE:
            if (m_game.GetViewing() != 0) m_render.RenderMonitor(m_game, m_time);
            else                          m_render.RenderOffice(m_game, m_time);
            if (m_game.GetScareTimer() > 0.0f) m_render.DrawAttack(m_game);
            break;
        case FNaF3Game::SCR_STATIC6:    m_render.RenderStaticDeath(); break;
        case FNaF3Game::SCR_NEXTDAY:    m_render.RenderNextDay(m_game.GetNight()); break;
        // v2.62: the dump's end screens
        case FNaF3Game::SCR_AD:         m_render.RenderAd(); break;
        case FNaF3Game::SCR_RARE2:      m_render.RenderRare2(); break;
        case FNaF3Game::SCR_ENDCHOOSER: m_render.RenderEndScreen(0); break;
        case FNaF3Game::SCR_ENDBAD:     m_render.RenderEndScreen(1); break;
        case FNaF3Game::SCR_ENDGOOD:    m_render.RenderEndScreen(2); break;
        case FNaF3Game::SCR_END2:       m_render.RenderEndScreen(3); break;
        // v2.63: the new flow screens + the minigames/cutscenes/extras
        case FNaF3Game::SCR_WAIT:       m_render.RenderWait(); break;
        case FNaF3Game::SCR_GAMEOVER:   m_render.RenderGameOver(); break;
        case FNaF3Game::SCR_RARE1:      m_render.RenderRare(1); break;
        case FNaF3Game::SCR_RARE3:      m_render.RenderRare(3); break;
        case FNaF3Game::SCR_LOAD:       m_render.RenderLoad(m_time); break;
        case FNaF3Game::SCR_CUTSCENE:   m_render.RenderCutscene(m_game, m_time); break;
        case FNaF3Game::SCR_MG:         m_render.RenderMinigame(m_game, m_time); break;
        case FNaF3Game::SCR_EXTRAS:     m_render.RenderExtras(m_game, m_time); break;
    }
}

// ============================================================
//  FNaF4Module — v2.61: the FNAF4 bedroom night loop (FNaF4Game).
//  Pad map (NO cursor): D-pad walks the five positions, A (hold)
//  = flashlight peek, X (hold) = shut the door, listening = stand
//  at a door with nothing held, B = out of the bed.
// ============================================================

static AudioSystem* s_fn4Audio = 0;
static PakLoader*   s_fn4Pak   = 0;
static void FNaF4SfxPlay(const char* name, bool loop, i32 channel, i32 volume) {
    if (!s_fn4Audio || !s_fn4Pak || !name) return;
    s_fn4Audio->SetChannelVolume(channel, CFVolumeToDb(volume));
    s_fn4Audio->PlayOnChannel(s_fn4Pak, name, loop, channel);
}
static void FNaF4SfxStop(const char* name) {
    if (s_fn4Audio && name) s_fn4Audio->Stop(name);
}
static void FNaF4ChVol(i32 channel, i32 volume) {
    if (s_fn4Audio) s_fn4Audio->SetChannelVolume(channel, CFVolumeToDb(volume));
}

bool FNaF4Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;
    m_pan = 138.0f;
    m_prevAudioScreen = -2;
    m_render.Init(services.pak, services.batch, services.text);
    s_fn4Audio = services.audio;
    s_fn4Pak   = services.pak;
    m_game.audio.play           = FNaF4SfxPlay;
    m_game.audio.stop           = FNaF4SfxStop;
    m_game.audio.channelVolume  = FNaF4ChVol;
    m_game.ResetToTitle();
    // v2.64: the fn4 save ("fn4") through the universal backend
    {
        Progress::GameProgressF4 p4;
        if (Progress::LoadF4(p4)) m_game.ApplyProgressF4(p4);
    }
    return true;
}

void FNaF4Module::Tick(f32 dt) {
    m_time += dt;

    // ---- translate the pad ----
    FNaF4Inputs in;
    in.aPressed     = m_services.input ? m_services.input->cameraToggle : false;
    in.aHeld        = m_services.input ? m_services.input->aHeld        : false;
    in.xHeld        = m_services.input ? m_services.input->xHeld        : false;
    in.bPressed     = m_services.input ? m_services.input->back         : false;
    in.upPressed    = m_services.input ? m_services.input->cameraUp     : false;
    in.downPressed  = m_services.input ? m_services.input->cameraDown   : false;
    in.leftPressed  = m_services.input ? m_services.input->cameraLeft   : false;
    in.rightPressed = m_services.input ? m_services.input->cameraRight  : false;

    m_game.Tick(dt, in);

    // v2.64: the fn4 save bridge (the night-win / game-over beats flip it)
    if (m_game.ConsumeSaveDirty()) {
        Progress::GameProgressF4 p4;
        m_game.FillProgressF4(p4);
        if (Progress::SaveF4(p4))
            printf("FNAF4 SAVE: night=%d\n", p4.night);
    }

    // ---- the bedroom pan eases toward the current position ----
    f32 target = 138.0f;
    switch (m_game.GetPosition()) {
        case FNaF4Game::P_LEFT:   target =   0.0f; break;
        case FNaF4Game::P_RIGHT:  target = 276.0f; break;
        case FNaF4Game::P_CLOSET: target = 276.0f; break;
        default:                  target = 138.0f; break;
    }
    const f32 k = dt * 8.0f > 1.0f ? 1.0f : dt * 8.0f;
    m_pan += (target - m_pan) * k;

    // ---- screen ambience (the game-managed loops pre-start here) ----
    const int scr = (int)m_game.GetScreen();
    if (scr != m_prevAudioScreen && m_services.audio && m_services.pak) {
        if (scr == (int)FNaF4Game::SCR_TITLE) {
            m_services.audio->Play(m_services.pak, "snd_title", true, 0.3f);
        } else {
            m_services.audio->Stop("snd_title");
        }
        if (scr == (int)FNaF4Game::SCR_BEDROOM) {
            // the loops the game's volume hooks move (breathing/kitchen/
            // minimonsters/fredbear) + the night crickets
            s_fn4Audio->PlayOnChannel(m_services.pak, "snd_breathing1",  true, 20); s_fn4Audio->SetChannelVolume(20, CFVolumeToDb(0));
            s_fn4Audio->PlayOnChannel(m_services.pak, "snd_kitchen",     true, 19); s_fn4Audio->SetChannelVolume(19, CFVolumeToDb(0));
            s_fn4Audio->PlayOnChannel(m_services.pak, "snd_minimonsters",true, 21); s_fn4Audio->SetChannelVolume(21, CFVolumeToDb(0));
            s_fn4Audio->PlayOnChannel(m_services.pak, "snd_fredbear",    true, 27); s_fn4Audio->SetChannelVolume(27, CFVolumeToDb(0));
            s_fn4Audio->PlayOnChannel(m_services.pak, "snd_crickets",    true, 0);  s_fn4Audio->SetChannelVolume(0,  CFVolumeToDb(25));
        } else if (m_prevAudioScreen == (int)FNaF4Game::SCR_BEDROOM) {
            s_fn4Audio->Stop("snd_breathing1"); s_fn4Audio->Stop("snd_kitchen");
            s_fn4Audio->Stop("snd_minimonsters"); s_fn4Audio->Stop("snd_fredbear");
            s_fn4Audio->Stop("snd_crickets");
        }
        m_prevAudioScreen = scr;
    }
}

void FNaF4Module::Render() {
    switch (m_game.GetScreen()) {
        case FNaF4Game::SCR_DISCLAIMER:
            // FNAF4's warning is RED (frame 0 "Frame 17")
            if (m_services.text) {
                m_services.text->DrawText((int)(465.0f * 1.25f), (int)(290.0f * 0.9375f),
                                          "WARNING!", 0xFF2020E0);
                m_services.text->DrawText((int)(255.0f * 1.25f), (int)(365.0f * 0.9375f),
                                          "THIS GAME CONTAINS FLASHING LIGHTS, LOUD", 0xFF2020E0);
                m_services.text->DrawText((int)(298.0f * 1.25f), (int)(393.0f * 0.9375f),
                                          "NOISES, AND LOTS OF JUMPSCARES!", 0xFF2020E0);
            }
            break;
        case FNaF4Game::SCR_TITLE:
            m_render.RenderTitle(m_time, m_game.GetOptionSelected(), m_game.IsBeat5());
            break;
        case FNaF4Game::SCR_NIGHTSTART: m_render.RenderNightStart(m_game.GetNight()); break;
        case FNaF4Game::SCR_BEDROOM:
            m_render.RenderBedroom(m_game, m_time, m_pan);
            if (m_game.GetAttackT() > 0.0f || m_game.GetBiteT() > 0.0f)
                m_render.DrawAttack(m_game);
            break;
        case FNaF4Game::SCR_NIGHTWIN:
            m_render.RenderNightWin();
            m_render.RenderNightWinDigits(m_game);
            break;
        // v2.64: the new flow screens
        case FNaF4Game::SCR_GAMEOVER:   m_render.RenderGameOver(); break;
        case FNaF4Game::SCR_GAMEOVER2:  m_render.RenderGameOver2(); break;
        case FNaF4Game::SCR_INTRO:      m_render.RenderIntro(); break;
        case FNaF4Game::SCR_PLUSH:
        case FNaF4Game::SCR_BB:         m_render.RenderMinigame(m_game); break;
        case FNaF4Game::SCR_LOCKBOX:
        case FNaF4Game::SCR_LOADX:      m_render.RenderLockbox(m_game); break;
        case FNaF4Game::SCR_EXTRAS:     m_render.RenderExtras(m_game); break;
        case FNaF4Game::SCR_CUTSCENE:   m_render.RenderCutscene(m_game); break;
        case FNaF4Game::SCR_ENDING:     m_render.RenderEnding(m_game); break;
        case FNaF4Game::SCR_TEST:       break;
        case FNaF4Game::SCR_NJSCARE:    break;
    }
}

// ============================================================
//  SLModule — v2.65: Sister Location wave 1 live (SLGame + SLRender).
//  The 1.5 GB pak boots through LoadStreaming; PreloadAsync rides the
//  room-to-room transitions (the go-to router arms a room; its textures
//  stream in on first use). Pad map (NO cursor): LS/W = walk hold (the
//  "crawl" model), RT = quick/loud walk, A = confirm/shock, X = the flash,
//  B = back, D-pad = menus. Sounds run through the shared channels.
// ============================================================

static AudioSystem* s_slAudio = 0;
static PakLoader*   s_slPak   = 0;
static void SLSfxPlay(const char* name, bool loop, i32 channel, i32 volume) {
    if (!s_slAudio || !s_slPak || !name) return;
    s_slAudio->SetChannelVolume(channel, CFVolumeToDb(volume));
    s_slAudio->PlayOnChannel(s_slPak, name, loop, channel);
}
static void SLSfxStop(const char* name) {
    if (s_slAudio && name) s_slAudio->Stop(name);
}
static void SLSfxPan(i32 channel, i32 pan) {
    // Labeled: AudioSystem has no per-channel pan yet — the Ballora side
    // cue rides the volume via the game loop instead (dump deviation).
    (void)channel; (void)pan;
}

bool SLModule::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;
    m_prevAudioScreen = -2;
    m_prevScreen = -2;
    m_prevX = false;
    m_render.Init(services.pak, services.batch, services.text);
    s_slAudio = services.audio;
    s_slPak   = services.pak;
    m_game.audio.play           = SLSfxPlay;
    m_game.audio.stop           = SLSfxStop;
    m_game.audio.channelVolume  = 0;
    m_game.audio.pan            = SLSfxPan;
    // v2.65: the fnaf_sl save ("sl") through the universal backend
    {
        Progress::GameProgressSL psl;
        if (Progress::LoadSL(psl)) m_game.ApplyProgressSL(psl);
    }
    m_game.ResetToTitle();
    return true;
}

void SLModule::Tick(f32 dt) {
    m_time += dt;

    // ---- translate the pad ----
    SLInputs in;
    const bool xNow = m_services.input ? m_services.input->xHeld : false;
    in.aPressed   = m_services.input ? m_services.input->cameraToggle : false;
    in.bPressed   = m_services.input ? m_services.input->back         : false;
    in.xPressed   = xNow && !m_prevX;
    in.yPressed   = m_services.input ? m_services.input->yToggle      : false;
    in.lbPressed  = m_services.input ? m_services.input->leftLightToggle  : false;
    in.rbPressed  = m_services.input ? m_services.input->rightLightToggle : false;
    in.upPressed    = m_services.input ? m_services.input->cameraUp     : false;
    in.downPressed  = m_services.input ? m_services.input->cameraDown   : false;
    in.leftPressed  = m_services.input ? m_services.input->cameraLeft   : false;
    in.rightPressed = m_services.input ? m_services.input->cameraRight  : false;
    if (m_services.input) {
        in.lookDir    = m_services.input->lookDir;
        in.flasherPressed = in.xPressed;                  // X = the flash
        in.wHeld      = m_services.input->aHeld;          // A hold walks
        in.shiftHeld  = (m_services.input->rightDoorAxis > 0.5f);  // RT
    }
    m_prevX = xNow;

    // v2.65: the title X-hold wipes the fnaf_sl save (the Warning frame's
    // Delete beat per the dump)
    if (m_game.GetScreen() == SLGame::SCR_TITLE && m_services.input &&
        m_services.input->xHeld) {
        m_xHoldT += dt;
        if (m_xHoldT >= 5.0f) {
            m_xHoldT = 0.0f;
            Progress::WipeSL();
            Progress::GameProgressSL pspec;
            Progress::ResetSL(pspec);
            m_game.ApplyProgressSL(pspec);
            printf("SL SAVE WIPED (X hold)\n");
        }
    } else {
        m_xHoldT = 0.0f;
    }

    m_game.Tick(dt, in);

    // the fnaf_sl save bridge
    if (m_game.ConsumeSaveDirty()) {
        Progress::GameProgressSL psl;
        m_game.FillProgressSL(psl);
        if (Progress::SaveSL(psl))
            printf("SL SAVE: night=%d\n", psl.current);
    }

    // ---- screen-to-screen ambience ----
    const int scr = (int)m_game.GetScreen();
    if (scr != m_prevAudioScreen && m_services.audio && m_services.pak) {
        if (scr == (int)SLGame::SCR_TITLE) {
            m_services.audio->Play(m_services.pak, "snd_Gradual Liquidation", true, 0.3f);
        }
        m_prevAudioScreen = scr;
    }
}

void SLModule::Render() {
    switch (m_game.GetScreen()) {
        case SLGame::SCR_WARNING:   m_render.RenderWarning(); break;
        case SLGame::SCR_TITLE:     m_render.RenderTitle(m_game, m_time); break;
        case SLGame::SCR_ELEVATOR:  m_render.RenderElevator(m_game, m_time); break;
        case SLGame::SCR_VENT:      m_render.RenderVent(m_game); break;
        case SLGame::SCR_HUB:       m_render.RenderHub(m_game); break;
        case SLGame::SCR_BABY:      m_render.RenderBaby(m_game); break;
        case SLGame::SCR_BALLORA:   m_render.RenderBallora(m_game, m_time); break;
        case SLGame::SCR_BREAKER:
        case SLGame::SCR_PS:
        case SLGame::SCR_PS2:
        case SLGame::SCR_UNDERDESK: m_render.RenderBreaker(m_game); break;
        case SLGame::SCR_FUNTIME:   m_render.RenderFuntime(m_game, m_time); break;
        case SLGame::SCR_WINNIGHT:  m_render.RenderWinNight(m_game); break;
        case SLGame::SCR_TVSHOW:    m_render.RenderTvShow(); break;
        case SLGame::SCR_GIRLVOICE: m_render.RenderGirlVoice(); break;
        case SLGame::SCR_DEATH:     m_render.RenderDeath(); break;
        case SLGame::SCR_GAMEOVER:  m_render.RenderGameOver(); break;
        default:                    m_render.RenderHold("the wave-2 room"); break;
    }
}

} // namespace fnaf
