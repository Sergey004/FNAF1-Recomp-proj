/**
 * Five Nights at Freddy's 1 — Recompilation
 * Progress.cpp: save/load of the night-flow progress (v2.10)
 *
 * Data provenance (original events):
 *  - "next day" groups 12/13 set Ini "beat6"/"beat7" when the 6 AM screen
 *    finishes with night counter 7 (Night 6 cleared) / 8 (Night 7 cleared
 *    with all AI at 20).  "the end" group 3 sets Ini "beatgame" when the
 *    Night-5 paycheck shows.
 *  - title groups 33/34/35/36: New Game starts "night number" 1, Continue
 *    starts Ini("level"), 6th night -> 6, custom -> 7.
 */

#include "Progress.h"
#include <cstdio>
#include <cstring>
#include <cstddef>   // offsetof

namespace fnaf {

static const u32 PROGRESS_MAGIC   = 0x31464E46u;  // 'FNF1' little-endian
static const u32 PROGRESS_VERSION = 1;

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

// Same canonical locations the pak loader probes. game:\ is the XEX
// directory on retail; e:\ and hdd:\ cover FSD/Aurora style installs.
static const char* s_paths[] = {
    "game:\\fnaf_save.bin",
    "D:\\fnaf_save.bin",
    "e:\\fnaf_save.bin",
    "hdd:\\fnaf_save.bin",
    "fnaf_save.bin",
    "./fnaf_save.bin"
};
static const int s_pathCount = (int)(sizeof(s_paths) / sizeof(s_paths[0]));

bool Progress::Load(GameProgress& p) {
    Reset(p);
    for (int i = 0; i < s_pathCount; ++i) {
        FILE* f = fopen(s_paths[i], "rb");
        if (!f) continue;
        GameProgress tmp;
        memset(&tmp, 0, sizeof(tmp));
        size_t got = fread(&tmp, 1, sizeof(tmp), f);
        fclose(f);
        if (got != sizeof(tmp))                continue;
        if (tmp.magic   != PROGRESS_MAGIC)     continue;
        if (tmp.version != PROGRESS_VERSION)   continue;
        if (tmp.checksum != ComputeChecksum(tmp)) continue;

        // sanitize
        if (tmp.nextNight < 1) tmp.nextNight = 1;
        if (tmp.nextNight > 7) tmp.nextNight = 7;
        p = tmp;
        return true;
    }
    return false;
}

bool Progress::Save(const GameProgress& p) {
    GameProgress out = p;
    out.magic     = PROGRESS_MAGIC;
    out.version   = PROGRESS_VERSION;
    if (out.nextNight < 1) out.nextNight = 1;
    if (out.nextNight > 7) out.nextNight = 7;
    out.checksum  = ComputeChecksum(out);

    for (int i = 0; i < s_pathCount; ++i) {
        FILE* f = fopen(s_paths[i], "wb");
        if (!f) continue;
        size_t put = fwrite(&out, 1, sizeof(out), f);
        fclose(f);
        if (put == sizeof(out)) return true;
    }
    return false;
}

i32 Progress::StarCount(const GameProgress& p) {
    i32 stars = 0;
    if (p.beat5) ++stars;
    if (p.beat6) ++stars;
    if (p.beat7) ++stars;
    return stars;
}

} // namespace fnaf
