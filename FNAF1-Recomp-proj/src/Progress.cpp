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
 * Storage: XContent INI file fnaf_save.ini with [freddy] section on Xbox 360.
 */

#include "Progress.h"
#include <cstdio>
#include <cstring>
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
// backend writes them under "save\" next to the .xex).
static const char* kXContentRoot = "fnaf_save";
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
        DWORD res = XShowDeviceSelectorUI(0, XCONTENTTYPE_SAVEDGAME, dwFlags, bytesRequested, &deviceID, &overlapped);
        if (res == ERROR_IO_PENDING) {
            while (!XHasOverlappedIoCompleted(&overlapped)) Sleep(16);
            res = XGetOverlappedResult(&overlapped, NULL, TRUE);
        }
        if (res != ERROR_SUCCESS) {
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
    DWORD res = XContentCreateEx(0, kXContentRoot, &content, dwContentFlags, &dwDisposition, NULL, 0, uliSize, NULL);
    return res == ERROR_SUCCESS;
}

// `file` is the short name ("fnaf_save.ini" / "fnaf_ach.ini"); `create`
// selects XCONTENTFLAG_CREATEALWAYS (save) vs OPENEXISTING (load).
static FILE* StorageOpen(const char* file, const char* mode, bool create) {
    if (!XContentMount(create)) return NULL;
    char path[128];
    fnaf::Snprintf(path, sizeof(path), "%s:\\%s", kXContentRoot, file);
    return fopen(path, mode);
}
static void StorageClose() { XContentClose(kXContentRoot, NULL); }

#else  // FNAF_LIVE_SAFE — local "save\" folder, no Xbox system APIs

static void EnsureLocalDir() { _mkdir("save"); }   // EEXIST/any error ignored

static FILE* StorageOpen(const char* file, const char* mode, bool /*create*/) {
    EnsureLocalDir();
    char path[128];
    fnaf::Snprintf(path, sizeof(path), "save\\%s", file);
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
    p.lives    = 0;
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
            else if (strcmp(key, "lives") == 0) p.lives = val;
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
        "[freddy]\nlevel=%d\nbeatgame=%d\nbeat6=%d\nbeat7=%d\nlives=%d\n",
        out.nextNight,
        out.beat5 ? 1 : 0,
        out.beat6 ? 1 : 0,
        out.beat7 ? 1 : 0,
        out.lives);

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
