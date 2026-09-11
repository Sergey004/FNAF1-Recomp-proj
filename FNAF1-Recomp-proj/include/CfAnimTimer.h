/**
 * Five Nights at Freddy's 1 — Recompilation
 * CfAnimTimer.h: Clickteam Fusion 2.5 animation timer, extracted into a
 * reusable helper so the renderer doesn't keep re-implementing the same
 * "accumulator + frame index" pattern inline.
 *
 * CF's "Fps" field (called `speed` here, 0..100) advances one frame every
 * (100 / speed) GAME frames. The original game runs at 60 FPS, so the frame
 * period in seconds is:
 *       (100 / speed) / 60   ==   1 / (speed * 0.6)
 * i.e. the animation framerate is speed * 0.6 (speed 50 -> 30 FPS,
 * 70 -> 42 FPS, 75 -> 45 FPS, 99 -> ~59.4 FPS).
 *
 * Compatible with C++03 / VS2010 (no lambdas, no nullptr).
 */

#ifndef FNAF_CF_ANIM_TIMER_H
#define FNAF_CF_ANIM_TIMER_H

#include "Types.h"

namespace fnaf {

// Seconds per frame for a CF animation `speed` (0..100). Returns 0 for
// invalid speeds (callers treat period <= 0 as "stop/inert").
static inline f32 CfFramePeriod(int speed) {
    if (speed <= 0) return 0.0f;
    return (100.0f / (f32)speed) / 60.0f;
}

// Stateless lookup: frame index of a `frameCount`-frame animation at time
// `elapsed` seconds. loop=true wraps (CF repeat 0, e.g. blinking REC/map);
// loop=false plays once and holds the last frame (CF repeat >= 1, e.g.
// jumpscares).
static inline int CfAnimFrame(int speed, f32 elapsed, int frameCount, bool loop) {
    if (frameCount <= 0) return 0;
    const f32 period = CfFramePeriod(speed);
    if (period <= 0.0f) return 0;
    int f = (int)(elapsed / period);
    if (f < 0) f = 0;
    if (loop) return f % frameCount;
    if (f > frameCount - 1) f = frameCount - 1;
    return f;
}

// Stateful Clickteam-style timer for the office FX that start/advance/stop
// across frames (static grain, tablet wipe/flash/raise). An idle timer has
// t < 0, brought to life by Start(), advanced with Tick(dt).
struct CfAnimTimer {
    f32  t;      // elapsed seconds since Start(); < 0 = idle
    int  frame;  // current frame index
    int  count;  // number of frames in the animation
    f32  period; // seconds per frame
    bool loop;   // true = wrap around, false = play once then idle

    CfAnimTimer() : t(-1.0f), frame(0), count(0), period(0.0f), loop(true) {}

    // One-time configuration. A configured-but-unstarted timer stays idle.
    void Configure(f32 periodSec, int frameCount, bool loop_) {
        period = periodSec;
        count  = frameCount;
        loop   = loop_;
        t      = -1.0f;
        frame  = 0;
    }

    // Configure by the raw CF speed value (0..100).
    void SetSpeed(int speed, int frameCount, bool loop_) {
        Configure(CfFramePeriod(speed), frameCount, loop_);
    }

    void Start() {
        if (count > 0 && period > 0.0f) { t = 0.0f; frame = 0; }
        else                            { t = -1.0f; frame = 0; }
    }
    void Stop() { t = -1.0f; }           // idle; frame position discarded

    bool Active() const { return t >= 0.0f; }

    // Advance by dt seconds. A non-looping animation parks on its last frame
    // and goes idle the moment the final frame index is crossed (CF's "on
    // animation finished" -> destroy the object).
    void Tick(f32 dt) {
        if (t < 0.0f) return;
        if (period <= 0.0f || count <= 0) { t = -1.0f; return; }
        t += dt;
        while (t >= period) {
            t -= period;
            if (frame < count - 1) {
                ++frame;
            } else if (loop) {
                frame = 0;
            } else {
                t = -1.0f;              // finished: idle, hold last frame
                break;
            }
        }
    }

    int Frame() const { return frame; }

    // True once a one-shot animation has finished (and auto-idled).
    bool Done() const { return !loop && t < 0.0f && frame >= count - 1 && count > 0; }
};

} // namespace fnaf

#endif // FNAF_CF_ANIM_TIMER_H