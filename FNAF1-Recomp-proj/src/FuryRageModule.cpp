/**
 * FuryRageModule.cpp: v2.108 — Fury's Rage AppModule implementation.
 *
 * The FNaFWorldModule pattern: file-scope audio trampolines (static
 * functions with block scope are illegal — the v2.84 build break), services
 * recorded once, GameInput translated to the game's plain input struct, the
 * save bridge on the game's dirty flag, the fade overlay drawn LAST.
 */

#include "FuryRageModule.h"
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
static AudioSystem* s_furyAudio = 0;
static PakLoader*   s_furyPak   = 0;

static void FURY_SfxPlay(const char* name, bool loop, i32 channel, i32 volume) {
    if (!s_furyAudio || !s_furyPak || !name) return;
    s_furyAudio->SetChannelVolume(channel, CFVolumeToDb(volume));
    s_furyAudio->PlayOnChannel(s_furyPak, name, loop, channel);
}
static void FURY_SfxStop(const char* name) {
    if (s_furyAudio && name) s_furyAudio->Stop(name);
}
static void FURY_ChVol(i32 channel, i32 volume) {
    if (s_furyAudio) s_furyAudio->SetChannelVolume(channel, CFVolumeToDb(volume));
}

// ------------------------------------------------------------

bool FuryRageModule::Load(AppServices& services) {
    m_services = services;
    m_wantsExit = false;
    m_time = 0.0f;

    m_render.Init(services.pak, services.batch, services.text);

    s_furyAudio = services.audio;
    s_furyPak   = services.pak;

    FuryRageAudioHooks cb;
    cb.play          = FURY_SfxPlay;
    cb.stop          = FURY_SfxStop;
    cb.channelVolume = FURY_ChVol;
    m_game.audio     = cb;

    {
        Progress::GameProgressFuryRage p;
        if (Progress::GameProgressFuryRage::Load(p)) m_game.ApplyProgress(p);
    }
    m_game.ResetToStart();
    return true;
}

void FuryRageModule::Unload() {
    if (m_game.IsSaveDirty()) {
        Progress::GameProgressFuryRage p;
        p.coins[0] = m_game.GetCoin(0);
        p.coins[1] = m_game.GetCoin(1);
        p.coins[2] = m_game.GetCoin(2);
        p.hardMode = m_game.GetHardMode() ? 2 : 0;
        p.character = m_game.GetCharSel();
        p.level = 1;
        p.playsel = m_game.GetTitleSel();
        Progress::GameProgressFuryRage::Save(p);
        m_game.ClearSaveDirty();
    }
}

void FuryRageModule::Tick(f32 dt) {
    m_time += dt;

    FuryRageInputs in;
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

void FuryRageModule::Render() {
    switch (m_game.GetScreen()) {
        case FR_TITLE:            m_render.RenderTitle(m_game);            break;
        case FR_CHARACTER_SELECT: m_render.RenderCharacterSelect(m_game); break;
        case FR_LEVELS:           m_render.RenderLevels(m_game);           break;
        default:                                m_render.DrawScene(m_game.GetScreen());   break;
    }
    // the CFT frame transition — LAST, over everything
    const f32 a = m_game.GetFadeAlpha();
    if (a > 0.0f && m_services.batch)
        m_services.batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f,
                                    ((u32)(a * 255.0f) << 24) | 0x00FFFFFFu);
}

} // namespace fnaf
