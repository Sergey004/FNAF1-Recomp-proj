/**
 * FuryRageGame.cpp: v2.108 — Fury's Rage module game logic.
 *
 * The screen graph and the counters come from the ctfak dump of
 * furys-rage.exe (build 293): see the header for the transition table.
 * The gameplay (frame 4 "levels") is a beat-'em-up arena; its camera and
 * the fighters are the next milestone — for now the screen shell (the
 * dump's art + the menu flow) runs.
 */

#include "FuryRageGame.h"

namespace fnaf {

FuryRageGame::FuryRageGame()
    : m_screen(FR_START), m_titleSel(0), m_charSel(0), m_hardMode(false),
      m_wantsExit(false), m_saveDirty(false), m_holdT(0.0f) {
    m_coins[0] = m_coins[1] = m_coins[2] = 0;
    m_fade.Init();
}

void FuryRageGame::ResetToStart() {
    m_screen = FR_START;
    m_titleSel = 0;
    m_charSel = 0;
    m_wantsExit = false;
    m_holdT = 0.0f;
    m_fade.Init();
    // the dump's frame 0 "start" holds the Ini extension (the save read)
    // for a beat and then continues to the title
    GoTo(FR_TITLE, 0.0f);
}

void FuryRageGame::ApplyProgress(const Progress::GameProgressFuryRage& p) {
    for (int i = 0; i < 3; ++i) m_coins[i] = p.coins[i];
    m_hardMode = (p.hardMode == 2);
    m_charSel = p.character;
    m_titleSel = p.playsel;
}

void FuryRageGame::GoTo(int screen, f32 fadeTime) {
    if (fadeTime <= 0.0f) {
        m_screen = screen;
        m_fade.Clear();
        return;
    }
    m_fade.phase = 1;          // fade out
    m_fade.alpha = 0.0f;
    m_fade.timer = 0.0f;
    m_fade.dur = fadeTime;
    m_fade.pending = screen;
    m_fade.from = 0.0f;
}

void FuryRageGame::Tick(f32 dt, const FuryRageInputs& in) {
    // the fade-out runs the old screen; the switch happens when it is black
    if (m_fade.phase == 1) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k > 1.0f) ? 1.0f : k;
        if (m_fade.timer >= m_fade.dur) {
            if (m_fade.pending >= 0) m_screen = m_fade.pending;
            m_fade.pending = -1;
            m_fade.phase = 2;      // fade in
            m_fade.timer = 0.0f;
            m_fade.dur = 0.35f;
        }
    } else if (m_fade.phase == 2) {
        m_fade.timer += dt;
        const f32 k = (m_fade.dur > 0.0f) ? (m_fade.timer / m_fade.dur) : 1.0f;
        m_fade.alpha = (k >= 1.0f) ? 0.0f : (1.0f - k);
        if (m_fade.timer >= m_fade.dur) {
            m_fade.phase = 0;
            m_fade.alpha = 0.0f;
        }
    }

    switch (m_screen) {
        case FR_START:
            break;
        case FR_TITLE:
            UpdateTitle(dt, in);
            break;
        case FR_CHARACTER_SELECT:
            UpdateCharacterSelect(dt, in);
            break;
        default:
            break;
    }
}

void FuryRageGame::UpdateTitle(f32 dt, const FuryRageInputs& in) {
    (void)dt;
    // the dump's title: "playsel" picks the entry, A confirms it and the
    // screen continues to the character select (TRANSITIONS: frame 2 ->
    // frame 3).
    if (in.upPressed)   { m_titleSel = (m_titleSel + 2) % 3; m_saveDirty = true; }
    if (in.downPressed) { m_titleSel = (m_titleSel + 1) % 3; m_saveDirty = true; }
    if (in.aPressed) {
        if (m_titleSel == 0) {
            GoTo(FR_CHARACTER_SELECT, 0.35f);
        } else {
            // the other entries leave the game (the dump's <end application>)
            m_wantsExit = true;
        }
    }
    if (in.bPressed) m_wantsExit = true;
}

void FuryRageGame::UpdateCharacterSelect(f32 dt, const FuryRageInputs& in) {
    (void)dt;
    if (in.leftPressed)  { m_charSel = (m_charSel + CHAR_COUNT - 1) % CHAR_COUNT; m_saveDirty = true; }
    if (in.rightPressed) { m_charSel = (m_charSel + 1) % CHAR_COUNT; m_saveDirty = true; }
    if (in.aPressed) {
        GoTo(FR_LEVELS, 0.35f);
    }
    if (in.bPressed) {
        GoTo(FR_TITLE, 0.35f);
    }
}

} // namespace fnaf
