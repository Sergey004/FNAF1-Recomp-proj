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
#include <cstring>   // strlen — the ending types the dump's talk-box lines

namespace fnaf {

// v2.71: declared at NAMESPACE scope, not inside the function bodies — a
// block-scope extern inside a member function is bound to the GLOBAL
// namespace by MSVC 2010 (g++ follows the standard and binds it to the
// enclosing namespace), so the block form linked under g++ but produced
// LNK2019 under vc100 (it wanted ::Fnaf4CsLineText(int) while
// src/FNaF4Game.cpp defines fnaf::Fnaf4CsLineText). Definitions live in
// src/FNaF4Game.cpp.
const char* Fnaf4CsLineText(i32 row);
u32         Fnaf4CsLineColor(i32 row);
const char* Fnaf4EndLineText(i32 row);
u32         Fnaf4EndLineColor(i32 row);

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

// v2.67: native-size draw from the pak cell (plates / glyph strips)
void FNaF4Render::DrawNat(int handle, float fx, float fy, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  (f32)t->origWidth * kScaleX, (f32)t->origHeight * kScaleY, color);
}

// v2.67: Clickteam counter from its CNTR strip (alphabet 0-9-+.e cells);
// v2.69 anchor fix: the instance point is the string's BOTTOM-RIGHT corner —
// proven on frame_2 (the "Night" word art 121x65 @(41,31) ends flush left of
// the counter @(212,93)) and the frame_3 clock (the AM plate @(958,32) pairs
// with the hour counter @(947,68)).
void FNaF4Render::DrawCounterNat(int value, const int* strip, float fx, float fy,
                                 u32 color) {
    char digits[12];
    Snprintf(digits, sizeof(digits), "%d", value < 0 ? 0 : value);
    f32 total = -1.0f;
    for (const char* c = digits; *c; ++c) {
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", strip[*c - '0']);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (!t || !t->texture || !m_batch) return;
        total += (f32)t->origWidth + 1.0f;
    }
    f32 x = fx - total;
    for (const char* c = digits; *c; ++c) {
        const int h = strip[*c - '0'];
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", h);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        m_batch->Draw(t->texture, x * kScaleX, (fy - (f32)t->origHeight) * kScaleY,
                      (f32)t->origWidth * kScaleX, (f32)t->origHeight * kScaleY, color);
        x += (f32)t->origWidth + 1.0f;
    }
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

    // footer rows — v2.67: the dump's own art cells at their instance
    // spots: img 257 (398,733) "Press and hold DELETE…", img 964 (773,733)
    // the copyright banner, img 317 (979,712) the version badge. The debug
    // font rows (and their overlap struggle) are gone.
    DrawNat(257, 398.0f, 733.0f, 0xFFFFFFFF);
    DrawNat(964, 773.0f, 733.0f, 0xFFFFFFFF);
    DrawNat(317, 979.0f, 712.0f, 0xFFFFFFFF);
}

// ---- the between-night cards -------------------------------------------

void FNaF4Render::RenderNightStart(i32 night) {
    // frame 2 "what night": the fullscreen bedroom backdrop (img 654,
    // 1024x768 at (0,0), layer 0 — the bed with plush Fredbear) under the
    // dump's own row — the "Night" word art (img 670, 121x65 at (41,31)) +
    // the Night COUNTER (strip 685..698, alphabet 0-9-+.e, the white serif
    // 30x72 digits) at its instance (212,93) — bottom-right anchored, so the
    // digit row ends flush right of the word; the wipe band (img 676) it
    // plays behind and the corner clock icon (img 656, 38x39 at (960,706)).
    // No debug font left.
    DrawNat(654,   0.0f,   0.0f, 0xFFFFFFFF);
    DrawNat(676,   0.0f,  26.0f, 0xFFFFFFFF);
    DrawNat(670,  41.0f,  31.0f, 0xFFFFFFFF);
    static const int kNightStrip[14] = { 685, 686, 687, 688, 689, 690, 691,
                                         692, 693, 694, 695, 696, 697, 698 };
    DrawCounterNat(night, kNightStrip, 212.0f, 93.0f, 0xFFFFFFFF);
    DrawNat(656, 960.0f, 706.0f, 0xFFFFFFFF);
}

void FNaF4Render::RenderNightWin() {
    // frame 5 "night win": the frame draws ONLY the four num cells + the
    // colon (see RenderNightWinDigits) over black — nothing else (the old
    // "6 AM / night complete" debug caption is gone).
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

    // the bedroom clock = the "hour" Counter. Its glyph list [629..649] =
    // the seven-seg digits '0'..'9' (21x50); 650/651 = '--'/'+' service
    // glyphs; 668 = the clock icon, 671 = the lamp blob (NOT letters — the
    // earlier blob came from drawing them). The "AM" plate = img 652
    // (49x26, seven-seg). Hour 12 → '1','2'; hours 1..5 one digit; then AM.
    // v2.69: dump-exact placement — the counter instance point (947,68) is
    // the string's BOTTOM-RIGHT ("12" stacks leftwards), and the AM plate
    // sits at its layout left/top (958,32); the cluster rides the world pan.
    {
        static const i32 kHr[10] = { 629, 630, 631, 633, 643, 645, 646, 647, 648, 649 };
        const i32 h = game.GetHour();
        const f32 hx = -pan;
        if (h == 0 || h == 12) {
            Draw(kHr[1], 904.0f + hx, 18.0f, 21.0f, 50.0f, 0xFFFFFFFF);
            Draw(kHr[2], 926.0f + hx, 18.0f, 21.0f, 50.0f, 0xFFFFFFFF);
        } else if (h >= 1 && h <= 5) {
            Draw(kHr[h], 926.0f + hx, 18.0f, 21.0f, 50.0f, 0xFFFFFFFF);
        }
        DrawNat(652, 958.0f + hx, 32.0f, 0xFFFFFFFF);
    }
    // v2.66: the tutorial plate is the dump's OWN image (1183, 300x79 at
    // (20,15) — "Close doors- Hold SHIFT / Use Flashlight- Hold CTRL /
    // Press X to hide tips. / Perhaps some things are best left forgotten,
    // for now."), shown at the night start; plus the big WARNING box 1175
    // at its dump spot (512,294). The first 8 s, then they fade.
    if (game.GetHour() == 0 && game.GetClock() < 8.0f) {
        const f32 k = 1.0f - game.GetClock() / 8.0f;
        const u32 a = (u32)(255.0f * (k > 1.0f ? 1.0f : k));
        Draw(1183, 20.0f, 15.0f, 300.0f, 79.0f, (a << 24) | 0xFFFFFFFFu);
        Draw(1175, 512.0f, 294.0f, 401.0f, 188.0f, (a << 24) | 0xFFFFFFFFu);
    }
    // v2.67: the forced-closet state is shown by its own art (the
    // no-look-away closet views above); the "FLASH THE CLOSET" caption was
    // ours, not the game's — gone.
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
    // frame 4 "game over": the dump's own two cells — the blood-splat hold
    // (img 78) under the white flash (img 260). No words in the original.
    Draw( 78, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(260, 0.0f, 0.0f, 1024.0f, 768.0f, 0x30FFFFFF);
}

void FNaF4Render::RenderGameOver2() {
    // frame 8: the minigame catch hold (4 s) — the dump's own plate
    // "TOO BAD." (img 867, 308x47 @ (512,376))
    Draw(867, 512.0f, 376.0f, 308.0f, 47.0f, 0xFFFFFFFF);
}

void FNaF4Render::RenderIntro() {
    // frames 6/17 "intro to plushtrap": the frame holds ONE object — the
    // dump's own title card (img 1267, 630x346 native, hotspot-corrected
    // (203,188)). The debug-font caption is gone.
    DrawNat(1267, 203.0f, 188.0f, 0xFFFFFFFF);
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

    // the win stamp ("great!", img 868) at its dump spot (395,285 native)
    if (pt.won) DrawNat(868, 395.0f, 285.0f, 0xFFFFFFFF);
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
    // v2.67: the minigame counter = the dump's OWN 46x78 strip [829..838]
    // (alphabet 0-9-+.e behind it: [829..842]) at its instance (1009,88),
    // right-anchored for two digits. The "A(HOLD): flash" hint was ours —
    // the original wallpaper teaches the controls (intro card) — gone.
    {
        static const i32 kHr[10] = { 829, 830, 831, 832, 833, 834, 835, 836, 837, 838 };
        const int t = pt.clock < 0 ? 0 : pt.clock;
        if (t >= 10) {
            Draw(kHr[(t / 10) % 10], 919.0f, 88.0f, 46.0f, 78.0f, 0xFFFFFFFF);
            Draw(kHr[t % 10], 963.0f, 88.0f, 46.0f, 78.0f, 0xFFFFFFFF);
        } else {
            Draw(kHr[t], 963.0f, 88.0f, 46.0f, 78.0f, 0xFFFFFFFF);
        }
    }
}

void FNaF4Render::RenderLockbox(const FNaF4Game& game) {
    // frame 9 "lockbox": the dump's own set — the box art (img 736,
    // 1024x768 cover) + the two lock dials (img 750, 23x58 at (455,485) and
    // (544,485)) + the hint plate (img 733 "Perhaps some things are best
    // left forgotten, for now." — hotspot-corrected (323,154)). The debug
    // "Press A to unlock" caption is gone.
    (void)game;
    Draw(736, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    DrawNat(750, 455.0f, 485.0f, 0xFFFFFFFF);
    DrawNat(750, 544.0f, 485.0f, 0xFFFFFFFF);
    DrawNat(733, 323.0f, 154.0f, 0xFFFFFFFF);
}

void FNaF4Render::RenderExtras(const FNaF4Game& game) {
    // frame 10 "extras": the menu rows are the dump's OWN plates —
    // animatronics 911 (79,47) / making foxy 904 (80,87) / making fredbear
    // 912 (80,127) / jumpscares 914 (79,167) / fun with plushtrap 913
    // (80,207) / shadow freddy 1133 (82,247) / cheats 315 (84,288) /
    // challenges 361 (85,328) / BB 1253 (85,369) / exit 915 (80,707) —
    // over the static (img 936) + the red line (img 925). The invented
    // debug-font rows are gone; the selection keeps a red highlight.
    const FNaF4Game::ExtrasState& ex = game.Ex();
    Draw(936, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    DrawNat(925, 0.0f, 28.0f, 0xFFFFFFFF);
    static const i32 kImg[10] = { 911, 904, 912, 914, 913, 1133, 315, 361, 1253, 915 };
    static const f32 kX[10]  = { 79, 80, 80, 79, 80, 82, 84, 85, 85, 80 };
    static const f32 kY[10]  = { 47, 87, 127, 167, 207, 247, 288, 328, 369, 707 };
    for (int i = 0; i < 10; ++i)
        DrawNat(kImg[i], kX[i], kY[i], (ex.row == i) ? 0xFFFF8080 : 0xFFFFFFFF);
    (void)kX; (void)kY;
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
    // v2.67: the debug "N am" corner clock is gone — the cutscene frame
    // carries no clock object.
    // the talk box: the dump's OWN plate (img 1083, 812x158 — the dark
    // navy panel with the gray border, Active 13) at the screenshot spot,
    // the line left-aligned on it with the speaker color (the real font
    // on the console is the game's serif; the stub shows its own).
    Draw(1083, 106.0f, 66.0f, 812.0f, 158.0f, 0xFFFFFFFF);
    if (m_text && cs.lineCount > 0) {
        const char* line = Fnaf4CsLineText(cs.curLine);
        if (line)
            m_text->DrawText(150, 110, line, Fnaf4CsLineColor(cs.curLine));
    }
    if (cs.done) Draw(976, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF4Render::RenderEnding(const FNaF4Game& game) {
    // frame 13 "ending": the dump's set — the scanline backdrop (img 977),
    // the children sprites (1172 at (280,580), 1169 (780,540), 1170
    // (420,580)) + the talk box plate (img 1083, 812x158, hotspot-corrected
    // (140,100)) with the "talk box" Text object's REAL script typed out at
    // (160,120) in the dump's own colors, and the advance arrow (img 1084)
    // at (900,200) once the line is done typing (the game cadence lives in
    // FNaF4Game::TickEnding — v2.67; no invented text).
    Draw(977, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    DrawNat(1172, 280.0f, 580.0f, 0xFFFFFFFF);
    DrawNat(1170, 420.0f, 580.0f, 0xFFFFFFFF);
    DrawNat(1169, 780.0f, 540.0f, 0xFFFFFFFF);
    Draw(1083, 140.0f, 100.0f, 812.0f, 158.0f, 0xFFFFFFFF);
    if (m_text) {
        const i32 row = game.GetEndLine();
        const char* line = Fnaf4EndLineText(row);
        if (line) {
            const i32 n = game.GetEndLetters();
            char buf[96];
            i32 i = 0;
            for (const char* c = line; *c && i < n && i < 90; ++c, ++i)
                buf[i] = *c;
            buf[i] = '\0';
            m_text->DrawText(160, 120, buf, Fnaf4EndLineColor(row));
            // the arrow shows only when the line finished typing
            const int len = (int)strlen(line);
            if (n >= len) DrawNat(1084, 900.0f, 200.0f, 0xFFFFFFFF);
        }
    }
}

} // namespace fnaf
