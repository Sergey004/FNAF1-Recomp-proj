/**
 * FNaF4Render.cpp: v2.61 — the FNAF4 renderer (title + the night loop).
 * Dump data: frame 1 "titlescreen" placement table + the bedroom frame 3
 * "level" (1300x768). The view overlays are the full-frame art of the
 * dump's animation tables: hall peeks 89/255 (shut doors 88/375), the
 * closet 422 with the Foxy stages 304/286/288/290 (Fredbear 266), the bed
 * 511 with the Freddy-counter states 492/805/806/807 (<=10), 423 (11-20),
 * 386 (21-30), 391 (>30), the walk darks 45/160/57 and the paranoia flash
 * 99. Jumpscares: 450/487/620 (img 180's anims), the bedroom attacks
 * 592/358/368, the bite 642.
 */

#include "FNaF4Render.h"
#include "FNaF4Game.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <cstdio>

namespace fnaf {

static const f32 kScaleX = 1280.0f / 1024.0f;   // full-stretch 16:9
static const f32 kScaleY = 720.0f / 768.0f;

void FNaF4Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

void FNaF4Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  fw * kScaleX, fh * kScaleY, color);
}

void FNaF4Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = (wx - pan) * kScaleX;
    if (sx >= 1280.0f || sx + fw * kScaleX <= 0.0f) return;
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, sx, wy * kScaleY, fw * kScaleX, fh * kScaleY, color);
}

// ---- title (frame 1 "titlescreen") -------------------------------------

void FNaF4Render::RenderDisclaimer() {
    // frame 0: the dump's legal splash = ONE centered banner (Active
    // img 962, 551x92 at (248,284)); its String child sits off-screen in
    // the export and is never moved (the old red paragraphs were invented).
    Draw(962, 248.0f, 284.0f, 551.0f, 92.0f, 0xFFFFFFFF);
}

void FNaF4Render::RenderTitle(f32 time, i32 optionSelected, bool beat5) {
    if (!m_batch || !m_pak) return;
    (void)time;

    // bg (Backdrop img_626 — the red sky)
    Draw(626, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // the big heading (obj "title", img_658, hotspot-corrected (589,-2))
    Draw(658, 589.0f, -2.0f, 408.0f, 765.0f, 0xFFFFFFFF);

    // menu (hotspot-corrected left/top). No selector object was dumped for
    // this frame — the selected row is a red-tint highlight (labeled).
    static const struct { i32 img; f32 x, y, w, h; } kRows[4] = {
        { 730, 442.0f, 379.0f, 158.0f, 17.0f },   // "New Game"
        { 737, 442.0f, 425.0f, 151.0f, 17.0f },   // "Continue"
        { 738, 441.0f, 470.0f, 161.0f, 17.0f },   // "6th Night"
        { 731, 472.0f, 517.0f,  98.0f, 17.0f }    // "Extra"
    };
    for (i32 i = 0; i < 4; ++i) {
        u32 tint = 0xFFFFFFFF;
        if (i == (optionSelected & 3)) tint = 0xFF5050E0;         // selected
        if (i == 2 && !beat5)          tint = 0xFF606060;         // locked
        if (i == 3)                    tint = 0xFF909090;         // deferred
        Draw(kRows[i].img, kRows[i].x, kRows[i].y, kRows[i].w, kRows[i].h, tint);
    }

    // footer texts (v2.66: spread on the 1280 row — they used to overlap)
    if (m_text) {
        m_text->DrawText(300, 738, "Press and hold DELETE to erase all data.", 0xFFC04040);
        m_text->DrawText(850, 738, "Copyright (c) 2015 Scott Cawthon", 0xFFC04040);
        m_text->DrawText(1090, 712, "v1.1", 0xFFC04040);
    }
}

// ---- the between-night cards -------------------------------------------

void FNaF4Render::RenderNightStart(i32 night) {
    if (m_text) {
        char buf[24];
        Snprintf(buf, sizeof(buf), "Night %d", night);
        m_text->DrawText((int)(540.0f * kScaleX), (int)(340.0f * kScaleY), buf, 0xFFFFFFFF);
        m_text->DrawText((int)(560.0f * kScaleX), (int)(390.0f * kScaleY), "12 AM", 0xFF909090);
    }
}

void FNaF4Render::RenderNightWin() {
    // frame 5 "night win": the clock digits (img 663/678) animate 5 -> 6;
    // debug-font stopgap until the digit art is wired
    if (m_text) {
        m_text->DrawText((int)(560.0f * kScaleX), (int)(320.0f * kScaleY), "6 AM", 0xFFFFFFFF);
        m_text->DrawText((int)(520.0f * kScaleX), (int)(380.0f * kScaleY),
                         "night complete", 0xFF909090);
    }
}

// ---- bedroom (frame 3 "level", 1300x768) --------------------------------

void FNaF4Render::RenderBedroom(const FNaF4Game& game, f32 time, f32 pan) {
    if (!m_batch || !m_pak) return;
    (void)time;

    // the bedroom base
    DrawWorld(4, 0.0f, 0.0f, 1300.0f, 768.0f, pan, 0xFFFFFFFF);

    // the walk darks (world-size overlays; first frames 45/160/57)
    if (game.IsWalking()) {
        const f32 k = game.GetWalkT() / 0.45f;          // 1 -> 0
        const u32 a = (u32)(255.0f * (k > 0.5f ? (1.0f - k) * 2.0f : k * 2.0f));
        DrawWorld(game.GetWalkDir(), 0.0f, 0.0f, 1300.0f, 768.0f, pan,
                  (a << 24) | 0x00FFFFFFu);
    }

    // the flashlight peeks (full-frame art)
    if (game.Peeking()) {
        switch (game.GetPosition()) {
            case FNaF4Game::P_LEFT:
                Draw(game.IsDoorShut(0) ? 88 : 89, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                break;
            case FNaF4Game::P_RIGHT:
                Draw(game.IsDoorShut(1) ? 375 : 255, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                break;
            case FNaF4Game::P_CLOSET: {
                // the closet base pan (422) + the occupant art by stage
                Draw(422, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                if (game.IsFoxyInCloset()) {
                    if (game.IsFredbearCloset()) {
                        Draw(266, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                    } else {
                        // stage 0 -> 304, 1 -> 286, 2 -> 288, 3 -> 290
                        const i32 c = game.GetClosetCounter();
                        const i32 art = (c >= 6) ? 290 : (c >= 4) ? 288
                                      : (c >= 2) ? 286 : 304;
                        Draw(art, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                    }
                }
                break;
            }
            case FNaF4Game::P_BED: {
                // the bed art by the Freddy counter (g232-238)
                const i32 c = game.GetFreddyCounter();
                if (c <= 0)       Draw(511, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                else if (c <= 10) {
                    static const i32 kMini[4] = { 492, 805, 806, 807 };
                    Draw(kMini[c % 4], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                }
                else if (c <= 20) Draw(423, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                else if (c <= 30) Draw(386, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                else              Draw(391, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
                break;
            }
            default:
                break;
        }
    }
    // the forced-flashlight closet view (you cannot look away)
    else if (game.GetFoxyGotYou() && game.GetPosition() == FNaF4Game::P_CLOSET) {
        Draw(422, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        if (game.IsFredbearCloset()) Draw(266, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        else {
            const i32 c = game.GetClosetCounter();
            const i32 art = (c >= 6) ? 290 : (c >= 4) ? 288 : (c >= 2) ? 286 : 304;
            Draw(art, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }

    // the paranoia black flash (img 99)
    if (game.GetBlackFlashA() > 0.5f) {
        const u32 a = (u32)game.GetBlackFlashA();
        Draw(99, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }

    // v2.66 (CNTR list + visual check): the bedroom clock = the "hour"
    // counter at (947,68). Its glyph list [629..649] = the seven-seg digits
    // '0'..'9' (21x50); 650/651 = '--'/'+' service glyphs; 668 = the clock
    // icon, 671 = the lamp blob (NOT letters — the earlier blob came from
    // drawing them). The "AM" plate = img 652 (49x26, seven-seg).
    // Hour 12 → '1','2'; hours 1..5 one digit; then AM.
    {
        static const i32 kHr[10] = { 629, 630, 631, 633, 643, 645, 646, 647, 648, 649 };
        const i32 h = game.GetHour();
        if (h == 0 || h == 12) {
            Draw(kHr[1], 884.0f, 62.0f, 21.0f, 50.0f, 0xFFFFFFFF);
            Draw(kHr[2], 906.0f, 62.0f, 21.0f, 50.0f, 0xFFFFFFFF);
        } else if (h >= 1 && h <= 5) {
            Draw(kHr[h], 906.0f, 62.0f, 21.0f, 50.0f, 0xFFFFFFFF);
        }
        Draw(652, 930.0f, 74.0f, 49.0f, 26.0f, 0xFFFFFFFF);
    }
    if (m_text && game.GetFoxyGotYou())
        m_text->DrawText(480, 20, "FLASH THE CLOSET", 0xFF6060FF);
}

// ---- the jumpscare / bite overlays ---------------------------------------

void FNaF4Render::DrawAttack(const FNaF4Game& game) {
    if (game.GetAttackImg() != 0)
        Draw(game.GetAttackImg(), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    else if (game.GetBiteT() > 0.0f)
        Draw(642, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

// ============================================================
// v2.64 — the new flow screens. The minigame view anims are the dump's
// own "Active" tables (plushtrap: 735 idle, 879 chair, 748 hall, anim 14
// = [884..879] the jump-back, 15-18 the rooms, 19 stage 2, 20 stage 3,
// 22 the win, 23 the 21-cell jumpscare; BB mirrors them at 1225..1290).
// ============================================================

void FNaF4Render::RenderGameOver() {
    // frame 4: the death hold (7 s) — the dump hides its art objects and
    // waits; a dark hold with the caption stands in (labeled)
    if (m_text) {
        m_text->DrawText((int)(410.0f * kScaleX), (int)(300.0f * kScaleY),
                         "G A M E   O V E R", 0xFFC0C0C0);
        m_text->DrawText((int)(390.0f * kScaleX), (int)(420.0f * kScaleY),
                         "Press A to continue", 0xFF808080);
    }
}

void FNaF4Render::RenderGameOver2() {
    // frame 8: the minigame catch hold (4 s)
    if (m_text)
        m_text->DrawText((int)(430.0f * kScaleX), (int)(330.0f * kScaleY),
                         "CAUGHT", 0xFFC0C0C0);
}

void FNaF4Render::RenderIntro() {
    // frames 6/17: the intro fade holds; a caption stands in (labeled)
    if (m_text) {
        m_text->DrawText((int)(330.0f * kScaleX), (int)(300.0f * kScaleY),
                         "FUN WITH PLUSHTRAP", 0xFFE0E0E0);
        m_text->DrawText((int)(400.0f * kScaleX), (int)(380.0f * kScaleY),
                         "Press A to start", 0xFF909090);
    }
}

void FNaF4Render::RenderNightWinDigits(const FNaF4Game& game) {
    // the 6 AM clock: the dump's OWN digit strip — the "num 1..4" actives
    // run a 10-frame sequence [663,664,665,666,667,669,672,673,674,675]
    // (index = digit; act #40 writes Random(10) until the settles at the
    // 2/2.5/3/3.5 s beats → "06:00") plus the colon (img 678). Layout:
    // num1..4 at (315,266)(413,266)(544,266)(642,266), cells 100x200,
    // colon 23x88 at (519,325)  — verbatim from frame_5 Night win.
    static const i32 kNum[10] = { 663, 664, 665, 666, 667, 669, 672, 673, 674, 675 };
    static const f32 kX[4] = { 315.0f, 413.0f, 544.0f, 642.0f };
    for (int i = 0; i < 4; ++i) {
        const int v = game.GetNightWinVal(i);
        Draw(kNum[v < 0 ? 0 : (v > 9 ? 9 : v)], kX[i], 266.0f, 100.0f, 200.0f,
             0xFFFFFFFF);
    }
    Draw(678, 519.0f, 325.0f, 23.0f, 88.0f, 0xFFFFFFFF);
}

void FNaF4Render::RenderMinigame(const FNaF4Game& game) {
    const FNaF4Game::PtState& pt = game.Pt();
    if (!m_batch) return;

    // the view animation: the dump's Active.anim tables
    static const int kPT[24] = {
        735, 735, 735, 735, 735, 735, 735, 735, 735, 735, 735, 735,
        879, 748, 884, 756, 776, 782, 788, 734, 872, 871, 878, 808
    };
    static const int kBB[24] = {
        735, 735, 735, 735, 735, 735, 735, 735, 735, 735, 735, 735,
        1225, 748, 1230, 1231, 1237, 1279, 1285, 1255, 1261, 1260, 1266, 1161
    };
    int anim = pt.viewAnim;
    if (pt.viewState == 0) anim = 0;
    if (pt.viewState == 1) {
        // the light-on look: the pose per position (g23-26)
        anim = (pt.hallPos == 0) ? 12 : (pt.hallPos == 4) ? 21 :
               (pt.hallPos == 7) ? 22 : 13;
    }
    const int* tab = (pt.game == 1) ? kBB : kPT;
    Draw(tab[anim < 24 ? anim : 0], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // the plushtrap/BB sprite in the mid-hall positions
    if (!pt.won && !pt.scare && pt.hallPos >= 2 && pt.hallPos <= 7)
        Draw(849, 892.0f, 478.0f, 67.0f, 24.0f, 0xFFFFFFFF);

    // the win stamp (img 868)
    if (pt.won) Draw(868, 514.0f, 308.0f, 120.0f, 60.0f, 0xFFFFFFFF);
    if (pt.scare) {
        // the jumpscare anim 23 (21 / 30 cells)
        static const int kScPT[21] = {
            808, 809, 810, 811, 812, 813, 814, 815, 816, 817, 818,
            819, 820, 821, 822, 823, 824, 825, 826, 827, 828
        };
        static const int kScBB[30] = {
            1161, 1162, 1176, 1185, 1187, 1188, 1189, 1190, 1191, 1192,
            1193, 1194, 1195, 1196, 1197, 1198, 1199, 1200, 1201, 1202,
            1203, 1243, 1244, 1245, 1246, 1247, 1248, 1249, 1250, 1252
        };
        const int n = (pt.game == 1) ? 30 : 21;
        const int f = (int)(game.GetClock() * (pt.game == 1 ? 45.0f : 30.0f)) % n;
        Draw(pt.game == 1 ? kScBB[f] : kScPT[f], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    }
    // v2.66: the minigame clock = the dump's own 20x39 digit strip
    // ([211..220], the same strip the bedroom clock uses)
    {
        static const i32 kHr[10] = { 211, 212, 213, 214, 215, 216, 217, 218, 219, 220 };
        const int t = pt.clock < 0 ? 0 : pt.clock;
        if (t >= 10) {
            Draw(kHr[(t / 10) % 10], 40.0f, 20.0f, 20.0f, 39.0f, 0xFFFFFFFF);
            Draw(kHr[t % 10], 62.0f, 20.0f, 20.0f, 39.0f, 0xFFFFFFFF);
        } else {
            Draw(kHr[t], 40.0f, 20.0f, 20.0f, 39.0f, 0xFFFFFFFF);
        }
    }
    if (m_text)
        m_text->DrawText((int)(700.0f * kScaleX), (int)(700.0f * kScaleY),
                         "A(HOLD): flash", 0xFF909090);
}

void FNaF4Render::RenderLockbox(const FNaF4Game& game) {
    // frame 9: the unlock box hold; the box art rows were not captured —
    // a dark hold + the caption stands in (labeled)
    (void)game;
    if (m_text) {
        m_text->DrawText((int)(390.0f * kScaleX), (int)(300.0f * kScaleY),
                         "T O Y   B O X", 0xFFE0E0E0);
        m_text->DrawText((int)(380.0f * kScaleX), (int)(380.0f * kScaleY),
                         "Press A to unlock", 0xFF909090);
    }
}

void FNaF4Render::RenderExtras(const FNaF4Game& game) {
    const FNaF4Game::ExtrasState& ex = game.Ex();
    static const char* kRows[10] = {
        "ANIMATRONICS", "MAKING OF", "PLUSHTRAP MAKING", "JUMPSCARES",
        "FUN WITH PLUSHTRAP", "SHADOW NIGHTS", "CHEATS", "CHALLENGES",
        "FUN WITH BALLOON BOY", "EXIT"
    };
    if (m_text) {
        for (int i = 0; i < 10; ++i) {
            m_text->DrawText((int)(380.0f * kScaleX),
                             (int)((150.0f + i * 46.0f) * kScaleY), kRows[i],
                             (ex.row == i) ? 0xFFFFFFFF : 0xFF909090);
        }
    }
}

void FNaF4Render::RenderCutscene(const FNaF4Game& game) {
    const FNaF4Game::CutsceneState& cs = game.Cut();
    if (!m_batch) return;
    // the house world: the walk layer + the dump's own pieces (hitbox 980,
    // the boy 983, the screen-follow 979). v2.66g: the REAL dialogue —
    // the strings recovered from the frame-12 events (act #88), the
    // speaker colors verbatim; the talk box (img 1083 plate family).
    const f32 ox = 512.0f - cs.camX, oy = 384.0f - cs.camY;
    Draw(980, ox + cs.px - 31.0f, oy + cs.py - 31.0f, 62.0f, 62.0f, 0xFFFFFFFF);
    Draw(983, ox + cs.px - 51.0f, oy + cs.py + 20.0f, 102.0f, 120.0f, 0xFF909090);
    // the WASD hint (img 311) rides above the player per the dump instance
    Draw(311, ox + cs.px - 55.0f, oy + cs.py - 120.0f, 110.0f, 46.0f, 0xFFFFFFFF);
    if (m_text) {
        char b[48];
        Snprintf(b, sizeof(b), "%d am", game.GetHour() == 0 ? 12 : game.GetHour());
        m_text->DrawText(1080, 40, b, 0xFF606060);
    }
    // the talk box with the CURRENT line (verbatim + the speaker color)
    if (m_text && cs.lineCount > 0) {
        extern const char* Fnaf4CsLineText(i32 row);
        extern u32 Fnaf4CsLineColor(i32 row);
        const char* line = Fnaf4CsLineText(cs.curLine);
        if (line)
            m_text->DrawTextCentered(600, line, Fnaf4CsLineColor(cs.curLine), 0);
    }
    if (cs.done) Draw(976, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF4Render::RenderEnding(const FNaF4Game& game) {
    // frame 13: the typewriter talk box; the dump's letters are sprite
    // images without a string table — the box + advance hint render on the
    // dump cadence with no invented text (labeled stop-gap)
    (void)game;
    if (m_text) {
        m_text->DrawText((int)(300.0f * kScaleX), (int)(560.0f * kScaleY),
                         "..................................................",
                         0xFFE0E0E0);
        m_text->DrawText((int)(300.0f * kScaleX), (int)(620.0f * kScaleY),
                         "A: next   B: skip", 0xFF909090);
    }
}

} // namespace fnaf
