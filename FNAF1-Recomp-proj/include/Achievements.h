/**
 * Five Nights at Freddy's 1 — Recompilation
 * Achievements.h: in-game achievement system (v2.14, v2.20 two-build)
 *
 * The original game is a retail title whose achievements (id, name,
 * description, gamerscore, icon) are baked into an Xbox LIVE SPA game-config
 * produced by the external "Game Configuration" tool; the runtime only calls
 * XUserWriteAchievements to mark an id as earned. This recomp cannot ship a
 * signed SPA, so achievements are implemented IN-GAME, plus an optional system
 * hook:
 *
 *   * a fixed table of 10 achievements (mirrors achievements.xml);
 *   * an unlock bitmask persisted (see Progress.h) -- to fnaf_save:\fnaf_ach.ini
 *     in the system build, or game:\save\fnaf_ach.ini in the Live Safe build;
 *   * v2.20 TWO BUILD FLAVORS controlled by FNAF_LIVE_SAFE in Achievements.cpp
 *     (also gating Progress.cpp's storage backend):
 *       - default (no macro)  -> "обычная": touches the Xbox system --
 *         XUserWriteAchievements on unlock + XShowAchievementsUI (system list)
 *         on Y;
 *       - FNAF_LIVE_SAFE      -> "Live Safe": no system calls; local files and
 *         the in-game UI only.
 *   * a transient on-screen "Achievement Unlocked" toast (drawn by GameRender)
 *     and a title-menu achievements screen (kept as the in-game fallback);
 *
 * This class owns state and logic; rendering lives in GameRender.
 */

#ifndef FNAF_ACHIEVEMENTS_H
#define FNAF_ACHIEVEMENTS_H

#include "Types.h"

namespace fnaf {

struct AchievementDef {
    int  id;              // 1..10
    int  gamerscore;      // 20..100
    const char* name;
    const char* description;
    const char* icon;     // file under game:\achievements_pics\ (informational)
    bool secret;          // shown as a locked mystery until earned
};

class Achievements {
public:
    enum { COUNT = 10 };

    Achievements();

    // Load the saved unlock bitmask. Call AFTER Progress::Load at boot so the
    // XContent device has already been chosen (no second selector dialog).
    void Init();

    // v2.35: hook the on-screen debug console (main's g_debugConsole) — the
    // XDK calls log results somewhere visible on the console (printf goes
    // nowhere there). Optional: NULL = printf-only.
    void SetDebugConsole(class DebugConsole* c) { m_console = c; }

    bool IsUnlocked(int id) const;

    // Night-scoped "prevent X" tracking, reset at the start of each night.
    void BeginNight(int night);
    void OnFoxyRan();
    void OnFreddyEast();

    // Unlock everything a completed night can earn (ids 1..9).
    void OnNightComplete(int night, bool perfect);
    // "No Hiding" (id 10) — caught by an animatronic.
    void OnJumpscare();

    void Unlock(int id);   // idempotent: mark + save + system write + toast

    // v2.17 DEV: unlock/clear the whole set (no toast spam)
    void UnlockAll();
    void ClearAll();

    // v2.20: open the SYSTEM achievements list (XShowAchievementsUI). Returns
    // true if the system UI was invoked (system build); false in the Live Safe
    // build -- the caller should show the in-game screen instead.
    bool ShowSystemUI();

    // ---- UI access ----
    const AchievementDef& Get(int i) const;   // 0..9
    int  UnlockedCount() const;
    int  TotalGamerscore() const;             // sum of unlocked gamerscore

    // ---- toast (drawn by GameRender) ----
    void Tick(float dt);
    bool        HasToast() const       { return m_toastId >= 0; }
    const char* ToastName() const;
    int         ToastGamerscore() const;

private:
    u32   m_unlocked;     // bit (id-1) set = achievement id unlocked
    class DebugConsole* m_console;   // v2.35: on-screen log sink (may be 0)
    bool  m_foxyRan;      // this night
    bool  m_freddyEast;   // this night
    int   m_night;
    int   m_toastId;      // -1 = none
    float m_toastTime;

    void Save();
    void SystemWrite(int id);
};

} // namespace fnaf

#endif // FNAF_ACHIEVEMENTS_H