/**
 * Five Nights at Freddy's 1 — Recompilation
 * Progress.cpp: save/load of the night-flow progress (v2.11)
 *
 * Data provenance (original events):
 *  - "next day" groups 12/13 set Ini "beat6"/"beat7" when the 6 AM screen
 *    finishes with night counter 7 (Night 6 cleared) / 8 (Night 7 cleared
 *    with all AI at 20).  "the end" group 3 sets Ini "beatgame" when the
 *    Night-5 paycheck shows.
 *  - title groups 33/34/35/36: New Game starts "night number" 1, Continue
 *    starts Ini("level"), 6th night -> 6, custom -> 7.
 *
 * Storage: XContent content "freddy" (system build) or
 * game:\save\freddy (Live Safe build) — the original's extension-less INI file.
 */

#include "Progress.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>    // atoi
#include <cstddef>   // offsetof
#include <direct.h>   // _mkdir (v2.20 local "Live Safe" backend)
#if !defined(FNAF_LIVE_SAFE)
#include <xtl.h>
#endif
#include "XdkCompat.h"

namespace fnaf {

static const u32 PROGRESS_MAGIC   = 0x31464E46u;  // 'FNF1' little-endian
static const u32 PROGRESS_VERSION = 1;

// Shared file names (the XContent root mounts them as a drive, the local
// backend writes them under "game:\save\" next to the .xex).
static const char* kXContentRoot = "fnaf_save";
// The ORIGINAL save file is literally named "freddy" (no extension); its
// contents are INI ([freddy] section). Both backends keep that exact name —
// the system build stores it as the XContent content "freddy", the Live Safe
// build as a plain file game:\save\freddy.
static const char* kXContentFile = "freddy";
static const char* kAchFile = "fnaf_ach.ini";

// ---------------------------------------------------------------------------
// v2.20 storage backend switch (inverted from v2.14):
//   default (no macro)   -> Xbox SYSTEM: XContent profile save + device
//                           selector. "обычная" build.
//   FNAF_LIVE_SAFE       -> LOCAL files under "save\" next to the .xex; no
//                           Xbox content/profile APIs are touched.
// Load/Save/LoadAchieve/SaveAchieve are written against StorageOpen/Close so
// the two backends share the INI logic.
// ---------------------------------------------------------------------------
#if !defined(FNAF_LIVE_SAFE)

static const WCHAR kXContentDisplayName[] = L"Five Nights at Freddy's 1 Save";
static XCONTENTDEVICEID g_saveDevice = XCONTENTDEVICE_ANY;
static bool g_deviceChosen = false;

// v2.43: after the device selector exhausts its retries, XAM keeps refusing
// new UI for a long while (real HW) — don't re-stall every subsequent
// StorageOpen in the same boot; cool down, then try the selector again later.
static DWORD s_selectorCooldownUntilMs = 0;

// v2.39: XContent functions require the index of a LOCALLY SIGNED-IN gamer.
// The original game is bound to the FIRST player (gamer index 0) — his profile
// is where its saves live (confirmed on the user's console: the boot import
// round-tripped through the player's profile). So we bind to index 0 only;
// an unsigned-in slot 0 (e.g. RGH without a profile) would make XContent fail
// with ERROR_ACCESS_DENIED, so the storage skips XContent entirely and falls
// back to the plain HDD folder.
static DWORD g_saveUser = 0xFFFFFFFF;

static DWORD PickSignedInUserIndex() {
    // On the 360 the first player's profile always lands in gamer slot 0.
    // Sign-in check per the XDK docs (XUSER_SIGNIN_STATE enum).
    if (XUserGetSigninState(0) != eXUserSigninState_NotSignedIn) return 0;
    return 0xFFFFFFFF;   // player 1 has no profile signed in
}

// v2.37: was the XContent container actually mounted by the current
// StorageOpen? Controls whether StorageClose must unmount it. When XContent
// is unavailable (e.g. RGH without a signed-in profile) the storage FALLS
// BACK to the plain HDD folder game:\save\ — this is what keeps the save
// and achievements working on such setups (and fixes the "fresh game after
// game over" bug).
static bool s_usingXcontent = false;

// Fallback dir for the SYSTEM build (used only when XContent is down):
// deliberately NOT game:\save\ — that path is both the Live Safe storage
// AND an import candidate, so our own fallback files must not sit there
// (otherwise boot would offer to import its own save).
static void EnsureLocalDirFallback() { _mkdir("game:\\save_fallback"); }

static bool XContentMount(bool create)
{
    if (!g_deviceChosen) {
        ULARGE_INTEGER bytesRequested;
        bytesRequested.QuadPart = XContentCalculateSize(64 * 1024, 1);
        DWORD dwFlags = XCONTENTFLAG_NONE;
        XCONTENTDEVICEID deviceID = XCONTENTDEVICE_ANY;
        // XShowDeviceSelectorUI is ASYNC: it needs a real XOVERLAPPED and must be
        // pumped to completion. The old NULL-overlapped call never let the picker
        // show up. Same pattern as XShowMessageBoxUI in main.cpp.
        XOVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        // v2.41: log the picker — it is a FULL-SCREEN system UI that waits
        // for the player to choose a storage device; unlogged it reads as
        // a hang (the v2.39 "hang on import").
        // v2.42: the picker often runs right after a message box was
        // dismissed, and XAM rejects new UI while the previous screen is
        // still tearing down (ACCESS_DENIED, seen on real HW: the unhook
        // lines land mid-import) — retry for up to 10 s before giving up.
        // v2.43: skip the whole thing while the cooldown from a recent
        // failure is active.
        if (GetTickCount() < s_selectorCooldownUntilMs) {
            printf("XContent: device selector on cooldown, using fallback storage\n");
            return false;
        }
        printf("XContent: device selector open (pick a storage device)\n");
        DWORD res = XShowDeviceSelectorUI(g_saveUser, XCONTENTTYPE_SAVEDGAME, dwFlags, bytesRequested, &deviceID, &overlapped);
        for (int attempt = 0; res == ERROR_ACCESS_DENIED && attempt < 40; ++attempt) {
            printf("XContent: device selector busy (ACCESS_DENIED), retry %d/40\n", attempt + 1);
            Sleep(250);
            memset(&overlapped, 0, sizeof(overlapped));
            res = XShowDeviceSelectorUI(g_saveUser, XCONTENTTYPE_SAVEDGAME, dwFlags, bytesRequested, &deviceID, &overlapped);
        }
        if (res == ERROR_IO_PENDING) {
            while (!XHasOverlappedIoCompleted(&overlapped)) Sleep(16);
            res = XGetOverlappedResult(&overlapped, NULL, TRUE);
        }
        printf("XContent: device selector res=0x%08X\n", res);
        if (res != ERROR_SUCCESS) {
            s_selectorCooldownUntilMs = GetTickCount() + 60000;
            return false;   // cancelled/failed: g_deviceChosen stays false -> retry next time
        }
        g_saveDevice = deviceID;
        g_deviceChosen = true;
    }

    XCONTENT_DATA content;
    memset(&content, 0, sizeof(content));
    content.DeviceID = g_saveDevice;
    content.dwContentType = XCONTENTTYPE_SAVEDGAME;
    wcscpy(content.szDisplayName, kXContentDisplayName);
    strncpy(content.szFileName, kXContentFile, XCONTENT_MAX_FILENAME_LENGTH - 1);

    DWORD dwContentFlags;
    if (create) {
        dwContentFlags = XCONTENTFLAG_CREATEALWAYS;
    } else {
        dwContentFlags = XCONTENTFLAG_OPENEXISTING;
    }

    DWORD dwDisposition = 0;
    ULARGE_INTEGER uliSize;
    uliSize.QuadPart = XContentCalculateSize(64 * 1024, 1);
    DWORD res = XContentCreateEx(g_saveUser, kXContentRoot, &content, dwContentFlags, &dwDisposition, NULL, 0, uliSize, NULL);
    if (res == ERROR_SHARING_VIOLATION || res == ERROR_ALREADY_EXISTS) {
        // v2.48: a leftover mount of this root from earlier in the boot poisons
        // every further open with SHARING_VIOLATION/ALREADY_EXISTS (real-HW
        // log). Close the stale root and retry once.
        printf("XContent: create hit 0x%08X — closing the stale root and retrying\n", (unsigned)res);
        XContentClose(kXContentRoot, NULL);
        dwDisposition = 0;
        res = XContentCreateEx(g_saveUser, kXContentRoot, &content, dwContentFlags, &dwDisposition, NULL, 0, uliSize, NULL);
    }
    printf("XContent: user=%u create=0x%08X disp=0x%X\n",
           (unsigned)g_saveUser, (unsigned)res, (unsigned)dwDisposition);
    return res == ERROR_SUCCESS;
}

// `file` is the short name ("freddy" / "fnaf_ach.ini"); `create`
// selects XCONTENTFLAG_CREATEALWAYS (save) vs OPENEXISTING (load).
// v2.37: XContent first (when it mounts); on ANY failure — mount or the
// file open inside the container — FALL BACK to the plain HDD folder
// (game:\save\ + file), so saves/achievements survive on RGH setups
// without a usable XContent/profile.
static FILE* StorageOpen(const char* file, const char* mode, bool create) {
#if !defined(FNAF_LIVE_SAFE)
    // Canonical XContent needs a LOCALLY SIGNED-IN gamer index; resolve it
    // once per boot. With nobody signed in, skip XContent entirely (it would
    // fail with ERROR_ACCESS_DENIED) and use the local fallback folder.
    if (g_saveUser == 0xFFFFFFFF) g_saveUser = PickSignedInUserIndex();
    s_usingXcontent = (g_saveUser != 0xFFFFFFFF) && XContentMount(create);
    if (s_usingXcontent) {
        char path[128];
        fnaf::Snprintf(path, sizeof(path), "%s:\\%s", kXContentRoot, file);
        FILE* f = fopen(path, mode);
        if (f) {
            // v2.42: honest backend log — the import log used to claim the
            // container while the write actually went to the fallback
            printf("SAVE: storage = XContent (%s:\\%s)\n", kXContentRoot, file);
            return f;
        }
        // v2.48: the container fopen FAILED but the mount succeeded — unmount
        // RIGHT HERE. The old code dropped s_usingXcontent without closing,
        // so StorageClose skipped the unmount, the root stayed mounted for
        // the rest of the boot, and every later XContentCreateEx answered with
        // 0x20 SHARING_VIOLATION / 0xB7 ALREADY_EXISTS (seen in the HW log).
        {
            DWORD ferr = GetLastError();
            DWORD fres = XContentFlush(kXContentRoot, NULL);
            DWORD cres = XContentClose(kXContentRoot, NULL);
            printf("SAVE: container file open failed (err=0x%08X) -> local fallback (flush=0x%08X close=0x%08X)\n",
                   (unsigned)ferr, (unsigned)fres, (unsigned)cres);
        }
        s_usingXcontent = false;   // container closed above -> local file below
    } else {
        // mount can fail for several honest reasons: no signed-in profile,
        // selector denied/cancelled, or the container simply not existing
        // yet (OPENEXISTING -> ERROR_PATH_NOT_FOUND on a clean setup)
        printf("SAVE: storage = local fallback (XContent mount failed or no profile)\n");
    }
    EnsureLocalDirFallback();
#endif
    char path[160];
    fnaf::Snprintf(path, sizeof(path), "game:\\save_fallback\\%s", file);
    return fopen(path, mode);
}
static void StorageClose() {
#if !defined(FNAF_LIVE_SAFE)
    if (s_usingXcontent) {
        // commit pattern per the docs: flush buffers, then close — close
        // must succeed for a write to be considered valid
        DWORD fres = XContentFlush(kXContentRoot, NULL);
        DWORD cres = XContentClose(kXContentRoot, NULL);
        printf("XContent: flush=0x%08X close=0x%08X\n", (unsigned)fres, (unsigned)cres);
        s_usingXcontent = false;
    }
#endif
}

#else  // FNAF_LIVE_SAFE — local "game:\save\" folder, no Xbox system APIs

static void EnsureLocalDir() { _mkdir("game:\\save"); }   // EEXIST/any error ignored

static FILE* StorageOpen(const char* file, const char* mode, bool /*create*/) {
    EnsureLocalDir();
    char path[160];
    fnaf::Snprintf(path, sizeof(path), "game:\\save\\%s", file);
    return fopen(path, mode);
}
static void StorageClose() { /* nothing mounted to close */ }

#endif // FNAF_LIVE_SAFE

void Progress::Reset(GameProgress& p) {
    memset(&p, 0, sizeof(p));
    p.magic     = PROGRESS_MAGIC;
    p.version   = PROGRESS_VERSION;
    p.nextNight = 1;
    p.beat5     = false;
    p.beat6     = false;
    p.beat7     = false;
    p.checksum  = 0;
}

static u32 ComputeChecksum(const GameProgress& p) {
    const u32* words = reinterpret_cast<const u32*>(&p);
    const size_t count = (offsetof(GameProgress, checksum)) / sizeof(u32);
    u32 sum = 0;
    for (size_t i = 0; i < count; ++i) sum ^= words[i];
    return sum ^ 0x5A5A5A5Au;
}

// Simple INI parser for [freddy] section
static bool ParseIniLine(const char* line, char* key, int* val) {
    // Skip leading whitespace
    while (*line && (*line == ' ' || *line == '\t')) ++line;
    // Expect key=
    char* pKey = key;
    while (*line && *line != '=' && *line != ' ' && *line != '\t' && *line != '\r' && *line != '\n') {
        *pKey++ = *line++;
    }
    *pKey = '\0';
    // Skip to value
    while (*line && (*line == ' ' || *line == '\t' || *line == '=')) ++line;
    if (!*line) return false;
    *val = atoi(line);
    return true;
}

bool Progress::Load(GameProgress& p) {
    Reset(p);
    char key[64];
    int val = 0;
    bool inFreddy = false;

    FILE* f = StorageOpen(kXContentFile, "rb", false);
    if (!f) return false;
    char buf[256];
    while (fgets(buf, sizeof(buf), f)) {
        char* eol = strchr(buf, '\r');
        if (eol) *eol = '\0';
        eol = strchr(buf, '\n');
        if (eol) *eol = '\0';
        char* s = buf;
        while (*s && (*s == ' ' || *s == '\t')) ++s;
        if (*s == '\0' || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            inFreddy = (strcmp(s, "[freddy]") == 0);
            continue;
        }
        if (!inFreddy) continue;
        if (ParseIniLine(s, key, &val)) {
            if (strcmp(key, "level") == 0) p.nextNight = val;
            else if (strcmp(key, "beatgame") == 0) p.beat5 = (val != 0);
            else if (strcmp(key, "beat6") == 0) p.beat6 = (val != 0);
            else if (strcmp(key, "beat7") == 0) p.beat7 = (val != 0);
        }
    }
    fclose(f);
    StorageClose();
    if (p.nextNight < 1 || p.nextNight > 7) return false;
    p.magic = PROGRESS_MAGIC;
    p.version = PROGRESS_VERSION;
    p.checksum = ComputeChecksum(p);
    return true;
}

bool Progress::Save(const GameProgress& pIn) {
    GameProgress out = pIn;
    if (out.nextNight < 1) out.nextNight = 1;
    if (out.nextNight > 7) out.nextNight = 7;

    char iniBuf[256];
    int len = fnaf::Snprintf(iniBuf, sizeof(iniBuf),
        "[freddy]\nlevel=%d\nbeatgame=%d\nbeat6=%d\nbeat7=%d\n",
        out.nextNight,
        out.beat5 ? 1 : 0,
        out.beat6 ? 1 : 0,
        out.beat7 ? 1 : 0);

    FILE* f = StorageOpen(kXContentFile, "wb", true);
    if (!f) return false;
    size_t put = fwrite(iniBuf, 1, (size_t)len, f);
    fclose(f);
    StorageClose();
    return put == (size_t)len;
}

i32 Progress::StarCount(const GameProgress& p) {
    i32 stars = 0;
    if (p.beat5) ++stars;
    if (p.beat6) ++stars;
    if (p.beat7) ++stars;
    return stars;
}

// v2.23: a valid loose "freddy" save to import (game:\save\freddy or
// game:\freddy), or NULL when none. System build only.
#if !defined(FNAF_LIVE_SAFE)
static const char* FindLooseSavePath() {
    static const char* kCandidates[2] = { "game:\\save\\freddy", "game:\\freddy" };
    for (int i = 0; i < 2; ++i) {
        FILE* f = fopen(kCandidates[i], "rb");
        if (!f) continue;

        char buffer[512];
        size_t n = fread(buffer, 1, sizeof(buffer) - 1, f);
        buffer[n] = '\0';
        fclose(f);

        // sanity: must be a [freddy] INI with a valid level 1..7
        char* lvl = strstr(buffer, "level=");
        if (n == 0 || !strstr(buffer, "[freddy]") || !lvl) continue;
        int v = atoi(lvl + 6);
        if (v < 1 || v > 7) continue;
        return kCandidates[i];
    }
    return NULL;
}
#endif

bool Progress::HasLooseSave() {
#if defined(FNAF_LIVE_SAFE)
    return false;
#else
    return FindLooseSavePath() != NULL;
#endif
}

// v2.23: bring a loose "freddy" save into the XContent container. This is how
// a Live Safe save (game:\save\freddy) or a manually-dropped original save
// (game:\freddy) is imported into the system build. No-op in Live Safe.
bool Progress::ImportSave() {
#if defined(FNAF_LIVE_SAFE)
    return false;
#else
    const char* srcPath = FindLooseSavePath();
    if (!srcPath) return false;

    FILE* src = fopen(srcPath, "rb");
    if (!src) return false;
    char buffer[512];
    size_t got = fread(buffer, 1, sizeof(buffer) - 1, src);
    buffer[got] = '\0';
    fclose(src);

    FILE* dst = StorageOpen(kXContentFile, "wb", true);
    if (!dst) return false;
    size_t put = fwrite(buffer, 1, got, dst);
    fclose(dst);
    StorageClose();
    if (put == got) {
        // v2.42: the destination is whatever StorageOpen actually served
        // (see the "SAVE: storage = ..." line) — don't hardcode it here
        printf("SAVE imported: %s\n", srcPath);
        return true;
    }
    return false;
#endif
}

// ---------------------------------------------------------------------------
// Achievements persistence (v2.14). A second file in the SAME fnaf_save root,
// decoupled from GameProgress so Progress::Reset (the hidden Delete-key wipe)
// only resets night flow and never clears earned achievements.
// ---------------------------------------------------------------------------

bool Progress::LoadAchieve(u32* bits) {
    if (!bits) return false;
    *bits = 0;
    FILE* f = StorageOpen(kAchFile, "rb", false);
    if (!f) return false;
    bool ok = false;
    char buf[128];
    while (fgets(buf, sizeof(buf), f)) {
        char* eol = strchr(buf, '\r'); if (eol) *eol = '\0';
        eol = strchr(buf, '\n'); if (eol) *eol = '\0';
        char* s = buf;
        while (*s && (*s == ' ' || *s == '\t')) ++s;
        if (strncmp(s, "unlocked=", 9) == 0) {
            *bits = (u32)atoi(s + 9);
            ok = true;
        }
    }
    fclose(f);
    StorageClose();
    return ok;
}

bool Progress::SaveAchieve(u32 bits) {
    char buf[64];
    int len = fnaf::Snprintf(buf, sizeof(buf), "unlocked=%u\n", (unsigned)bits);
    FILE* f = StorageOpen(kAchFile, "wb", true);
    if (!f) return false;
    size_t put = fwrite(buf, 1, (size_t)len, f);
    fclose(f);
    StorageClose();
    return put == (size_t)len;
}

} // namespace fnaf
