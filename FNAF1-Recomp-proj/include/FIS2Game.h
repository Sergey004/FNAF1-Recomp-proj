/**
 * FIS2Game.h: v2.108 — Freddy in Space 2 module game logic.
 *
 * Clean-room port of the original Freddy in Space 2 flow (the ctfak dump of
 * fis2.exe, build 292). The screen graph is the dump's TRANSITIONS.txt:
 *
 *   frame 14 "disclaimer"   -> "level complete" (the boot order)
 *   frame 1  "intro"        -> "title screen"
 *   frame 2  "title screen" -> "stage select"
 *   frame 3  "stage select" -> the six platformer levels (Foxy, Chica,
 *                               Bonnie, Puppet, Final Boss, Secret Boss),
 *                               "total", "dedicated" or back to the title
 *   each level              -> the next level or the stage select
 *   frame 10 "level complete" / 12 "THE END" / 13 "credits"
 *
 * The original window is 720x480 (3:2); the port renders 1280x720, so the
 * game is pillarboxed (1080x720, 100 px bars).
 *
 * VS2010/C++03: no nullptr, no lambdas, no std::string.
 */

#ifndef FNAF_FIS2_GAME_H
#define FNAF_FIS2_GAME_H

#include "Types.h"
#include "Progress.h"
#include "ScreenFade.h"

namespace fnaf {

struct FIS2Inputs {
    bool aPressed;        // A edge  — confirm / jump
    bool bPressed;        // B edge  — back
    bool upPressed, downPressed, leftPressed, rightPressed;
    bool startPressed;
    FIS2Inputs() : aPressed(false), bPressed(false), upPressed(false),
                   downPressed(false), leftPressed(false), rightPressed(false),
                   startPressed(false) {}
};

struct FIS2AudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    FIS2AudioHooks() : play(0), stop(0), channelVolume(0) {}
};

// the dump's frames (fis2.exe)
enum FIS2Screen {
    FIS2_DEDICATED = 0,
    FIS2_INTRO = 1,
    FIS2_TITLE = 2,
    FIS2_STAGE_SELECT = 3,
    FIS2_LEVEL_FOXY = 4,
    FIS2_LEVEL_CHICA = 5,
    FIS2_LEVEL_BONNIE = 6,
    FIS2_LEVEL_PUPPET = 7,
    FIS2_LEVEL_FINAL_BOSS = 8,
    FIS2_LEVEL_SECRET_BOSS = 9,
    FIS2_LEVEL_COMPLETE = 10,
    FIS2_TOTAL = 11,
    FIS2_THE_END = 12,
    FIS2_CREDITS = 13,
    FIS2_DISCLAIMER = 14,
    FIS2_SCREEN_COUNT
};

class FIS2Game {
public:
    FIS2Game();

    FIS2AudioHooks audio;

    void ResetToBoot();

    void Tick(f32 dt, const FIS2Inputs& in);

    int  GetScreen() const { return m_screen; }
    int  GetStageSel() const { return m_stageSel; }
    f32  GetFadeAlpha() const { return m_fade.alpha; }
    bool WantsExit() const { return m_wantsExit; }

    bool IsSaveDirty() const { return m_saveDirty; }
    void ClearSaveDirty() { m_saveDirty = false; }
    void ApplyProgress(const Progress::GameProgressFIS2& p);

private:
    void GoTo(int screen, f32 fadeTime);

    int   m_screen;
    int   m_stageSel;     // the stage-select cursor (the dump's counter)
    bool  m_wantsExit;
    bool  m_saveDirty;
    ScreenFade m_fade;
};

} // namespace fnaf

#endif // FNAF_FIS2_GAME_H
