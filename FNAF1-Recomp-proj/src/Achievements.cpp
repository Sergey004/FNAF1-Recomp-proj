/**
 * Five Nights at Freddy's 1 — Recompilation
 * Achievements.cpp: in-game achievement system (v2.47: official SPA ids)
 *
 * See Achievements.h for the design. The 10 achievements below mirror the
 * official game config: FNAF1-Recomp-proj.xlast --(XLAST)--> the compiled
 * .spa embedded into the XEX as a resource section named by the title id,
 * plus the generated include/FNAF1-Recomp-proj.spa.h whose ACHIEVEMENT_*
 * constants are the ids used below. XUserWriteAchievements with those ids
 * matches what the system already knows about the title from the embedded
 * .spa (names / gamerscore / icons), so no runtime registration is needed.
 */

#include "Achievements.h"
#include "Progress.h"
#include "DebugConsole.h"
#include "FNAF1-Recomp-proj.spa.h"   // v2.47: official ACHIEVEMENT_* ids
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

// v2.47: the ids MUST stay the contiguous range 1..COUNT — the fnaf_ach.ini
// bitmask persists "bit (id-1) set = achievement id earned". These asserts
// pin the official values so that a regenerated spa.h with shifted ids fails
// THIS build loudly instead of silently re-reading someone else's mask.
static_assert(ACHIEVEMENT_ONE_NIGHT_AT_FREDDYS   == 1,  "spa.h id 1 must stay 1");
static_assert(ACHIEVEMENT_TWO_NIGHTS_AT_FREDDYS  == 2,  "spa.h id 2 must stay 2");
static_assert(ACHIEVEMENT_THREE_NIGHTS_AT_FREDDYS== 3,  "spa.h id 3 must stay 3");
static_assert(ACHIEVEMENT_FOUR_NIGHTS_AT_FREDDYS == 4,  "spa.h id 4 must stay 4");
static_assert(ACHIEVEMENT_FIVE_NIGHTS_AT_FREDDYS == 5,  "spa.h id 5 must stay 5");
static_assert(ACHIEVEMENT_OVERTIME               == 6,  "spa.h id 6 must stay 6");
static_assert(ACHIEVEMENT_NO_TAMPERING           == 7,  "spa.h id 7 must stay 7");
static_assert(ACHIEVEMENT_NO_RUNNING             == 8,  "spa.h id 8 must stay 8");
static_assert(ACHIEVEMENT_NO_LAUGHING            == 9,  "spa.h id 9 must stay 9");
static_assert(ACHIEVEMENT_NO_HIDING              == 10, "spa.h id 10 must stay 10");

// v2.47: id/gamerscore mirror the official config (the .xlast project); id 4
// is 50G there (this table used to hand-roll 30 — total is 400 G now). The
// console has no visibility flag for achievements (the config's
// showUnachieved does nothing), so in-game `secret` stays hand-picked:
// 6/7/10 are the hidden ones. Name/description text stays in-game too — the
// config's language strings are placeholders and its friendly names are
// restricted (no apostrophes etc.); reading them via XReadStringsFromSpaFile
// (the SPASTRING_* ids in the spa header) is future work.
static const AchievementDef kAchievements[Achievements::COUNT] = {
    { ACHIEVEMENT_ONE_NIGHT_AT_FREDDYS,    20, "One Night at Freddy's",   "Survive your first night on the job.",               "ach_night1.png",    false },
    { ACHIEVEMENT_TWO_NIGHTS_AT_FREDDYS,   20, "Two Nights at Freddy's",  "Survive a second night.",                            "ach_night2.png",    false },
    { ACHIEVEMENT_THREE_NIGHTS_AT_FREDDYS, 30, "Three Nights at Freddy's","Survive a third night.",                             "ach_night3.png",    false },
    { ACHIEVEMENT_FOUR_NIGHTS_AT_FREDDYS,  50, "Four Nights at Freddy's", "Survive a fourth night.",                            "ach_night4.png",    false },
    { ACHIEVEMENT_FIVE_NIGHTS_AT_FREDDYS,  50, "Five Nights at Freddy's", "Survive all five nights.",                           "ach_night5.png",    false },
    { ACHIEVEMENT_OVERTIME,                50, "Overtime",                "Survive the sixth night.",                           "ach_night6.png",    true  },
    { ACHIEVEMENT_NO_TAMPERING,           100, "No Tampering",            "Complete Custom Night with AI set to 20/20/20/20.",  "ach_420.png",       true  },
    { ACHIEVEMENT_NO_RUNNING,              30, "No Running",              "Prevent Foxy from leaving Pirate Cove on Night 4.",  "ach_foxy.png",      false },
    { ACHIEVEMENT_NO_LAUGHING,             30, "No Laughing",             "Keep Freddy from reaching the East Hall on Night 5.","ach_freddy.png",    false },
    { ACHIEVEMENT_NO_HIDING,               20, "No Hiding",               "Get caught by an animatronic.",                      "ach_jumpscare.png", true  }
};

Achievements::Achievements()
    : m_unlocked(0)
    , m_unlockedF2(0)
    , m_console(0)
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
    if (m_console) m_console->Print("Achievements: unlocked=0x%03X", (unsigned)m_unlocked);
    // v2.62: the FNAF2 add-on mask rides along (fnaf2_ach.ini)
    if (Progress::LoadAchieveF2(&bits)) m_unlockedF2 = bits;
    printf("Achievements: fnaf2 mask=0x%03X\n", (unsigned)m_unlockedF2);
#if !defined(FNAF_LIVE_SAFE)
    // v2.48: log profile readiness once at boot — the profile write silently
    // no-ops when nobody is signed in, so state it (0 = nobody, 1 = local,
    // 2 = also online).
    printf("Achievements: p0 signin=%d\n", (int)XUserGetSigninState(0));
    if (m_console) m_console->Print("ACH p0 signin=%d", (int)XUserGetSigninState(0));
#endif
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
    // Nights 1..5 map to ACHIEVEMENT_*_NIGHT(S)_AT_FREDDYS in order. The
    // official ids happen to be contiguous — say it with the table, not math.
    static const int kNightAch[5] = {
        ACHIEVEMENT_ONE_NIGHT_AT_FREDDYS,   ACHIEVEMENT_TWO_NIGHTS_AT_FREDDYS,
        ACHIEVEMENT_THREE_NIGHTS_AT_FREDDYS, ACHIEVEMENT_FOUR_NIGHTS_AT_FREDDYS,
        ACHIEVEMENT_FIVE_NIGHTS_AT_FREDDYS
    };
    if (night >= 1 && night <= 5) {
        Unlock(kNightAch[night - 1]);
    } else if (night == 6) {
        Unlock(ACHIEVEMENT_OVERTIME);         // survive the sixth night
    } else if (night == 7 && perfect) {
        Unlock(ACHIEVEMENT_NO_TAMPERING);     // Custom Night 20/20/20/20
    }
    if (night == 4 && !m_foxyRan)    Unlock(ACHIEVEMENT_NO_RUNNING);
    if (night == 5 && !m_freddyEast) Unlock(ACHIEVEMENT_NO_LAUGHING);
}

void Achievements::OnJumpscare() {
    Unlock(ACHIEVEMENT_NO_HIDING);
}

// v2.17 DEV helpers. v2.48: UnlockAll ALSO writes the whole set to the
// profile in one batch — the Guide list then visibly populates, which is the
// one-glance check that the embedded SPA was picked up.
void Achievements::UnlockAll() {
    m_unlocked = 0;
    for (int i = 0; i < COUNT; ++i) m_unlocked |= (1u << i);
    Save();
    SystemWriteAll();   // v2.48: was local-only
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
    if (!Progress::SaveAchieve(m_unlocked)) {
        printf("ACH save FAILED\n");
        if (m_console) m_console->Print("ACH save FAILED");
    }
}

// ---- v2.62: the FNAF2 add-on slots (ids 11..20) ----

void Achievements::UnlockFnaf2(int slot) {
    if (slot < 0 || slot > 9) return;
    const int id = 11 + slot;                 // the add-on id range (see the header)
    if (m_unlockedF2 & (1u << slot)) return;  // already earned
    m_unlockedF2 |= (1u << slot);
    if (!Progress::SaveAchieveF2(m_unlockedF2)) printf("ACH F2 save FAILED\n");
    // the profile write: the same pipe as the base ids; harmless failure
    // until the spa carries the add-on ids (the local mask stays the record)
#if !defined(FNAF_LIVE_SAFE)
    if (XUserGetSigninState(0) == eXUserSigninState_NotSignedIn) {
        printf("ACH F2 %d -> skipped (no signed-in profile at slot 0)\n", id);
        if (m_console) m_console->Print("ACH F2 %d -> skipped (no profile)", id);
        return;
    }
    XUSER_ACHIEVEMENT a;
    a.dwUserIndex     = 0;
    a.dwAchievementId = (DWORD)id;
    DWORD res = XUserWriteAchievements(1, &a, NULL);
    printf("ACH F2 %d -> 0x%08X (%s)\n", id, (unsigned)res, AchErrName(res));
    if (m_console) m_console->Print("ACH F2 %d -> 0x%08X (%s)", id, (unsigned)res, AchErrName(res));
#else
    printf("ACH F2 %d (local only)\n", id);
#endif
}

// v2.35: human-readable names for the XDK results achievements can hit.
static const char* AchErrName(DWORD res) {
    switch (res) {
        case ERROR_SUCCESS:          return "OK";
        case ERROR_ACCESS_DENIED:    return "ACCESS_DENIED";
        case ERROR_INVALID_PARAMETER:return "BAD_PARAM";
        case ERROR_DEVICE_NOT_CONNECTED: return "DEVICE_NOT_CONNECTED";
        case ERROR_NO_MORE_FILES:    return "NO_DISK_SPACE";
        default: return "?";
    }
}

void Achievements::SystemWrite(int id) {
#if !defined(FNAF_LIVE_SAFE)
    // System build: mark the achievement earned on the FIRST player's profile
    // (gamer index 0 — the original binds to him). Only when a profile is
    // actually signed in at slot 0; otherwise the XAM call would fail with
    // ACCESS_DENIED and we'd rather skip it — the local fnaf_ach.ini + the
    // in-game UI stay the record (as on a profile-less RGH).
    if (XUserGetSigninState(0) == eXUserSigninState_NotSignedIn) {
        printf("ACH %d -> skipped (no signed-in profile at slot 0)\n", id);
        if (m_console) m_console->Print("ACH %d -> skipped (no signed-in profile at slot 0)", id);
        return;
    }
    XUSER_ACHIEVEMENT a;
    a.dwUserIndex     = 0;             // FIRST player (gamer index 0) — the original binds to him
    a.dwAchievementId = (DWORD)id;     // ACHIEVEMENT_* from the generated spa.h (v2.47)
    DWORD res = XUserWriteAchievements(1, &a, NULL);
    printf("ACH %d -> 0x%08X (%s)\n", id, (unsigned)res, AchErrName(res));
    if (m_console) m_console->Print("ACH %d -> 0x%08X (%s)", id, (unsigned)res, AchErrName(res));
#else
    // Live Safe build: no Xbox profile write; the local game:\save\fnaf_ach.ini +
    // in-game UI are the entire record.
    (void)id;
    printf("ACH %d (local only)\n", id);
#endif
}

// v2.48: write the WHOLE set in one batched call (XUserWriteAchievements
// takes an array) — used by the dev "unlock all" so the Guide list actually
// shows all ten with their SPA names/scores/icons.
void Achievements::SystemWriteAll() {
#if !defined(FNAF_LIVE_SAFE)
    if (XUserGetSigninState(0) == eXUserSigninState_NotSignedIn) {
        printf("ACH all -> skipped (no signed-in profile at slot 0)\n");
        if (m_console) m_console->Print("ACH all -> skipped (no profile)");
        return;
    }
    XUSER_ACHIEVEMENT batch[COUNT];
    for (int i = 0; i < COUNT; ++i) {
        batch[i].dwUserIndex     = 0;             // FIRST player (gamer slot 0)
        batch[i].dwAchievementId = (DWORD)kAchievements[i].id;
    }
    DWORD res = XUserWriteAchievements(COUNT, batch, NULL);
    printf("ACH all -> 0x%08X (%s)\n", (unsigned)res, AchErrName(res));
    if (m_console) m_console->Print("ACH all -> 0x%08X (%s)", (unsigned)res, AchErrName(res));
#endif
}

bool Achievements::ShowSystemUI() {
#if !defined(FNAF_LIVE_SAFE)
    // System build: open the Xbox Guide "Achievements" list for this title.
    DWORD res = XShowAchievementsUI(0);
    printf("ACH UI -> 0x%08X (%s)\n", (unsigned)res, AchErrName(res));
    if (m_console) m_console->Print("ACH UI -> 0x%08X (%s)", (unsigned)res, AchErrName(res));
    return true;
#else
    // Live Safe build: no system UI — caller falls back to the in-game screen.
    return false;
#endif
}

} // namespace fnaf