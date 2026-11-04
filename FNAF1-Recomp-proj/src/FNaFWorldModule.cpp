/**
 * FNaFWorldModule.cpp: v2.90 — FNAF World AppModule implementation.
 *
 * The SLModule pattern: file-scope audio trampolines (static functions with
 * block scope are illegal — the v2.84 build break), services recorded once,
 * GameInput translated to the game's plain input struct, the save bridge on
 * the game's dirty flag, the fade overlay drawn LAST.
 */

#include "FNaFWorldModule.h"
#include "AudioSystem.h"
#include "TextRenderer.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "InputSystem.h"
#include "Progress.h"
#include "XdkCompat.h"
#include <stdio.h>

namespace fnaf {

// ---- audio trampolines (file scope) ------------------------------------
static AudioSystem* s_fwAudio = 0;
static PakLoader*   s_fwPak   = 0;

static void FW_SfxPlay(const char* name, bool loop, i32 channel, i32 volume) {
    if (!s_fwAudio || !s_fwPak || !name) return;
    s_fwAudio->SetChannelVolume(channel, CFVolumeToDb(volume));
    s_fwAudio->PlayOnChannel(s_fwPak, name, loop, channel);
}
static void FW_SfxStop(const char* name) {
    if (s_fwAudio && name) s_fwAudio->Stop(name);
}
static void FW_ChVol(i32 channel, i32 volume) {
    if (s_fwAudio) s_fwAudio->SetChannelVolume(channel, CFVolumeToDb(volume));
}

// v2.78-style streaming preload: the pak is room-sorted by the RecompPack
// (step 2 wires the per-frame index tables; a no-op until then).
static void FW_PreloadFrame(i32 /*frame*/) {
}

// ------------------------------------------------------------

bool FNaFWorldModule::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;
    m_prevScreen = -2;
    m_prevX = false;

    m_render.Init(services.pak, services.batch, services.text);

    s_fwAudio = services.audio;
    s_fwPak   = services.pak;

    FNaFWorldAudioHooks cb;
    cb.play          = FW_SfxPlay;
    cb.stop          = FW_SfxStop;
    cb.channelVolume = FW_ChVol;
    m_game.audio     = cb;
    m_game.preloadFrame = &FW_PreloadFrame;

    // v2.80 convention: the legacy-container migration first, then the load
    Progress::MigrateLegacySave("fnafworld");
    {
        Progress::GameProgressFW pw;
        if (Progress::LoadFW(pw)) m_game.ApplyProgress(pw);
    }
    m_game.ResetToTitle();
    return true;
}

void FNaFWorldModule::Unload() {
    // flush the save if the game left it dirty
    if (m_game.ConsumeSaveDirty()) {
        Progress::GameProgressFW pw;
        m_game.FillProgress(pw);
        Progress::SaveFW(pw);
    }
}

void FNaFWorldModule::Tick(f32 dt) {
    m_time += dt;

    // ---- translate the pad (real GameInput fields only) ----
    FNaFWorldInputs in;
    in.aPressed = m_services.input ? m_services.input->cameraToggle : false;
    in.aHeld    = m_services.input ? m_services.input->aHeld        : false;
    in.bPressed = m_services.input ? m_services.input->back         : false;
    in.yPressed = m_services.input ? m_services.input->yToggle      : false;
    in.menuPressed = m_services.input ? m_services.input->pause     : false;  // v2.91: START = the world menu
    in.upPressed    = m_services.input ? m_services.input->cameraUp    : false;
    in.downPressed  = m_services.input ? m_services.input->cameraDown  : false;
    in.leftPressed  = m_services.input ? m_services.input->cameraLeft  : false;
    in.rightPressed = m_services.input ? m_services.input->cameraRight : false;
    in.xHeld        = m_services.input ? m_services.input->xHeld       : false;
    if (m_services.input) {
        in.lookDir  = m_services.input->lookDir;
        in.lookDirY = m_services.input->lookDirY;
    }

    m_game.Tick(dt, in);

    // the save bridge (the game flips the bit on its write beats)
    if (m_game.ConsumeSaveDirty()) {
        Progress::GameProgressFW pw;
        m_game.FillProgress(pw);
        if (Progress::SaveFW(pw))
            printf("FNAFWORLD SAVE: zone=%d tokens=%d\n", pw.zoneLevel, pw.fazTokens);
    }

    if (m_game.ExitRequested()) m_wantsExit = true;

    // ---- screen ambience (frame fades handled by the game's ScreenFade) ----
    const int scr = (int)m_game.GetScreen();
    if (scr != m_prevScreen) {
        m_prevScreen = scr;
    }
}

void FNaFWorldModule::Render() {
    switch (m_game.GetScreen()) {
        case FNaFWorldGame::SCR_DISCLAIMER: m_render.RenderSplash();             break;
        case FNaFWorldGame::SCR_TITLE:      m_render.RenderTitle(m_game, m_time); break;
        case FNaFWorldGame::SCR_FILESETUP:  m_render.RenderFileSetup(m_game);    break;
        case FNaFWorldGame::SCR_WORLD:      m_render.RenderWorld(m_game);
                                            // v2.91: the world menu rides OVER
                                            if (m_game.GetMenuScreen() != 0)
                                                m_render.RenderMenu(m_game);
                                            break;
        case FNaFWorldGame::SCR_BATTLE:     m_render.RenderBattle(m_game);       break;
        case FNaFWorldGame::SCR_GAMEOVER:   m_render.RenderGameOver();           break;
        default:                            m_render.RenderWorld(m_game);        break;
    }
    // the CFT frame transition — LAST, over everything
    m_render.DrawFade(m_game.GetFadeAlpha());
}

} // namespace fnaf