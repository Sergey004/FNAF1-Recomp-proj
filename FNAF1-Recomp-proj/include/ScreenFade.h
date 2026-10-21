/**
 * ScreenFade.h — the CFT frame transition (STDT/FADE) model (v2.76).
 *
 * Every fade in all five shipped games is the same thing: a BLACK fullscreen
 * overlay whose alpha ramps 0..1 (fade-out) or 1..0 (fade-in). The dump
 * stores them per FRAME as a "Transition" chunk (module STDT / name FADE /
 * moduleFile cctrans.dll, colour [0,0,0,0]) with a duration in raw
 * milliseconds — 560 / 900 / 1010 / 1130 / 2000 / 2030.
 *
 * CFT's screen change plays the OLD frame's fade-out first and only then
 * loads the new frame and plays ITS fade-in. So a screen change is
 * DEFERRED: the old screen keeps being drawn (and keeps running) while it
 * fades to black; the new screen appears out of black afterwards.
 *
 * The fade overlay must be the LAST thing drawn in a frame and must live
 * OUTSIDE BeginSceneCapture/EndSceneCapture — inside the capture the
 * perspective parabola would bend it.
 *
 * Durations per screen: docs/FRAME_FADES.md (the full dump export for all
 * five games + the mapping "our screen -> the dump's frame").
 */
#ifndef FNaf1_SCREEN_FADE_H
#define FNaf1_SCREEN_FADE_H

namespace fnaf {

// phase 0 = idle, 1 = fading out, 2 = fading in.
// alpha  = 0 (transparent) .. 1 (fully black).
// pending = the screen to switch to when the fade-out completes (-1 = none).
struct ScreenFade {
    int   phase;
    float alpha;
    float timer;
    float dur;      // seconds
    int   pending;
    float from;     // the alpha the current fade-out started from (0 = clear)

    void Init() {
        phase = 0; alpha = 0.0f; timer = 0.0f; dur = 0.0f; pending = -1; from = 0.0f;
    }
    void Clear() {
        phase = 0; alpha = 0.0f; timer = 0.0f; dur = 0.0f; pending = -1; from = 0.0f;
    }
    bool  Active() const { return phase != 0; }
    float Alpha() const  { return alpha; }
    // TRUE while a screen change waits for the fade-out to finish — only
    // then may a new request be ignored (the fade-IN must never swallow
    // the input: the new frame is already live in CFT).
    bool  Switching() const { return phase == 1; }

    // Begin the fade-out that precedes a screen change. The switch itself
    // happens when Tick() reports the pending target. The ramp continues
    // from the CURRENT alpha, so a fade-out started during a fade-in does
    // not flash back to clear.
    void StartOut(int target, float durSec) {
        from = alpha; phase = 1; timer = 0.0f; dur = durSec; pending = target;
    }
    // Begin the fade-in of the screen that just appeared.
    void StartIn(float durSec) {
        phase = 2; alpha = 1.0f; timer = 0.0f; dur = durSec; pending = -1; from = 0.0f;
    }

    // Advance the ramp. Returns the screen to switch to when a fade-out has
    // just completed (-1 = nothing to do).
    int Tick(float dt) {
        if (phase == 0) return -1;
        timer += dt;
        if (phase == 1) {                 // fading out: from -> 1
            if (timer >= dur) {
                alpha = 1.0f;
                const int target = pending;
                pending = -1;
                phase = 0;
                return target;
            }
            alpha = from + (1.0f - from) * (timer / dur);
        } else {                          // fading in: 1 -> 0
            if (timer >= dur) {
                phase = 0;
                alpha = 0.0f;
            } else {
                alpha = 1.0f - timer / dur;
            }
        }
        return -1;
    }
};

} // namespace fnaf

#endif // FNaf1_SCREEN_FADE_H
