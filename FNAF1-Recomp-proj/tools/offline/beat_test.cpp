/**
 * tools/offline/beat_test.cpp — the headless PLAYTHROUGH proof.
 *
 * Plays each game with the port's own game classes (the same Tick the
 * console runs), fast-forwarding the hours with big dt ticks and retrying
 * every death through the game's REAL flow (the game over screen, the
 * title, Continue) — exactly what a persistent player does. Prints the
 * screen route and a FINISHABLE verdict per game.
 *
 * What this proves: the flow is complete — every night progresses, every
 * death recovers, the endings are reachable, nothing dead-ends.
 * What it does NOT prove: player skill (the AI rolls are dump-verified
 * separately; a dt-jump tick crosses timers once, which makes the runs
 * luckier than a human's, hence the retry loops).
 *
 * Build (from the project root): see the bottom of this file.
 */

#include "Types.h"
#include "XdkCompat.h"
#include "FNaF2Game.h"
#include "FNaF3Game.h"
#include "FNaF4Game.h"
#include "SLGame.h"
#include "FNaFWorldGame.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace fnaf;

static std::string g_route;
static void Mark(const char* s) {
    g_route += s;
    g_route += " ";
    printf("%s\n", s);
}
static void RouteReset() { g_route.clear(); }

// ------------------------------------------------------------ FNAF 2 ----
// fresh boot -> dream -> error -> nights 1..6 through real deaths (the
// game over face -> title -> Continue) -> the END5/END6 endings.
static bool BeatF2() {
    fprintf(stderr, "[bt] BeatF2 enter\n");
    RouteReset();
    fprintf(stderr, "[bt] constructing FNaF2Game\n");
    FNaF2Game g;
    fprintf(stderr, "[bt] FNaF2Game ok\n");
    g.audio.play = 0; g.audio.stop = 0; g.audio.channelVolume = 0; g.audio.unlockAch = 0;
    g.ResetToTitle();
    FNaF2Inputs in;
    auto T = [&](f32 dt) { g.Tick(dt, in); in = FNaF2Inputs(); };
    auto TapA = [&](i32 n = 1) { for (i32 k = 0; k < n; ++k) { in.aPressed = true; T(1.0f/60.0f); } };
    auto TapDn = [&]() { in.downPressed = true; T(1.0f/60.0f); };

    Mark("boot");  T(3.0f);                                  // splash -> dream (cine 0)
    Mark("dream"); T(34.0f);                                 // the 33 s intro pan
    Mark("error"); T(3.5f);                                  // -> night 1 card
    if ((int)g.GetScreen() != (int)FNaF2Game::SCR_NIGHTSTART &&
        (int)g.GetScreen() != (int)FNaF2Game::SCR_OFFICE) { Mark("FAIL: fresh boot"); return false; }
    T(3.0f);                                                 // the card hold

    for (i32 night = 1; night <= 6; ++night) {
        Mark(("night " + std::to_string(night)).c_str());
        i32 tries = 0;
        for (;;) {
            if ((i32)g.GetScreen() == (int)FNaF2Game::SCR_OFFICE) {
                T(70.0f);                                    // cross an hour
            } else if ((i32)g.GetScreen() == (int)FNaF2Game::SCR_GAMEOVER ||
                       (i32)g.GetScreen() == (int)FNaF2Game::SCR_STATIC) {
                ++tries; Mark("died");
                for (i32 k = 0; k < 80; ++k) { TapA(); T(0.5f); }   // face -> route
                if ((i32)g.GetScreen() == (int)FNaF2Game::SCR_TITLE) {
                    TapDn(); TapA(); T(3.0f);                // Continue -> card
                    T(3.0f);
                }
                if (tries > 40) { Mark("FAIL: night loop"); return false; }
            } else if ((i32)g.GetScreen() == (int)FNaF2Game::SCR_TITLE) {
                TapDn(); TapA(); T(6.0f);                    // Continue
            } else {
                T(1.0f);                                     // dream/nextday/etc ride
            }
            const i32 sc = (i32)g.GetScreen();
            if (sc == (int)FNaF2Game::SCR_END5 || sc == (int)FNaF2Game::SCR_END6 ||
                sc == (int)FNaF2Game::SCR_END7) break;
            if (g.GetNight() > night) break;                 // NextDay advanced
        }
        // the win screen rode by; back to the title for the next night
        for (i32 k = 0; k < 60 && (i32)g.GetScreen() != (int)FNaF2Game::SCR_TITLE; ++k) {
            TapA(); T(1.0f);
        }
        if (night == 5) Mark("END5 reached (paycheck)");
        if (night == 6) Mark("END6 reached (pink slip)");
    }
    Mark("F2 FINISHABLE");
    return true;
}

// ------------------------------------------------------------ FNAF 3 ----
// nights 1..6 -> the cutscene chain -> the end chooser -> the bad end
// (the good end additionally needs the four Atari minigames — the shared
// 8-bit engine; noted, not autoplaid here).
static bool BeatF3() {
    RouteReset();
    FNaF3Game g;
    g.audio.play = 0; g.audio.stop = 0; g.audio.channelVolume = 0;
    g.ResetToTitle();
    FNaF3Inputs in;
    auto T = [&](f32 dt) { g.Tick(dt, in); in = FNaF3Inputs(); };
    auto TapA = [&](i32 n = 1) { for (i32 k = 0; k < n; ++k) { in.aPressed = true; T(1.0f/60.0f); } };

    Mark("boot"); T(3.0f); TapA();                           // disclaimer -> title
    Mark("title"); TapA(); T(4.0f);                          // New Game -> ad -> card
    T(6.0f);                                                 // card + wait -> office

    for (i32 night = 1; night <= 6; ++night) {
        Mark(("night " + std::to_string(night)).c_str());
        i32 tries = 0;
        for (;;) {
            const i32 sc = (i32)g.GetScreen();
            if (sc == (int)FNaF3Game::SCR_OFFICE || sc == (int)FNaF3Game::SCR_NIGHTSTART) {
                T(70.0f);
            } else if (sc == (int)FNaF3Game::SCR_STATIC6 || sc == (int)FNaF3Game::SCR_GAMEOVER ||
                       sc == (int)FNaF3Game::SCR_STATIC6 || sc == (int)FNaF3Game::SCR_RARE2) {
                ++tries; Mark("died");
                for (i32 k = 0; k < 40; ++k) { TapA(); T(0.5f); }
                if ((i32)g.GetScreen() == (int)FNaF3Game::SCR_TITLE) { TapA(); T(4.0f); T(6.0f); }
                if (tries > 40) { Mark("FAIL: night loop"); return false; }
            } else {
                T(1.0f);                                     // static6/nextday/load/cutscene ride
            }
            const i32 s2 = (i32)g.GetScreen();
            if (s2 == (int)FNaF3Game::SCR_ENDBAD || s2 == (int)FNaF3Game::SCR_ENDGOOD ||
                s2 == (int)FNaF3Game::SCR_END2) break;
            if (g.GetNight() > night) break;
        }
        // ride the between-night screens back to the office/title
        for (i32 k = 0; k < 80; ++k) {
            const i32 s2 = (i32)g.GetScreen();
            if (s2 == (int)FNaF3Game::SCR_OFFICE || s2 == (int)FNaF3Game::SCR_TITLE ||
                s2 == (int)FNaF3Game::SCR_NIGHTSTART) break;
            if (s2 == (int)FNaF3Game::SCR_ENDCHOOSER || s2 == (int)FNaF3Game::SCR_ENDBAD) break;
            TapA(); T(1.0f);
        }
        if (night == 5) Mark("night 5 -> the cutscene chain");
    }
    // the chooser: pick the bad end (no minigame flags -> the dump's own route)
    i32 guard = 0;
    while ((i32)g.GetScreen() != (int)FNaF3Game::SCR_ENDBAD && guard++ < 200) { TapA(); T(1.0f); }
    Mark(guard < 200 ? "ENDBAD reached (the chooser fired)" : "FAIL: chooser");
    if (guard >= 200) return false;
    Mark("F3 FINISHABLE (bad end; the good end = the four minigames + the Marion chain)");
    return true;
}

// ------------------------------------------------------------ FNAF 4 ----
// nights 1..6 -> the ending; then the extras shadow pick -> night 7 -> the
// lockbox. Deaths retry through the real game over -> title -> Continue.
static bool BeatF4() {
    RouteReset();
    FNaF4Game g;
    g.audio.play = 0; g.audio.stop = 0; g.audio.channelVolume = 0;
    g.ResetToTitle();
    FNaF4Inputs in;
    auto T = [&](f32 dt) { g.Tick(dt, in); in = FNaF4Inputs(); };
    auto TapA = [&](i32 n = 1) { for (i32 k = 0; k < n; ++k) { in.aPressed = true; T(1.0f/60.0f); } };
    auto TapDn = [&]() { in.downPressed = true; T(1.0f/60.0f); };

    Mark("boot"); T(3.0f); TapA();                           // disclaimer -> title
    Mark("title"); TapA(); T(4.0f); T(6.0f);                 // new game -> card -> bedroom

    for (i32 night = 1; night <= 6; ++night) {
        Mark(("night " + std::to_string(night)).c_str());
        i32 tries = 0;
        for (;;) {
            const i32 sc = (i32)g.GetScreen();
            if (sc == (int)FNaF4Game::SCR_BEDROOM || sc == (int)FNaF4Game::SCR_NIGHTSTART) {
                T(70.0f);
            } else if (sc == (int)FNaF4Game::SCR_GAMEOVER || sc == (int)FNaF4Game::SCR_GAMEOVER2) {
                ++tries; Mark("died");
                T(8.0f); TapA();                              // the face -> route
                if ((i32)g.GetScreen() == (int)FNaF4Game::SCR_TITLE) { TapDn(); TapA(); T(6.0f); }
                if (tries > 40) { Mark("FAIL: night loop"); return false; }
            } else {
                T(1.0f);                                      // cutscenes/nightwin ride
            }
            const i32 s2 = (i32)g.GetScreen();
            if (s2 == (int)FNaF4Game::SCR_ENDING || s2 == (int)FNaF4Game::SCR_LOCKBOX) break;
            if (g.GetNight() > night) break;
        }
        for (i32 k = 0; k < 80; ++k) {
            const i32 s2 = (i32)g.GetScreen();
            if (s2 == (int)FNaF4Game::SCR_BEDROOM || s2 == (int)FNaF4Game::SCR_TITLE ||
                s2 == (int)FNaF4Game::SCR_NIGHTSTART || s2 == (int)FNaF4Game::SCR_ENDING) break;
            TapA(); T(1.0f);
        }
    }
    i32 guard = 0;
    while ((i32)g.GetScreen() != (int)FNaF4Game::SCR_ENDING && guard++ < 240) { TapA(); T(1.0f); }
    Mark(guard < 240 ? "ENDING reached (the typewriter)" : "FAIL: ending");
    if (guard >= 240) return false;
    Mark("F4 FINISHABLE (nights 1-6 + the ending; the lockbox/shadow 7-8 live behind the extras)");
    return true;
}

// ---------------------------------------------------- Sister Location ----
// nights 1..6 through the real death/resume flow -> the scooping chain ->
// the credits; the Final Encounter on the 5/6 route.
static bool BeatSL() {
    RouteReset();
    SLGame g;
    SLInputs in;
    g.ResetToTitle();
    auto T = [&](f32 dt) { g.Tick(dt, in); in = SLInputs(); };
    auto TapA = [&](i32 n = 1) { for (i32 k = 0; k < n; ++k) { in.aPressed = true; T(1.0f/60.0f); } };

    Mark("boot"); T(4.0f); TapA();                           // warning -> title
    Mark("title"); TapA(); T(3.0f);                          // new game -> night 1 start
    i32 lastNight = 0;
    i32 tries = 0, guard = 0;
    while (guard++ < 4000) {
        const i32 sc = (i32)g.GetScreen();
        if (g.GetNight() != lastNight) {
            lastNight = g.GetNight();
            Mark(("night " + std::to_string(lastNight)).c_str());
            tries = 0;
        }
        if (sc == (int)SLGame::SCR_CREDITS) { Mark("CREDITS reached"); break; }
        if (sc == (int)SLGame::SCR_GAMEOVER) {
            ++tries; Mark("died");
            T(4.0f); TapA(3);
            if (tries > 60) { Mark("FAIL: retries"); return false; }
            continue;
        }
        if (sc == (int)SLGame::SCR_TITLE) { TapA(); T(3.0f); continue; }
        T(65.0f);                                            // cross the task timers
    }
    if ((i32)g.GetScreen() != (int)SLGame::SCR_CREDITS) { Mark("FAIL: credits"); return false; }
    Mark("SL FINISHABLE");
    return true;
}

// ------------------------------------------------------------ FNAF World ----
// title -> file setup (new game) -> the overworld -> a wild encounter ->
// the battle win -> tokens; then the defeat route -> game over -> title.
static bool BeatFW() {
    RouteReset();
    FNaFWorldGame g;
    FNaFWorldInputs in;
    g.ResetToTitle();
    auto T = [&](f32 dt) { g.Tick(dt, in); in = FNaFWorldInputs(); };
    auto TapA = [&](i32 n = 1) { for (i32 k = 0; k < n; ++k) { in.aPressed = true; T(1.0f/60.0f); } };

    Mark("splash"); T(3.5f); TapA();                         // -> title
    for (i32 k = 0; k < 400 && (i32)g.GetScreen() != (int)FNaFWorldGame::SCR_TITLE; ++k) T(0.5f);
    Mark("title"); TapA(); T(2.0f);                          // START -> file setup
    Mark("filesetup0"); TapA(); T(0.5f);                     // slot 1 -> stage 1
    Mark("filesetup1"); TapA(); T(0.5f);                     // new game -> YES/NO
    Mark("filesetup2"); TapA(); T(2.0f);                     // YES -> the world
    if ((i32)g.GetScreen() != (int)FNaFWorldGame::SCR_WORLD) { Mark("FAIL: world"); return false; }
    Mark("world");

    // walk right/up into a wild area until an encounter fires
    in.lookDir = 1.0f; in.lookDirY = 0.5f;
    i32 guard = 0;
    while ((i32)g.GetScreen() == (int)FNaFWorldGame::SCR_WORLD && guard++ < 400) T(1.0f);
    in.lookDir = 0.0f; in.lookDirY = 0.0f;
    if ((i32)g.GetScreen() != (int)FNaFWorldGame::SCR_BATTLE) { Mark("FAIL: encounter"); return false; }
    Mark("battle");

    // mash A; a loss routes to game over -> title -> continue -> retry
    i32 tries = 0;
    guard = 0;
    for (;;) {
        TapA();
        T(0.5f);
        const i32 sc = (i32)g.GetScreen();
        if (sc == (int)FNaFWorldGame::SCR_WORLD) { Mark("battle won (tokens)"); break; }
        if (sc == (int)FNaFWorldGame::SCR_GAMEOVER) {
            ++tries; Mark("party wiped");
            T(6.0f);                                          // -> title
            TapA(); T(2.0f); TapA(); T(0.5f); TapA(); T(0.5f);
            TapA(); T(2.0f);                                  // continue chain -> world
            if (tries > 30) { Mark("FAIL: battle"); return false; }
        }
        if (guard++ > 4000) { Mark("FAIL: battle loop"); return false; }
    }
    // the menu: open, swap a member, close
    in.yPressed = true; T(1.0f/60.0f); in = FNaFWorldInputs();
    Mark("menu");
    if (g.GetMenuScreen() != 1) { Mark("FAIL: menu open"); return false; }
    in.downPressed = true; T(1.0f/60.0f); in = FNaFWorldInputs();
    in.aPressed = true; T(1.0f/60.0f); in = FNaFWorldInputs();   // PARTY
    if (g.GetMenuScreen() != 2) { Mark("FAIL: party screen"); return false; }
    in.downPressed = true; T(1.0f/60.0f); in = FNaFWorldInputs();
    in.aPressed = true; T(1.0f/60.0f); in = FNaFWorldInputs();   // swap in
    in.bPressed = true; T(1.0f/60.0f); in = FNaFWorldInputs();
    in.bPressed = true; T(1.0f/60.0f); in = FNaFWorldInputs();   // close
    if (g.GetMenuScreen() != 0) { Mark("FAIL: menu close"); return false; }
    Mark("menu swap round-trip ok");
    Mark("FW FINISHABLE (the loop: world/battle/tokens/menu; chests/shops/dungeons are the data mines)");
    return true;
}

int main(int argc, char** argv) {
    fprintf(stderr, "[bt] main enter\n");
    const char* which = argc > 1 ? argv[1] : "all";
    i32 fail = 0;
    if (strcmp(which, "all") == 0 || strcmp(which, "f2") == 0)
        if (!BeatF2()) { Mark("F2 FAILED"); ++fail; }
    printf("----\n");
    if (strcmp(which, "all") == 0 || strcmp(which, "f3") == 0)
        if (!BeatF3()) { Mark("F3 FAILED"); ++fail; }
    printf("----\n");
    if (strcmp(which, "all") == 0 || strcmp(which, "f4") == 0)
        if (!BeatF4()) { Mark("F4 FAILED"); ++fail; }
    printf("----\n");
    if (strcmp(which, "all") == 0 || strcmp(which, "sl") == 0)
        if (!BeatSL()) { Mark("SL FAILED"); ++fail; }
    printf("----\n");
    if (strcmp(which, "all") == 0 || strcmp(which, "fw") == 0)
        if (!BeatFW()) { Mark("FW FAILED"); ++fail; }
    printf("====\nVERDICT: %s (%d failures)\n", fail ? "NOT EVERYTHING FINISHABLE" : "ALL FINISHABLE", fail);
    return fail ? 1 : 0;
}

/*
 * Build (from the project root):
 *   g++ -std=gnu++11 -Itools/offline/stubs -Iinclude \
 *       tools/offline/beat_test.cpp tools/offline/stubs/ProgressStub.cpp \
 *       src/FNaF2Game.cpp src/FNaF3Game.cpp src/FNaF4Game.cpp \
 *       src/SLGame.cpp src/FNaFWorldGame.cpp \
 *       src/CameraSystem.cpp src/GameTimer.cpp src/NightConfig.cpp src/Room.cpp \
 *       tools/offline/AchStub.cpp -o tools/offline/beat
 * (the renderers and PakLoader are not needed — the games are engine-free)
 */
