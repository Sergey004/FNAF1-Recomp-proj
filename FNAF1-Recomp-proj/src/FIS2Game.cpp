/**
 * FIS2Game.cpp: v2.108 — Freddy in Space 2 module game logic.
 *
 * The screen graph and the timings come from the ctfak dump of fis2.exe.
 * The gameplay frames (the six platformer levels) are the next milestone;
 * for now the shell runs: the boot order, the title, the stage select and
 * the level hand-off.
 */

#include "FIS2Game.h"

namespace fnaf {

FIS2Game::FIS2Game()
    : m_screen(FIS2_DISCLAIMER), m_stageSel(0), m_wantsExit(false),
      m_saveDirty(false) {
    m_fade.Init();
}

void FIS2Game::ResetToBoot() {
    m_screen = FIS2_DISCLAIMER;
    m_stageSel = 0;
    m_wantsExit = false;
    m_fade.Init();
    // the dump's frame 14 "disclaimer" continues to the level-complete card
    GoTo(FIS2_LEVEL_COMPLETE, 0.0f);
}

void FIS2Game::ApplyProgress(const Progress::GameProgressFIS2& p) {
    m_stageSel = p.stageSel;
}

void FIS2Game::GoTo(int screen, f32 fadeTime) {
    if (fadeTime <= 0.0f) {
        m_screen = screen;
        m_fade.Clear();
        return;
    }
    m_fade.phase = 1;
    m_fade.alpha = 0.0f;
    m_fade.timer = 0.0f;
    m_fade.dur = fadeTime;
    m_fade.pending = screen;
    m_fade.from = 0.0f;
}

void FIS2Game::Tick(f32 dt, const FIS2Inputs& in) {
    if (m_fade.phase == 1) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k > 1.0f) ? 1.0f : k;
        if (m_fade.timer >= m_fade.dur) {
            if (m_fade.pending >= 0) m_screen = m_fade.pending;
            m_fade.pending = -1;
            m_fade.phase = 2;
            m_fade.timer = 0.0f;
            m_fade.dur = 0.3f;
        }
    } else if (m_fade.phase == 2) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k >= 1.0f) ? 0.0f : (1.0f - k);
        if (m_fade.timer >= m_fade.dur) { m_fade.phase = 0; m_fade.alpha = 0.0f; }
    }

    switch (m_screen) {
        case FIS2_TITLE:
            if (in.aPressed) GoTo(FIS2_STAGE_SELECT, 0.3f);
            if (in.bPressed) m_wantsExit = true;
            break;
        case FIS2_STAGE_SELECT:
            if (in.leftPressed)  { m_stageSel = (m_stageSel + 5) % 6; m_saveDirty = true; }
            if (in.rightPressed) { m_stageSel = (m_stageSel + 1) % 6; m_saveDirty = true; }
            if (in.aPressed) {
                static const int kLevel[6] = {
                    FIS2_LEVEL_FOXY, FIS2_LEVEL_CHICA, FIS2_LEVEL_BONNIE,
                    FIS2_LEVEL_PUPPET, FIS2_LEVEL_FINAL_BOSS,
                    FIS2_LEVEL_SECRET_BOSS
                };
                GoTo(kLevel[m_stageSel], 0.3f);
            }
            if (in.bPressed) GoTo(FIS2_TITLE, 0.3f);
            break;
        default:
            break;
    }
}

} // namespace fnaf
