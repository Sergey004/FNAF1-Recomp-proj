/**
 * Five Nights at Freddy's 1 — Recompilation
 * Progress.h: persistent night-flow progress (v2.11)
 *
 * Replaces the old in-memory "SetUnlockedNight" placeholder. Mirrors the
 * original game's Ini object (frame title groups 30-43 / "next day"
 * groups 12-13), which stored exactly these values:
 *   level   — the night Continue starts (1..7; next uncompleted night)
 *   beatgame— Night 5 cleared  -> title star 1 + "6th night" button
 *   beat6   — Night 6 cleared  -> title star 2 + "custom night" button
 *   beat7   — Night 7 (custom) cleared -> title star 3
 *
 * Storage: INI file via XContent on Xbox 360, [freddy] section
 *   level=...
 *   beatgame=...
 *   beat6=...
 *   beat7=...
 * File name: fnaf_save.ini
 */

#ifndef FNAF_PROGRESS_H
#define FNAF_PROGRESS_H

#include "Types.h"

namespace fnaf {

struct GameProgress {
    u32 magic;        // 'FNF1'
    u32 version;      // 1
    i32 nextNight;    // 1..7 — what Continue starts
    bool beat5;       // Night 5 complete  (star 1 + 6th night button)
    bool beat6;       // Night 6 complete  (star 2 + custom night button)
    bool beat7;       // Night 7 complete  (star 3)
    i32 lives;        // original Ini "lives" value, persisted per save
    u32 checksum;     // sum of everything above (xor 0x5A5A5A5A)
};

class Progress {
public:
    // Fill defaults: night 1, nothing beaten.
    static void Reset(GameProgress& p);

    // Load save from XContent; true if a valid save was read into p.
    static bool Load(GameProgress& p);

    // Write save via XContent. True on success.
    static bool Save(const GameProgress& p);

    // Number of title-screen stars (0..3) implied by the flags.
    static i32 StarCount(const GameProgress& p);
};

} // namespace fnaf

#endif // FNAF_PROGRESS_H
