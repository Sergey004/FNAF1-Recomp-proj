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
#include <ctime>      // time/localtime (v2.74: the system date)
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
// The ORIGINAL save file is literally named "freddy" (no extension); its
// contents are INI ([freddy] section). Both backends keep that exact name —
// the system build stores it as the XContent content "freddy", the Live Safe
// build as a plain file game:\save\freddy.
static const char* kAchFile = "fnaf_ach.ini";
// v2.80: the FNAF1 progress file name (the Load/Save/Import call sites);
// the container itself resolves through the kSaveContainers table.
static const char* const kF1SaveFile = "freddy";

// ---------------------------------------------------------------------------
// v2.80: ONE XCONTENT CONTAINER PER GAME MODULE ("как миры Minecraft").
// Pre-v2.80 every module's file rode in a single container ("freddy" under
// "fnaf_save" — one dashboard tile for all five games). Now every module
// gets its OWN package: its own mount root, its own STFS file name and its
// own dashboard display name. Every save file name is module-unique, so
// StorageOpen derives the container from the file name — no game-side call
// changes anywhere. Display names must stay <= 31 WCHARs (the 360's
// XCONTENT_MAX_DISPLAYNAME_LENGTH = 32 incl. the terminator), roots <= 12.
// ---------------------------------------------------------------------------
struct SaveContainerSpec {
    const char* files[2];        // the module's file names (module-unique)
    const char* rootName;        // the XContent mount root
    const char* fileName;        // the STFS package name
    const WCHAR* displayName;    // the dashboard tile
};
static const SaveContainerSpec kSaveContainers[] = {
    // FNAF1: the ORIGINAL container, byte-for-byte today's tile — its
    // progress migrates by identity and the tile doubles as the full
    // pre-migration backup (see Progress::MigrateLegacySave).
    { { "freddy", "fnaf_ach.ini" },  "fnaf_save",  "freddy",  L"Five Nights at Freddy's 1 Save" },
    { { "freddy2", "fnaf2_ach.ini" },"fnaf2_save", "freddy2", L"Five Nights at Freddy's 2 Save" },
    { { "freddy3", 0 },              "fnaf3_save", "freddy3", L"Five Nights at Freddy's 3 Save" },
    { { "fn4", 0 },                  "fnaf4_save", "fn4",     L"Five Nights at Freddy's 4 Save" },
    { { "sl", 0 },                   "fnsl_save",  "sl",      L"Sister Location Save" },
};
static const SaveContainerSpec* FindContainerForFile(const char* file) {
    for (int i = 0; i < (int)(sizeof(kSaveContainers) / sizeof(kSaveContainers[0])); ++i)
        for (int j = 0; j < 2; ++j) {
            const char* f = kSaveContainers[i].files[j];
            if (f && strcmp(f, file) == 0) return &kSaveContainers[i];
        }
    return &kSaveContainers[0];   // unknown file -> the F1 container (as before)
}
// v2.80: the pre-v2.80 single container, addressed for the migration reads.
static const SaveContainerSpec kLegacyContainer = { { 0, 0 }, "fnaf_save", "freddy", L"Five Nights at Freddy's 1 Save" };

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

// v2.80: which container root is currently mounted — StorageClose unmounts
// exactly that one (v2.37's s_usingXcontent flag became this pointer: with
// per-module containers the root is no longer a constant).

// Fallback dir for the SYSTEM build (used only when XContent is down):
// deliberately NOT game:\save\ — that path is both the Live Safe storage
// AND an import candidate, so our own fallback files must not sit there
// (otherwise boot would offer to import its own save).
static void EnsureLocalDirFallback() { _mkdir("game:\\save_fallback"); }

static void MaybeSetThumbnail(const XCONTENT_DATA& content);

// v2.50: the device pick as its own step — Progress::PrimeStorage() calls it
// once at boot (while nothing else is on screen); on a single-device console
// (Slim + HDD only, no MU slots) the selector auto-answers with the HDD id
// WITHOUT showing UI, so later saves/mounts never poke XAM's UI pipeline
// mid-flow (that was the ACCESS_DENIED storm after message boxes).
static bool PickStorageDevice()
{
    if (g_deviceChosen) return true;
    {
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
    return true;
}

// v2.80: mount ONE container (per the spec) and open a file inside it.
// On a fopen failure the container is UNMOUNTED right here (the v2.48
// stale-mount rule — a left-mounted root poisons every later create with
// SHARING_VIOLATION). Returns 0 when the mount or the file open fails.
static bool XContentMountSpec(const SaveContainerSpec* spec, bool create)
{
    if (!PickStorageDevice()) return false;

    XCONTENT_DATA content;
    memset(&content, 0, sizeof(content));
    content.DeviceID = g_saveDevice;
    content.dwContentType = XCONTENTTYPE_SAVEDGAME;
    wcsncpy(content.szDisplayName, spec->displayName, XCONTENT_MAX_DISPLAYNAME_LENGTH - 1);
    strncpy(content.szFileName, spec->fileName, XCONTENT_MAX_FILENAME_LENGTH - 1);

    DWORD dwContentFlags;
    if (create) {
        dwContentFlags = XCONTENTFLAG_CREATEALWAYS;
    } else {
        dwContentFlags = XCONTENTFLAG_OPENEXISTING;
    }

    DWORD dwDisposition = 0;
    ULARGE_INTEGER uliSize;
    uliSize.QuadPart = XContentCalculateSize(64 * 1024, 1);
    DWORD res = XContentCreateEx(g_saveUser, spec->rootName, &content, dwContentFlags, &dwDisposition, NULL, 0, uliSize, NULL);
    if (res == ERROR_SHARING_VIOLATION || res == ERROR_ALREADY_EXISTS) {
        // v2.48: a leftover mount of this root from earlier in the boot poisons
        // every further open with SHARING_VIOLATION/ALREADY_EXISTS (real-HW
        // log). Close the stale root and retry once.
        printf("XContent: create hit 0x%08X — closing the stale root and retrying\n", (unsigned)res);
        XContentClose(spec->rootName, NULL);
        dwDisposition = 0;
        res = XContentCreateEx(g_saveUser, spec->rootName, &content, dwContentFlags, &dwDisposition, NULL, 0, uliSize, NULL);
    }
    printf("XContent: user=%u device=0x%08X root=%s create=0x%08X disp=0x%X\n",
           (unsigned)g_saveUser, (unsigned)g_saveDevice, spec->rootName, (unsigned)res, (unsigned)dwDisposition);
    if (res == ERROR_SUCCESS) MaybeSetThumbnail(content);
    return res == ERROR_SUCCESS;
}

// v2.48: where the 360 gets the save's picture from — the title icon in the
// device picker comes from the embedded .spa (X_IMAGEID_GAME), and per-content
// thumbnails are title-written PNGs via XContentSetThumbnail (max 15616 B).
// Set ours once per CONTAINER (per root) right after it mounts, so the
// storage manager shows the game icon instead of a blank tile.
static char s_thumbRootDone[16] = "";
static void MaybeSetThumbnail(const XCONTENT_DATA& content) {
    // v2.80: the caller passes the mounted spec's content; the root is
    // tracked through the content's file name (one container per file name).
    if (strncmp(s_thumbRootDone, content.szFileName, sizeof(s_thumbRootDone) - 1) == 0) return;
    FILE* f = fopen("game:\\achievements_pics\\game_icon.png", "rb");
    if (!f) { printf("XContent: thumbnail skipped (game:\\achievements_pics\\game_icon.png missing)\n"); return; }
    static BYTE buf[15616];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    if (n == 0) return;
    DWORD res = XContentSetThumbnail(g_saveUser, &content, buf, (DWORD)n, NULL);
    printf("XContent: thumbnail -> 0x%08X (%u bytes)\n", (unsigned)res, (unsigned)n);
    strncpy(s_thumbRootDone, content.szFileName, sizeof(s_thumbRootDone) - 1);
    s_thumbRootDone[sizeof(s_thumbRootDone) - 1] = '\0';
}

// `file` is the short name ("freddy" / "freddy2" / ...); `create` selects
// XCONTENTFLAG_CREATEALWAYS (save) vs OPENEXISTING (load). The container is
// resolved from the file name (v2.80: one container per game module).
// v2.37: XContent first (when it mounts); on ANY failure — mount or the
// file open inside the container — FALL BACK to the plain HDD folder
// (game:\save_fallback\ + file), so saves/achievements survive on RGH setups
// without a usable XContent/profile.
static const char* s_mountedRoot = 0;   // v2.80: the root StorageClose must unmount

static FILE* MountAndOpen(const SaveContainerSpec* spec, const char* file, const char* mode, bool create) {
    if (!XContentMountSpec(spec, create)) return 0;
    char path[128];
    fnaf::Snprintf(path, sizeof(path), "%s:\\%s", spec->rootName, file);
    FILE* f = fopen(path, mode);
    if (!f) {
        // v2.48: the container fopen FAILED but the mount succeeded — unmount
        // RIGHT HERE. The old code dropped s_usingXcontent without closing,
        // so StorageClose skipped the unmount, the root stayed mounted for
        // the rest of the boot, and every later XContentCreateEx answered with
        // 0x20 SHARING_VIOLATION / 0xB7 ALREADY_EXISTS (seen in the HW log).
        DWORD ferr = GetLastError();
        // v2.50: ERROR_FILE_NOT_FOUND on a READ is expected on a fresh
        // container (the file not written yet) — not an error.
        DWORD fres = XContentFlush(spec->rootName, NULL);
        DWORD cres = XContentClose(spec->rootName, NULL);
        if (ferr == ERROR_FILE_NOT_FOUND && mode[0] == 'r')
            printf("SAVE: no '%s' in %s yet (fresh) (flush=0x%08X close=0x%08X)\n",
                   file, spec->rootName, (unsigned)fres, (unsigned)cres);
        else
            printf("SAVE: container file open failed (err=0x%08X) (flush=0x%08X close=0x%08X)\n",
                   (unsigned)ferr, (unsigned)fres, (unsigned)cres);
        return 0;
    }
    s_mountedRoot = spec->rootName;
    printf("SAVE: storage = XContent (%s:\\%s)\n", spec->rootName, file);
    return f;
}

static FILE* StorageOpen(const char* file, const char* mode, bool create) {
#if !defined(FNAF_LIVE_SAFE)
    // Canonical XContent needs a LOCALLY SIGNED-IN gamer index; resolve it
    // once per boot. With nobody signed in, skip XContent entirely (it would
    // fail with ERROR_ACCESS_DENIED) and use the local fallback folder.
    if (g_saveUser == 0xFFFFFFFF) g_saveUser = PickSignedInUserIndex();
    if (g_saveUser != 0xFFFFFFFF) {
        const SaveContainerSpec* spec = FindContainerForFile(file);
        FILE* f = MountAndOpen(spec, file, mode, create);
        if (f) return f;
        // fall through to the local fallback
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
    if (s_mountedRoot) {
        // commit pattern per the docs: flush buffers, then close — close
        // must succeed for a write to be considered valid
        DWORD fres = XContentFlush(s_mountedRoot, NULL);
        DWORD cres = XContentClose(s_mountedRoot, NULL);
        printf("XContent: flush=0x%08X close=0x%08X (%s)\n", (unsigned)fres, (unsigned)cres, s_mountedRoot);
        s_mountedRoot = 0;
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

// ---------------------------------------------------------------------------
// v2.80: the pre-v2.80 -> per-module migration. A boot older than v2.80 kept
// EVERY module's file in the single legacy container (the "freddy" package
// under "fnaf_save" — which is FNAF1's container today, so F1 migrates by
// identity). When the MODULE container lacks its file but the legacy one has
// it, copy the bytes over once. The legacy copy is NOT deleted: the F1 tile
// doubles as a complete backup of the migration moment (delete it from the
// dashboard manually if you want; the module never reads it again).
// Call once per module file, from the module's Load, BEFORE its Progress
// load (AppModules.cpp).
// ---------------------------------------------------------------------------
void Progress::MigrateLegacySave(const char* file) {
#if !defined(FNAF_LIVE_SAFE)
    if (g_saveUser == 0xFFFFFFFF) g_saveUser = PickSignedInUserIndex();
    if (g_saveUser == 0xFFFFFFFF) return;          // no profile: the local fallback rules
    const SaveContainerSpec* spec = FindContainerForFile(file);
    if (strcmp(spec->rootName, kLegacyContainer.rootName) == 0) return;   // F1 lives there already

    // 1) already inside the module container?
    FILE* f = MountAndOpen(spec, file, "rb", false);
    if (f) { fclose(f); StorageClose(); return; }

    // 2) does the legacy container carry it? read the whole file (the INI
    // saves are tiny; the 256 KB cap is paranoia)
    FILE* lf = MountAndOpen(&kLegacyContainer, file, "rb", false);
    if (!lf) return;
    long n = 0;
    char* buf = 0;
    fseek(lf, 0, SEEK_END);
    n = ftell(lf);
    fseek(lf, 0, SEEK_SET);
    if (n > 0 && n <= 256 * 1024) {
        buf = (char*)malloc((size_t)n);
        if (buf && fread(buf, 1, (size_t)n, lf) != (size_t)n) { free(buf); buf = 0; }
    }
    fclose(lf);
    StorageClose();
    if (!buf) return;
    // 3) write it into the module container
    FILE* nf = MountAndOpen(spec, file, "wb", true);
    if (!nf) { free(buf); return; }
    size_t put = fwrite(buf, 1, (size_t)n, nf);
    fclose(nf);
    StorageClose();
    free(buf);
    printf("SAVE: migrated '%s' into the %s container (%u bytes; the legacy copy stays as a backup)\n",
           file, spec->rootName, (unsigned)put);
#endif
}

// v2.50: resolve the storage device once up front (called from main right
// after the first presented frame, BEFORE any message box). On a
// single-device console (Slim + internal HDD, no MU slots) the selector
// answers silently with the HDD id — after this, nothing in the save path
// ever boots XAM's UI pipeline (that pipeline is exactly what broke with
// ACCESS_DENIED storms when first called right after a box).
void Progress::PrimeStorage() {
#if !defined(FNAF_LIVE_SAFE)
    if (g_saveUser == 0xFFFFFFFF) g_saveUser = PickSignedInUserIndex();
    if (g_saveUser == 0xFFFFFFFF) {
        printf("XContent: prime — no signed-in profile, local storage only\n");
        return;
    }
    bool ok = PickStorageDevice();
    printf("XContent: prime -> %s (device 0x%08X)\n",
           ok ? "picked" : "not picked yet", (unsigned)g_saveDevice);
#endif
}

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

    FILE* f = StorageOpen(kF1SaveFile, "rb", false);
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

    FILE* f = StorageOpen(kF1SaveFile, "wb", true);
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

    FILE* dst = StorageOpen(kF1SaveFile, "wb", true);
    if (!dst) return false;
    size_t put = fwrite(buffer, 1, got, dst);
    fclose(dst);
    StorageClose();
    if (put == got) {
        // v2.42: the destination is whatever StorageOpen actually served
        // (see the "SAVE: storage = ..." line) — don't hardcode it here
        printf("SAVE imported: %s\n", srcPath);
        // v2.55: remove only the truly-loose source (game:\freddy, next to
        // the .xex) after a successful import — without that every boot
        // re-offers the same file. game:\save\freddy is NEVER touched: that
        // path is the Live Safe build's OWN save store, and removing it would
        // erase that build's progress. Deletion failure is logged, non-fatal.
        if (strcmp(srcPath, "game:\\freddy") == 0) {
            if (remove(srcPath) == 0) {
                printf("SAVE: loose source %s removed after import\n", srcPath);
            } else {
                printf("SAVE: WARN could not remove loose source %s after import\n", srcPath);
            }
        } else {
            printf("SAVE: keeping %s (Live Safe home) after import\n", srcPath);
        }
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

// ---- v2.62: the FNAF2 achievement mask (fnaf2_ach.ini, bits 0..9) ----
// v2.79: the file is gated with a "v=2" marker — the v1 mask predates the
// slot remap (the Xbox One set), its bits mean DIFFERENT achievements now,
// so an unmarked file is discarded instead of re-read.

bool Progress::LoadAchieveF2(u32* bits) {
    *bits = 0;
    bool ok = false;
    bool v2 = false;
    FILE* f = StorageOpen("fnaf2_ach.ini", "rb", false);
    if (!f) return false;
    char buf[128];
    while (fgets(buf, sizeof(buf), f)) {
        char* eol = strchr(buf, '\r');
        if (eol) *eol = '\0';
        eol = strchr(buf, '\n');
        if (eol) *eol = '\0';
        char* s = buf;
        while (*s && (*s == ' ' || *s == '\t')) ++s;
        if (strncmp(s, "v=2", 3) == 0) {
            v2 = true;
            continue;
        }
        if (strncmp(s, "unlocked=", 9) == 0) {
            *bits = (u32)atoi(s + 9);
            ok = true;
        }
    }
    fclose(f);
    StorageClose();
    if (!v2) { *bits = 0; return false; }
    return ok;
}

bool Progress::SaveAchieveF2(u32 bits) {
    char buf[64];
    int len = fnaf::Snprintf(buf, sizeof(buf), "v=2\nunlocked=%u\n", (unsigned)bits);
    FILE* f = StorageOpen("fnaf2_ach.ini", "wb", true);
    if (!f) return false;
    size_t put = fwrite(buf, 1, (size_t)len, f);
    fclose(f);
    StorageClose();
    return put == (size_t)len;
}

// ------------------------------------------------------------
// v2.62: the FNAF2 save — file "freddy2", section [freddy2], the dump's
// own Ini keys (level/cine/turn/beatgame/beat6/beat7/c1..c10). Same
// StorageOpen backend as everything else (XContent container + the local
// fallback folder).
// ------------------------------------------------------------

void Progress::ResetF2(GameProgressF2& p) {
    p.level = 1;
    p.cine = 0;
    p.turn = 0;
    p.beatgame = false;
    p.beat6 = false;
    p.beat7 = false;
    p.term = false;
    for (i32 i = 0; i < 10; ++i) p.c[i] = false;
}

bool Progress::LoadF2(GameProgressF2& p) {
    ResetF2(p);
    FILE* f = StorageOpen("freddy2", "rb", false);
    if (!f) return false;
    char buf[128];
    bool inSection = false;
    bool any = false;
    char key[64];
    int val = 0;
    while (fgets(buf, sizeof(buf), f)) {
        char* eol = strchr(buf, '\r');
        if (eol) *eol = '\0';
        eol = strchr(buf, '\n');
        if (eol) *eol = '\0';
        char* s = buf;
        while (*s && (*s == ' ' || *s == '\t')) ++s;
        if (*s == '\0' || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            inSection = (strcmp(s, "[freddy2]") == 0);
            continue;
        }
        if (!inSection) continue;
        if (ParseIniLine(s, key, &val)) {
            any = true;
            if      (strcmp(key, "level") == 0)    p.level = val;
            else if (strcmp(key, "cine") == 0)     p.cine = val;
            else if (strcmp(key, "turn") == 0)     p.turn = val;
            else if (strcmp(key, "beatgame") == 0) p.beatgame = (val != 0);
            else if (strcmp(key, "beat6") == 0)    p.beat6 = (val != 0);
            else if (strcmp(key, "beat7") == 0)    p.beat7 = (val != 0);
            else if (strcmp(key, "term") == 0)     p.term = (val != 0);   // v2.74
            else if (key[0] == 'c' && key[1] >= '1' && key[1] <= '9') {
                const int idx = atoi(key + 1);
                if (idx >= 1 && idx <= 10) p.c[idx - 1] = (val != 0);
            }
        }
    }
    fclose(f);
    StorageClose();
    if (p.level < 1 || p.level > 8) p.level = 1;
    if (!any) return false;
    return true;
}

bool Progress::SaveF2(const GameProgressF2& pIn) {
    GameProgressF2 out = pIn;
    if (out.level < 1) out.level = 1;
    char iniBuf[512];
    int len = fnaf::Snprintf(iniBuf, sizeof(iniBuf),
        "[freddy2]\nlevel=%d\ncine=%d\nturn=%d\nbeatgame=%d\nbeat6=%d\nbeat7=%d\n"
        "c1=%d\nc2=%d\nc3=%d\nc4=%d\nc5=%d\nc6=%d\nc7=%d\nc8=%d\nc9=%d\nc10=%d\n",
        out.level, out.cine, out.turn,
        out.beatgame ? 1 : 0, out.beat6 ? 1 : 0, out.beat7 ? 1 : 0,
        out.c[0] ? 1 : 0, out.c[1] ? 1 : 0, out.c[2] ? 1 : 0, out.c[3] ? 1 : 0,
        out.c[4] ? 1 : 0, out.c[5] ? 1 : 0, out.c[6] ? 1 : 0, out.c[7] ? 1 : 0,
        out.c[8] ? 1 : 0, out.c[9] ? 1 : 0);

    FILE* f = StorageOpen("freddy2", "wb", true);
    if (!f) return false;
    size_t put = fwrite(iniBuf, 1, (size_t)len, f);
    fclose(f);
    StorageClose();
    return put == (size_t)len;
}

void Progress::WipeF2() {
    // the title's X-hold wipe (dump title group 49): level=1 and every flag
    // back to 0 — the save file is rewritten with the defaults
    GameProgressF2 p;
    ResetF2(p);
    SaveF2(p);
}

// ---- v2.62: the FNAF3 save (freddy3). v2.63 carries the full dump key
// set: progression flags, the minigame chain and the cheats toggles. ----

void Progress::ResetF3(GameProgressF3& p) {
    p.level = 1;
    p.cine = 0;
    p.beat6 = false;
    p.goodend = false;
    p.fourthStar = false;
    p.bb = false;
    p.cake = false;
    p.k1 = false; p.k2 = false; p.k3 = false; p.k4 = false;
    p.fastNights = false;
    p.ventProof = false;
    p.hyper = false;
    p.noCams = false;
    p.term = false;
}

bool Progress::LoadF3(GameProgressF3& p) {
    ResetF3(p);
    FILE* f = StorageOpen("freddy3", "rb", false);
    if (!f) return false;
    char buf[128];
    bool inSection = false;
    bool any = false;
    char key[64];
    int val = 0;
    while (fgets(buf, sizeof(buf), f)) {
        char* eol = strchr(buf, '\r');
        if (eol) *eol = '\0';
        eol = strchr(buf, '\n');
        if (eol) *eol = '\0';
        char* s = buf;
        while (*s && (*s == ' ' || *s == '\t')) ++s;
        if (*s == '\0' || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            inSection = (strcmp(s, "[freddy3]") == 0);
            continue;
        }
        if (!inSection) continue;
        if (ParseIniLine(s, key, &val)) {
            any = true;
            if      (strcmp(key, "level") == 0)      p.level = val;
            else if (strcmp(key, "cine") == 0)       p.cine = val;
            else if (strcmp(key, "beat6") == 0)      p.beat6 = val != 0;
            else if (strcmp(key, "goodend") == 0)    p.goodend = val != 0;
            else if (strcmp(key, "4thstar") == 0)    p.fourthStar = val != 0;
            else if (strcmp(key, "bb") == 0)         p.bb = val != 0;
            else if (strcmp(key, "cake") == 0)       p.cake = val != 0;
            else if (strcmp(key, "k1") == 0)         p.k1 = val != 0;
            else if (strcmp(key, "k2") == 0)         p.k2 = val != 0;
            else if (strcmp(key, "k3") == 0)         p.k3 = val != 0;
            else if (strcmp(key, "k4") == 0)         p.k4 = val != 0;
            else if (strcmp(key, "fast") == 0)       p.fastNights = val != 0;
            else if (strcmp(key, "vents") == 0)      p.ventProof = val != 0;
            else if (strcmp(key, "hyper") == 0)      p.hyper = val != 0;
            else if (strcmp(key, "nocams") == 0)     p.noCams = val != 0;
            else if (strcmp(key, "term") == 0)       p.term = val != 0;   // v2.74
        }
    }
    fclose(f);
    StorageClose();
    if (p.level < 1 || p.level > 7) p.level = 1;
    if (!any) return false;
    return true;
}

bool Progress::SaveF3(const GameProgressF3& pIn) {
    GameProgressF3 out = pIn;
    if (out.level < 1) out.level = 1;
    char iniBuf[512];
    int len = fnaf::Snprintf(iniBuf, sizeof(iniBuf),
        "[freddy3]\nlevel=%d\ncine=%d\nbeat6=%d\ngoodend=%d\n4thstar=%d\n"
        "bb=%d\ncake=%d\nk1=%d\nk2=%d\nk3=%d\nk4=%d\n"
        "fast=%d\nvents=%d\nhyper=%d\nnocams=%d\n",
        out.level, out.cine,
        out.beat6 ? 1 : 0, out.goodend ? 1 : 0, out.fourthStar ? 1 : 0,
        out.bb ? 1 : 0, out.cake ? 1 : 0,
        out.k1 ? 1 : 0, out.k2 ? 1 : 0, out.k3 ? 1 : 0, out.k4 ? 1 : 0,
        out.fastNights ? 1 : 0, out.ventProof ? 1 : 0,
        out.hyper ? 1 : 0, out.noCams ? 1 : 0);
    FILE* f = StorageOpen("freddy3", "wb", true);
    if (!f) return false;
    size_t put = fwrite(iniBuf, 1, (size_t)len, f);
    fclose(f);
    StorageClose();
    return put == (size_t)len;
}

// ---- v2.64: the FNAF4 save (fn4) ----

void Progress::ResetF4(GameProgressF4& p) {
    p.night = 1;
    p.scene = 0;
    p.beat5 = false; p.beat6 = false; p.beat7 = false; p.beat8 = false;
    p.s1 = false; p.s2 = false; p.s3 = false;
    p.s4 = false; p.s5 = false; p.s6 = false;
    p.test = false;
    p.cheatHouseMap = false; p.fastNights = false; p.cheatRadar = false;
    p.blindMode = false; p.instaFoxy = false; p.madFreddy = false;
    p.allNightmare = false;
}

bool Progress::LoadF4(GameProgressF4& p) {
    ResetF4(p);
    FILE* f = StorageOpen("fn4", "rb", false);
    if (!f) return false;
    char buf[128];
    bool inSection = false;
    bool any = false;
    char key[64];
    int val = 0;
    while (fgets(buf, sizeof(buf), f)) {
        char* eol = strchr(buf, '\r');
        if (eol) *eol = '\0';
        eol = strchr(buf, '\n');
        if (eol) *eol = '\0';
        char* s = buf;
        while (*s && (*s == ' ' || *s == '\t')) ++s;
        if (*s == '\0' || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            inSection = (strcmp(s, "[fn4]") == 0);
            continue;
        }
        if (!inSection) continue;
        if (ParseIniLine(s, key, &val)) {
            any = true;
            if      (strcmp(key, "night") == 0)   p.night = val;
            else if (strcmp(key, "scene") == 0)   p.scene = val;
            else if (strcmp(key, "beat5") == 0)   p.beat5 = val != 0;
            else if (strcmp(key, "beat6") == 0)   p.beat6 = val != 0;
            else if (strcmp(key, "beat7") == 0)   p.beat7 = val != 0;
            else if (strcmp(key, "beat8") == 0)   p.beat8 = val != 0;
            else if (strcmp(key, "s1") == 0)      p.s1 = val != 0;
            else if (strcmp(key, "s2") == 0)      p.s2 = val != 0;
            else if (strcmp(key, "s3") == 0)      p.s3 = val != 0;
            else if (strcmp(key, "s4") == 0)      p.s4 = val != 0;
            else if (strcmp(key, "s5") == 0)      p.s5 = val != 0;
            else if (strcmp(key, "s6") == 0)      p.s6 = val != 0;
            else if (strcmp(key, "test") == 0)    p.test = val != 0;
            else if (strcmp(key, "housemap") == 0) p.cheatHouseMap = val != 0;
            else if (strcmp(key, "fast") == 0)     p.fastNights = val != 0;
            else if (strcmp(key, "radar") == 0)    p.cheatRadar = val != 0;
            else if (strcmp(key, "blind") == 0)    p.blindMode = val != 0;
            else if (strcmp(key, "instafoxy") == 0) p.instaFoxy = val != 0;
            else if (strcmp(key, "madfreddy") == 0) p.madFreddy = val != 0;
            else if (strcmp(key, "allnightmare") == 0) p.allNightmare = val != 0;
        }
    }
    fclose(f);
    StorageClose();
    if (p.night < 1 || p.night > 8) p.night = 1;
    if (!any) return false;
    return true;
}

bool Progress::SaveF4(const GameProgressF4& pIn) {
    GameProgressF4 out = pIn;
    if (out.night < 1) out.night = 1;
    if (out.night > 8) out.night = 8;
    char iniBuf[512];
    int len = fnaf::Snprintf(iniBuf, sizeof(iniBuf),
        "[fn4]\nnight=%d\nscene=%d\nbeat5=%d\nbeat6=%d\nbeat7=%d\nbeat8=%d\n"
        "s1=%d\ns2=%d\ns3=%d\ns4=%d\ns5=%d\ns6=%d\ntest=%d\n"
        "housemap=%d\nfast=%d\nradar=%d\nblind=%d\ninstafoxy=%d\nmadfreddy=%d\nallnightmare=%d\n",
        out.night, out.scene,
        out.beat5 ? 1 : 0, out.beat6 ? 1 : 0, out.beat7 ? 1 : 0, out.beat8 ? 1 : 0,
        out.s1 ? 1 : 0, out.s2 ? 1 : 0, out.s3 ? 1 : 0,
        out.s4 ? 1 : 0, out.s5 ? 1 : 0, out.s6 ? 1 : 0,
        out.test ? 1 : 0,
        out.cheatHouseMap ? 1 : 0, out.fastNights ? 1 : 0, out.cheatRadar ? 1 : 0,
        out.blindMode ? 1 : 0, out.instaFoxy ? 1 : 0, out.madFreddy ? 1 : 0,
        out.allNightmare ? 1 : 0);
    FILE* f = StorageOpen("fn4", "wb", true);
    if (!f) return false;
    size_t put = fwrite(iniBuf, 1, (size_t)len, f);
    fclose(f);
    StorageClose();
    return put == (size_t)len;
}

// ---- v2.65: the Sister Location save ("sl") ----

void Progress::ResetSL(GameProgressSL& p) {
    p.current = 1;
    p.intro = false;
    p.beat1 = false;
    p.beat3 = false;
    p.keycard = false;
    p.endsceneno = 0;
    p.star104 = false;
}

bool Progress::LoadSL(GameProgressSL& p) {
    ResetSL(p);
    FILE* f = StorageOpen("sl", "rb", false);
    if (!f) return false;
    char buf[128];
    bool inSection = false;
    bool any = false;
    char key[64];
    int val = 0;
    while (fgets(buf, sizeof(buf), f)) {
        char* eol = strchr(buf, '\r');
        if (eol) *eol = '\0';
        eol = strchr(buf, '\n');
        if (eol) *eol = '\0';
        char* s = buf;
        while (*s && (*s == ' ' || *s == '\t')) ++s;
        if (*s == '\0' || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            inSection = (strcmp(s, "[sl]") == 0);
            continue;
        }
        if (!inSection) continue;
        if (ParseIniLine(s, key, &val)) {
            any = true;
            if      (strcmp(key, "current") == 0)    p.current = val;
            else if (strcmp(key, "intro") == 0)      p.intro = val != 0;
            else if (strcmp(key, "beat1") == 0)      p.beat1 = val != 0;
            else if (strcmp(key, "beat3") == 0)      p.beat3 = val != 0;
            else if (strcmp(key, "keycard") == 0)    p.keycard = val != 0;
            else if (strcmp(key, "endsceneno") == 0) p.endsceneno = val;
            else if (strcmp(key, "104") == 0)        p.star104 = val != 0;
        }
    }
    fclose(f);
    StorageClose();
    if (p.current < 1 || p.current > 5) p.current = 1;
    if (!any) return false;
    return true;
}

bool Progress::SaveSL(const GameProgressSL& pIn) {
    GameProgressSL out = pIn;
    if (out.current < 1) out.current = 1;
    if (out.current > 5) out.current = 5;
    char iniBuf[256];
    int len = fnaf::Snprintf(iniBuf, sizeof(iniBuf),
        "[sl]\ncurrent=%d\nintro=%d\nbeat1=%d\nbeat3=%d\nkeycard=%d\n"
        "endsceneno=%d\n104=%d\n",
        out.current,
        out.intro ? 1 : 0, out.beat1 ? 1 : 0, out.beat3 ? 1 : 0,
        out.keycard ? 1 : 0, out.endsceneno, out.star104 ? 1 : 0);
    FILE* f = StorageOpen("sl", "wb", true);
    if (!f) return false;
    size_t put = fwrite(iniBuf, 1, (size_t)len, f);
    fclose(f);
    StorageClose();
    return put == (size_t)len;
}

void Progress::WipeSL() {
    // the Warning-frame Delete wipe keeps `104` and the cheats OFF; the next
    // save starts clean (current=1)
    GameProgressSL p;
    ResetSL(p);
    SaveSL(p);
}

} // namespace fnaf
