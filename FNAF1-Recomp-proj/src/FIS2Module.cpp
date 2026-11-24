/**
 * FIS2Module.cpp: v2.108 — Freddy in Space 2 AppModule implementation.
 *
 * The FNaFWorldModule pattern: file-scope audio trampolines (static
 * functions with block scope are illegal — the v2.84 build break), services
 * recorded once, GameInput translated to the game's plain input struct, the
 * save bridge on the game's dirty flag, the fade overlay drawn LAST.
 */

#include "FIS2Module.h"
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
static AudioSystem* s_fisAudio = 0;
static PakLoader*   s_fisPak   = 0;

static void FIS_SfxPlay(const char* name, bool loop, i32 channel, i32 volume) {
    if (!s_fisAudio || !s_fisPak || !name) return;
    s_fisAudio->SetChannelVolume(channel, CFVolumeToDb(volume));
    s_fisAudio->PlayOnChannel(s_fisPak, name, loop, channel);
}
static void FIS_SfxStop(const char* name) {
    if (s_fisAudio && name) s_fisAudio->Stop(name);
}
static void FIS_ChVol(i32 channel, i32 volume) {
    if (s_fisAudio) s_fisAudio->SetChannelVolume(channel, CFVolumeToDb(volume));
}

// ------------------------------------------------------------

bool FIS2Module::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;

    m_render.Init(services.pak, services.batch, services.text);

    s_fisAudio = services.audio;
    s_fisPak   = services.pak;

    FIS2AudioHooks cb;
    cb.play          = FIS_SfxPlay;
    cb.stop          = FIS_SfxStop;
    cb.channelVolume = FIS_ChVol;
    m_game.audio     = cb;

    {
        Progress::GameProgressFIS2 p;
        if (Progress::GameProgressFIS2::Load(p)) m_game.ApplyProgress(p);
    }
    m_game.ResetToBoot();
    return true;
}

void FIS2Module::Unload() {
    if (m_game.IsSaveDirty()) {
        Progress::GameProgressFIS2 p;
        p.stageSel = m_game.GetStageSel();
        Progress::GameProgressFIS2::Save(p);
        m_game.ClearSaveDirty();
    }
}

void FIS2Module::Tick(f32 dt) {
    m_time += dt;

    FIS2Inputs in;
    if (m_services.input) {
        in.aPressed     = m_services.input->cameraToggle;
        in.bPressed     = m_services.input->back;
        in.upPressed    = m_services.input->cameraUp;
        in.downPressed  = m_services.input->cameraDown;
        in.leftPressed  = m_services.input->cameraLeft;
        in.rightPressed = m_services.input->cameraRight;
        in.startPressed = m_services.input->pause;
    }

    m_game.Tick(dt, in);
    if (m_game.WantsExit()) m_wantsExit = true;
}

void FIS2Module::Render() {
    switch (m_game.GetScreen()) {
        case FIS2_TITLE:       m_render.RenderTitle(m_game);       break;
        case FIS2_STAGE_SELECT:m_render.RenderStageSelect(m_game); break;
        case FIS2_LEVEL_FOXY:
        case FIS2_LEVEL_CHICA:
        case FIS2_LEVEL_BONNIE:
        case FIS2_LEVEL_PUPPET:
        case FIS2_LEVEL_FINAL_BOSS:
        case FIS2_LEVEL_SECRET_BOSS:
                                m_render.RenderLevel(m_game);      break;
        default:               m_render.DrawScene(m_game.GetScreen()); break;
    }
    // the CFT frame transition — LAST, over everything
    const f32 a = m_game.GetFadeAlpha();
    if (a > 0.0f && m_services.batch)
        m_services.batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f,
                                    ((u32)(a * 255.0f) << 24) | 0x00FFFFFFu);
}

} // namespace fnaf
