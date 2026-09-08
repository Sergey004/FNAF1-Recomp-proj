/**
 * Five Nights at Freddy's 1 — Recompilation
 * Achievements.cpp: in-game achievement system (v2.14)
 *
 * See Achievements.h for the design. The 10 achievements below mirror
 * achievements.xml 1:1 (id / gamerscore / name / description / icon / secret).
 */

#include "Achievements.h"
#include "Progress.h"
#include <cstdio>

// ---------------------------------------------------------------------------
// BUILD SWITCH — two flavors (v2.20, INVERTED from v2.14):
//   default (no macro)      -> "обычная": touches the Xbox system. Saves/ach
//      go to the signed-in profile (XContent + XUserWriteAchievements), and Y
//      opens the SYSTEM achievements list (XShowAchievementsUI).
//   FNAF_LIVE_SAFE defined  -> "Live Safe": does NOT touch the system. Local
//      files under "save\" next to the .xex; the in-game UI (toast + screen)
//      is the whole experience.
// Toggle the local build by defining FNAF_LIVE_SAFE below (or via /D on the
// compiler command line). Leave it out for the default system build:
// #define FNAF_LIVE_SAFE
// ---------------------------------------------------------------------------

#if !defined(FNAF_LIVE_SAFE)
#include <xtl.h>      // XUserWriteAchievements / XShowAchievementsUI (xbox.h)
#endif

namespace fnaf {

static const float kToastSeconds = 4.0f;

static const AchievementDef kAchievements[Achievements::COUNT] = {
    { 1,   20, "One Night at Freddy's",   "Survive your first night on the job.",              "ach_night1.png",    false },
    { 2,   20, "Two Nights at Freddy's",  "Survive a second night.",                           "ach_night2.png",    false },
    { 3,   30, "Three Nights at Freddy's","Survive a third night.",                            "ach_night3.png",    false },
    { 4,   30, "Four Nights at Freddy's", "Survive a fourth night.",                           "ach_night4.png",    false },
    { 5,   50, "Five Nights at Freddy's", "Survive all five nights.",                          "ach_night5.png",    false },
    { 6,   50, "Overtime",                "Survive the sixth night.",                          "ach_night6.png",    true  },
    { 7,  100, "No Tampering",            "Complete Custom Night with AI set to 20/20/20/20.","ach_420.png",       true  },
    { 8,   30, "No Running",              "Prevent Foxy from leaving Pirate Cove on Night 4.", "ach_foxy.png",      false },
    { 9,   30, "No Laughing",             "Keep Freddy from reaching the East Hall on Night 5.","ach_freddy.png",   false },
    { 10,  20, "No Hiding",               "Get caught by an animatronic.",                     "ach_jumpscare.png", true  }
};

Achievements::Achievements()
    : m_unlocked(0)
    , m_foxyRan(false)
    , m_freddyEast(false)
    , m_night(0)
    , m_toastId(-1)
    , m_toastTime(0.0f)
{
}

void Achievements::Init() {
    u32 bits = 0;
    if (Progress::LoadAchieve(&bits)) m_unlocked = bits;
    printf("Achievements: unlocked=0x%03X\n", (unsigned)m_unlocked);
}

bool Achievements::IsUnlocked(int id) const {
    if (id < 1 || id > COUNT) return false;
    return (m_unlocked & (1u << (id - 1))) != 0;
}

void Achievements::BeginNight(int night) {
    m_night = night;
    m_foxyRan = false;
    m_freddyEast = false;
}

void Achievements::OnFoxyRan()    { m_foxyRan = true; }
void Achievements::OnFreddyEast() { m_freddyEast = true; }

void Achievements::OnNightComplete(int night, bool perfect) {
    if (night >= 1 && night <= 5) {
        Unlock(night);                       // survive nights 1..5 -> ids 1..5
    } else if (night == 6) {
        Unlock(6);                           // Overtime
    } else if (night == 7 && perfect) {
        Unlock(7);                           // No Tampering (20/20/20/20)
    }
    if (night == 4 && !m_foxyRan)    Unlock(8);   // No Running
    if (night == 5 && !m_freddyEast) Unlock(9);   // No Laughing
}

void Achievements::OnJumpscare() {
    Unlock(10);
}

// v2.17 DEV helpers (no toast/system-write spam — just flip the bitmask).
void Achievements::UnlockAll() {
    m_unlocked = 0;
    for (int i = 0; i < COUNT; ++i) m_unlocked |= (1u << i);
    Save();
    m_toastId = -1;
    m_toastTime = 0.0f;
}

void Achievements::ClearAll() {
    m_unlocked = 0;
    Save();
    m_toastId = -1;
    m_toastTime = 0.0f;
}

void Achievements::Unlock(int id) {
    if (id < 1 || id > COUNT) return;
    if (IsUnlocked(id)) return;               // already earned: no rewrite/toast
    m_unlocked |= (1u << (id - 1));
    Save();
    SystemWrite(id);
    m_toastId = id;
    m_toastTime = kToastSeconds;
}

const AchievementDef& Achievements::Get(int i) const {
    if (i < 0) i = 0;
    if (i >= COUNT) i = COUNT - 1;
    return kAchievements[i];
}

int Achievements::UnlockedCount() const {
    int n = 0;
    for (int i = 0; i < COUNT; ++i) if (IsUnlocked(i + 1)) ++n;
    return n;
}

int Achievements::TotalGamerscore() const {
    int gs = 0;
    for (int i = 0; i < COUNT; ++i) if (IsUnlocked(i + 1)) gs += kAchievements[i].gamerscore;
    return gs;
}

void Achievements::Tick(float dt) {
    if (m_toastId >= 0) {
        m_toastTime -= dt;
        if (m_toastTime <= 0.0f) {
            m_toastId = -1;
            m_toastTime = 0.0f;
        }
    }
}

const char* Achievements::ToastName() const {
    if (m_toastId < 1 || m_toastId > COUNT) return "";
    return kAchievements[m_toastId - 1].name;
}

int Achievements::ToastGamerscore() const {
    if (m_toastId < 1 || m_toastId > COUNT) return 0;
    return kAchievements[m_toastId - 1].gamerscore;
}

void Achievements::Save() {
    Progress::SaveAchieve(m_unlocked);
}

void Achievements::SystemWrite(int id) {
#if !defined(FNAF_LIVE_SAFE)
    // System build: mark the achievement earned on the signed-in profile. The
    // name/GS/icon shown by the Guide come from the title's SPA config, but the
    // earned flag is written by this call (works on devkit/LIVE and RGH/JTAG).
    XUSER_ACHIEVEMENT a;
    a.dwUserIndex     = 0;             // first controller (single-profile console)
    a.dwAchievementId = (DWORD)id;
    DWORD res = XUserWriteAchievements(1, &a, NULL);
    printf("ACH %d -> 0x%08X\n", id, (unsigned)res);
#else
    // Live Safe build: no Xbox profile write; the local save\fnaf_ach.ini +
    // in-game UI are the entire record.
    (void)id;
    printf("ACH %d (local only)\n", id);
#endif
}

bool Achievements::ShowSystemUI() {
#if !defined(FNAF_LIVE_SAFE)
    // System build: open the Xbox Guide "Achievements" list for this title.
    XShowAchievementsUI(0);
    return true;
#else
    // Live Safe build: no system UI — caller falls back to the in-game screen.
    return false;
#endif
}

} // namespace fnaf