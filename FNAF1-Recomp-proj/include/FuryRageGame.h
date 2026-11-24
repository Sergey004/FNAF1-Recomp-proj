/**
 * FuryRageGame.h: v2.108 — Fury's Rage module game logic.
 *
 * Clean-room port of the original Fury's Rage flow (the ctfak dump of
 * furys-rage.exe, build 293): the screen graph is the dump's
 * TRANSITIONS.txt —
 *
 *   frame 0 "start"            (the Ini/save read)  -> "Frame 5" | "title screen"
 *   frame 1 "Frame 5"          (the boot hold)      -> "title screen"
 *   frame 2 "title screen"                            -> "character select"
 *   frame 3 "character select"                        -> "levels"
 *   frame 4 "levels"         (the beat-em-up world) -> "ending 2" | the menu
 *   frame 5 "ending"                                -> "title screen" | "character select"
 *   frame 6 "ending 2"                              -> "title screen" | exit
 *
 * The art of every screen is dump-derived (FuryRageScenes.h, generated from
 * the FrameLayout files); the logic here mirrors the events: the title shows
 * the hard-mode badge only when the save's "hard mode?" counter is 2, the
 * three coins follow the save's c1/c2/c3 counts and the title selection is
 * the dump's "playsel" counter.
 *
 * VS2010/C++03: no nullptr, no lambdas, no std::string.
 */

#ifndef FNAF_FURYRAGE_GAME_H
#define FNAF_FURYRAGE_GAME_H

#include "Types.h"
#include "Progress.h"
#include "ScreenFade.h"

namespace fnaf {

// ------------------------------------------------------------
// Input snapshot — translated from GameInput by the module.
// ------------------------------------------------------------
struct FuryRageInputs {
    bool aPressed;        // A edge  — confirm
    bool bPressed;        // B edge  — back
    bool upPressed, downPressed, leftPressed, rightPressed;
    bool startPressed;
    FuryRageInputs() : aPressed(false), bPressed(false), upPressed(false),
                       downPressed(false), leftPressed(false), rightPressed(false),
                       startPressed(false) {}
};

// Audio hooks the module fills (all optional).
struct FuryRageAudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    FuryRageAudioHooks() : play(0), stop(0), channelVolume(0) {}
};

// The dump's screens (the frame indices of furys-rage.exe).
enum FuryRageScreen {
    FR_START = 0,
    FR_FRAME5 = 1,
    FR_TITLE = 2,
    FR_CHARACTER_SELECT = 3,
    FR_LEVELS = 4,
    FR_ENDING = 5,
    FR_ENDING2 = 6,
    FR_SCREEN_COUNT
};

class FuryRageGame {
public:
    enum {
        CHAR_FREDDY = 0,
        CHAR_BONNIE = 1,
        CHAR_CHICA = 2,
        CHAR_FOXY = 3,
        CHAR_COUNT
    };

    FuryRageGame();

    // the module owns the services; the game only talks through the hooks
    FuryRageAudioHooks audio;

    void ResetToStart();

    void Tick(f32 dt, const FuryRageInputs& in);

    // what the renderer needs
    int  GetScreen() const { return m_screen; }
    int  GetTitleSel() const { return m_titleSel; }
    int  GetCharSel() const { return m_charSel; }
    bool GetHardMode() const { return m_hardMode; }
    int  GetCoin(int i) const { return (i >= 0 && i < 3) ? m_coins[i] : 0; }
    f32  GetFadeAlpha() const { return m_fade.alpha; }
    bool WantsExit() const { return m_wantsExit; }

    // the save bridge
    bool IsSaveDirty() const { return m_saveDirty; }
    void ClearSaveDirty() { m_saveDirty = false; }
    void ApplyProgress(const Progress::GameProgressFuryRage& p);

private:
    void GoTo(int screen, f32 fadeTime);
    void UpdateTitle(f32 dt, const FuryRageInputs& in);
    void UpdateCharacterSelect(f32 dt, const FuryRageInputs& in);

    int   m_screen;
    int   m_titleSel;      // the dump's "playsel" counter
    int   m_charSel;       // the character-select cursor
    bool  m_hardMode;      // the save's "hard mode?" == 2
    int   m_coins[3];      // the save's c1/c2/c3
    bool  m_wantsExit;
    bool  m_saveDirty;
    f32   m_holdT;         // the title/select confirmation hold
    ScreenFade m_fade;
};

} // namespace fnaf

#endif // FNAF_FURYRAGE_GAME_H
