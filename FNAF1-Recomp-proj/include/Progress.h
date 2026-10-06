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
 * Storage (v2.20/v2.23 two backends, gated by FNAF_LIVE_SAFE in Progress.cpp):
 *   default (no macro)  -> Xbox system XContent (content "freddy" + device
 *                          selector);
 *   FNAF_LIVE_SAFE      -> local "game:\save\freddy" next to the .xex.
 *   INI [freddy] section, values:
 *   level=...
 *   beatgame=...
 *   beat6=...
 *   beat7=...
 * File name: "freddy" (the original save name, NO extension) in both builds.
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
    u32 checksum;     // sum of everything above (xor 0x5A5A5A5A)
};

class Progress {
public:
    // Fill defaults: night 1, nothing beaten.
    static void Reset(GameProgress& p);

    // Load save; true if a valid save was read into p.
    static bool Load(GameProgress& p);

    // Write save. True on success.
    static bool Save(const GameProgress& p);

    // v2.23: import a loose "freddy" save (game:\freddy or game:\save\freddy)
    // into the XContent save container. Called once at boot in the system
    // build; a no-op in the Live Safe build (which reads the loose file
    // directly and has no XContent). Returns true if a file was found+imported.
    static bool ImportSave();

    // v2.23: true when a valid loose "freddy" save exists to import (so the
    // front-end can prompt first). No-op/false in the Live Safe build.
    static bool HasLooseSave();

    // v2.50: resolve the storage device once up front. On a single-device
    // console (Slim with just the HDD — no MU slots) the selector silently
    // returns the HDD (no UI shown per XDK docs), so all later saves/mounts
    // never invoke XAM's UI pipeline mid-flow (that was the ACCESS_DENIED
    // storm after boxes). Call soon after boot, before any message box.
    // No-op in the Live Safe build.
    static void PrimeStorage();

    // Number of title-screen stars (0..3) implied by the flags.
    static i32 StarCount(const GameProgress& p);

    // ---- v2.14 achievements (separate file, same storage root) ----
    // The achievement-unlock bitmask is kept OUT of GameProgress so the
    // Delete-key progress wipe never clears it. Stored as fnaf_ach.ini
    // (key `unlocked=N`, bit i-1 = achievement id i unlocked). Backend follows
    // the same FNAF_LIVE_SAFE switch as Load/Save.
    static bool LoadAchieve(u32* bits);
    static bool SaveAchieve(u32 bits);

    // ---- v2.62: the FNAF2 achievement mask (fnaf2_ach.ini) ----
    // bits 0..9 = the FNAF2 add-on slots (spa ids 11..20); kept SEPARATE from
    // the FNAF1 mask so the two games never step on each other.
    static bool LoadAchieveF2(u32* bits);
    static bool SaveAchieveF2(u32 bits);

    // ---- v2.62: the FNAF2 save ("freddy2", the dump's own INI file) ----
    // Keys mirror the original Ini extension exactly: level (the night the
    // 6 AM screen saved = the NEXT night), cine (dream counter), turn
    // (minigame rotation), beatgame (an ending screen was reached — unlocks
    // the 6th night), beat6 (night 6 — unlocks custom), beat7 (custom with
    // all 20; never read by the dump, kept for fidelity), c1..c10 (challenge
    // beaten flags).
    struct GameProgressF2 {
        i32 level;        // 1..8 (the next night; the title caps 5 for Continue)
        i32 cine;
        i32 turn;
        bool beatgame;
        bool beat6;
        bool beat7;
        bool c[10];       // c1..c10
    };
    static void ResetF2(GameProgressF2& p);
    static bool LoadF2(GameProgressF2& p);      // false = no/fresh save
    static bool SaveF2(const GameProgressF2& p);
    static void WipeF2();                       // the title X-hold wipe

    // ---- v2.62: the FNAF3 save ("freddy3") — the dump writes level (the
    // next night, after the next-day increment) and cine ----
    struct GameProgressF3 {
        i32 level;    // 1..7 (the next night to play)
        i32 cine;
    };
    static void ResetF3(GameProgressF3& p);
    static bool LoadF3(GameProgressF3& p);
    static bool SaveF3(const GameProgressF3& p);
};

} // namespace fnaf

#endif // FNAF_PROGRESS_H
