/**
 * FNaF2Render.cpp: v2.31 — the FNAF2 renderer (see FNaF2Render.h).
 * All coordinates/handles are dump data: frame layouts, placement
 * tables (FrameLayout/frame_N_*.txt) and asset_mapping_fnaf2.hpp.
 */

#include "FNaF2Render.h"
#include "FNaF2Game.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"   // Snprintf — XDK CRT predates C99 snprintf
#include <cstdio>

namespace fnaf {

// frame 1024x768 -> 720p pillarbox: scale 0.9375, 160 px bars
// frame 1024x768 -> FULL-STRETCH 1280x720 (PC-window behaviour):
// X scale 1.25, Y scale 0.9375, no bars
static const f32 kScaleX = 1280.0f / 1024.0f;   // 1.25
static const f32 kScaleY = 720.0f / 768.0f;     // 0.9375

void FNaF2Render::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

// ---- private helpers --------------------------------------------------

void FNaF2Render::Draw(int handle, float fx, float fy, float fw, float fh, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    // v2.72: clamp UVs to the ORIGINAL size — textures are GPU-padded out to
    // 32px-aligned dims and the padding rows hold junk (white stripes on the
    // title glyphs); plain 0..1 sampling stretched that junk into the quad.
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  fw * kScaleX, fh * kScaleY, 0.0f, 0.0f, u1, v1, color);
}

// native-size draw (counters/glyphs and plates whose dims live in the pak)
void FNaF2Render::DrawNat(int handle, float fx, float fy, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, fx * kScaleX, fy * kScaleY,
                  (f32)t->origWidth * kScaleX, (f32)t->origHeight * kScaleY,
                  0.0f, 0.0f, u1, v1, color);
}

// v2.67: a Clickteam counter rendered from its CNTR glyph list — the dump's
// dt=0 strips are the alphabet [0-9 - + . e] (14 cells, per-glyph native
// widths); only the digits matter for these games. The instance point is the
// string's BOTTOM-RIGHT corner — verified against frame_3 (the "12" right edge
// sits 14 px left of the AM plate @(956,78) and shares its baseline, "Night 1"
// pairs with its plate @(863,34)..34+22) and frame_12 (a one-digit AI level
// lands between the < > arrows, snug to the right one).
void FNaF2Render::DrawCounterNat(int value, const int* strip, float fx, float fy,
                                 u32 color) {
    if (value < 0) value = -value;                 // the '-' cell rides strip[10]
    char digits[12];
    Snprintf(digits, sizeof(digits), "%d", value);
    f32 total = -1.0f;                             // CTF digits butt-join + 1px
    for (const char* c = digits; *c; ++c) {
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", strip[*c - '0']);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (!t || !t->texture || !m_batch) return;
        total += (f32)t->origWidth + 1.0f;
    }
    f32 x = fx - total;
    for (const char* c = digits; *c; ++c) {
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", strip[*c - '0']);
        PakLoadedTexture* t = m_pak->FindTexture(name);
        const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
        const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
        m_batch->Draw(t->texture, x * kScaleX, (fy - (f32)t->origHeight) * kScaleY,
                      (f32)t->origWidth * kScaleX, (f32)t->origHeight * kScaleY,
                      0.0f, 0.0f, u1, v1, color);
        x += (f32)t->origWidth + 1.0f;
    }
}

void FNaF2Render::DrawWorld(int handle, float wx, float wy, float fw, float fh,
                            float pan, u32 color) {
    const f32 sx = (wx - pan) * kScaleX;
    if (sx >= 1280.0f || sx + fw * kScaleX <= 0.0f) return;   // off-window
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, sx, wy * kScaleY, fw * kScaleX, fh * kScaleY,
                  0.0f, 0.0f, u1, v1, color);
}

int FNaF2Render::StaticFrame(f32 time) const {
    // obj "static" anim 0 = [332,334,328,329,330,331] @ speed 99 = 59.4 FPS
    static const int kFrames[6] = { 332, 334, 328, 329, 330, 331 };
    const f32 period = 100.0f / 99.0f / 60.0f;   // speed->fps = ×0.6
    return kFrames[(int)(time / period) % 6];
}

// ---- disclaimer (frame 0 "Frame 17"): black + white mono warning ----
void FNaF2Render::RenderDisclaimer(const FNaF2Game& game) {
    // frame 0 "Frame 17": the dump draws ONE centered banner (Backdrop
    // img 574, 535x121 at (268,278), hot (-17,-7)); its String child sits
    // off-screen in the export and is never moved — the legal splash IS
    // the banner (the old debug-font paragraphs were invented).
    (void)game;
    Draw(574, 268.0f, 278.0f, 535.0f, 121.0f, 0xFFFFFFFF);
}

// ---- title (frame 1 "title", 1024x768) --------------------------------

void FNaF2Render::RenderTitle(const FNaF2Game& game, f32 time) {
    if (!m_batch || !m_pak) return;

    // ---- the background "video FROM Freddy's eyes" (dump groups 4+8-11):
    // at boot alterable[0] == 0 -> anim 12 = img_362 (the eye view IS the
    // first scene); every 2 s a Random(50) re-roll maps 0/1/2 to the eye
    // views 362/470/215 and anything else to the normal bg 321. We use a
    // stateless hash per 2 s slot (same 3/51 duty as Random(50)).
    int bgImg = 321;
    {
        const f32 kRollPeriod = 2.0f;
        const int slot = (int)(time / kRollPeriod);
        if (slot == 0) {
            bgImg = 362;                                // boot state: eye view
        } else {
            u32 h = (u32)slot * 2654435761u + 1u;
            h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
            const int roll = (int)(h % 51u);
            bgImg = (roll == 0) ? 362 : (roll == 1) ? 470 : (roll == 2) ? 215 : 321;
        }
    }
    // group 5: the bg's alpha coefficient := Random(250) every 6 s — the
    // FNAF2 "lamp" (the backdrop breathes dim-to-bright; coeff -> alpha =
    // (255-c)/255, the same convention as FNAF1's v2.57 title lamp).
    {
        u32 h = (u32)(int)(time / 6.0f) * 2654435761u + 5u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const u32 lampAlpha = 255u - (u32)(h % 250u);
        Draw(bgImg, 0.0f, 0.0f, 1024.0f, 768.0f, (lampAlpha << 24) | 0x00FFFFFFu);
    }

    // ---- the "static" obj (dump group 1): alpha coefficient := 50+Random(100)
    // every 1.8 s -> alpha 0.42..0.80 (the old draw was OPAQUE — the bg
    // drowned); position (0,0), fullscreen 1024x768, no X jitter (the dump
    // sets only the alpha). Point-sampled so the grain survives the
    // 1024->1280 stretch (FNAF1's v2.57 overlay). REMOVED DEVIATION: the
    // invented "glitch" (imgs 65/73/210 every ~7 s) — those images are the
    // PUPPET (full body / head) and the title dump never fires the static
    // object's anims 12-15 (they are office cam events); that was the
    // "Puppet appearing out of nowhere" on the title.
    {
        u32 h = (u32)(int)(time / 1.8f) * 2654435761u + 7u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const u32 sAlpha = 255u - (50u + h % 100u);
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", StaticFrame(time));
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (t && t->texture && m_batch) {
            const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
            const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
            m_batch->DrawPoint(t->texture, 0.0f, 0.0f, 1280.0f, 720.0f,
                               0.0f, 0.0f, u1, v1, (sAlpha << 24) | 0x00FFFFFFu);
        }
    }

    // ---- "blip flash 2" (img_68, fullscreen band flash): group 5 rolls
    // alterable[0] := Random(3) every 6 s and groups 6/7 show it only on 1;
    // group 4 re-rolls its alpha := 200+Random(50) every 2 s (very faint,
    // 0.02..0.22). Was missing entirely.
    {
        u32 hw = (u32)(int)(time / 6.0f) * 2654435761u + 11u;
        hw ^= hw >> 13;  hw *= 3266489917u;  hw ^= hw >> 16;
        if (hw % 3u == 1u) {
            u32 h = (u32)(int)(time / 2.0f) * 2654435761u + 13u;
            h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
            const u32 bAlpha = 255u - (200u + h % 50u);
            Draw(68, 0.0f, 0.0f, 1024.0f, 768.0f, (bAlpha << 24) | 0x00FFFFFFu);
        }
    }

    // logo (obj "Active", img_469, hotspot-corrected (96,39))
    Draw(469, 96.0f, 39.0f, 300.0f, 280.0f, 0xFFFFFFFF);

    // menu rows — dump layout positions (hotspot-corrected left/top):
    // new game 301 (86,437) 222x31 / continue 303 (86,507) 222x34 /
    // 6th night 298 (90,582) 252x42 / custom 438 (89,650) 339x42.
    // Continue is ALWAYS visible (dump groups 45-48 gate only the 6th
    // night+star1 by beatgame and custom+star2 by beat6). Star 3 (246,341)
    // needs the all-20 flag — not tracked yet, skipped.
    Draw(301,  86.0f, 437.0f, 222.0f,  31.0f, 0xFFFFFFFF);   // "new game"
    Draw(303,  86.0f, 507.0f, 222.0f,  34.0f, 0xFFFFFFFF);   // "continue"
    if (game.IsBeat5()) Draw(298,  90.0f, 582.0f, 252.0f,  42.0f, 0xFFFFFFFF);   // "6th night"
    if (game.IsBeat6()) Draw(438,  89.0f, 650.0f, 339.0f,  42.0f, 0xFFFFFFFF);   // "custom night"
    if (game.IsBeat5()) Draw(593,  94.0f, 341.0f,  57.0f,  55.0f, 0xFFFFFFFF);   // star 1
    if (game.IsBeat6()) Draw(593, 171.0f, 341.0f,  57.0f,  55.0f, 0xFFFFFFFF);   // star 2
    // the selector (dump "Active 4", img 229 at (33,512) with continue at
    // 507) rides the selected row (+5 px onto each row's y)
    {
        static const f32 kRowY[4] = { 442.0f, 512.0f, 587.0f, 655.0f };
        const i32 sel = game.GetOptionSelected();
        Draw(229,  33.0f, kRowY[sel],  43.0f,  26.0f, 0xFFFFFFFF);
    }

    // "Night N" under the Continue row (dump groups 51/52: night word 270 +
    // the night number counter at (185,567) show only while Continue is
    // selected; the counter value is the Ini level capped at 5, g58).
    // v2.66 (CNTR-proven): the counter's glyph list = [230..243] (the
    // alphabet 0-9-+.e, 14x17) — the digit 'N' = 230 + N.
    if (game.GetOptionSelected() == 1) {
        Draw(270,  97.0f, 549.0f,  63.0f,  22.0f, 0xFFFFFFFF);   // "Night" art
        const i32 lv = game.GetLastNight();
        Draw(230 + (lv < 1 ? 1 : (lv > 9 ? 9 : lv)),
             185.0f, 560.0f, 14.0f, 17.0f, 0xFFFFFFFF);
    }

    // footer — v2.67: ALL THREE rows are the dump's own art. img_294
    // "v 1.033" (25,733), img_631 "Press and hold delete to reset all
    // data." (334,732), img_253 "©2014 Scott Cawthon v 1.033" (790,730 —
    // the frame_1 "Active 6" instance; the old debug-font (c) line is gone).
    // The "Demo" tag (img_487) stays hidden in the full game (DEMO? == 0).
    DrawNat(294,  25.0f, 733.0f, 0xFFFFFFFF);
    DrawNat(631, 334.0f, 732.0f, 0xFFFFFFFF);
    DrawNat(253, 790.0f, 735.0f, 0xFFFFFFFF);
}

// ---- office (frame 3 "Frame 1", 1600x768, panning window) -------------

void FNaF2Render::RenderOffice(const FNaF2Game& game, f32 time, f32 pan, i32 sceneValue) {
    if (!m_batch || !m_pak) return;

    // v2.59: the scene selector drives the office view — dark office (35)
    // keeps the panned world; any hall/lit view (36/73/93/97/84/76/50/55/56/
    // 58/99) REPLACES the room image with its own full-frame art (the mask,
    // static, sprites and HUD still draw on top).
    bool hallView = false;
    if (sceneValue != 35 && sceneValue != 0) {
        const i32 img = FNaF2Game::SceneValueImg(sceneValue);
        if (img > 0) {
            char name[32];
            Snprintf(name, sizeof(name), "img_%d", img);
            PakLoadedTexture* t = m_pak->FindTexture(name);
            if (t && t->texture) {
                const f32 w = (t->origWidth  > 0) ? (f32)t->origWidth  : 1024.0f;
                const f32 h = (t->origHeight > 0) ? (f32)t->origHeight : 768.0f;
                m_batch->Draw(t->texture, 0.0f, 0.0f, w * kScaleX, h * kScaleY, 0xFFFFFFFF);
            }
            hallView = true;
        }
    }

    // v2.71: NO hand tint on the room. The dark-office look lives in the
    // dump's own art (the replay draws the frame with its stored
    // coefficients and the walls read clearly); lit states arrive as lit
    // scene images through the hall-view branch above, and darkness ramps
    // via the blackout mask below. The old 28% multiply double-darkened
    // the already-dark art into mud (the "not the office" complaint).
    const u32 worldCol = 0xFFFFFFFF;
    if (!hallView) {

    // layer 0 — the room
    DrawWorld( 92,   0.0f,   0.0f, 1600.0f, 768.0f, pan, worldCol);  // bg
    DrawWorld(507,   0.0f,   0.0f, 1600.0f, 255.0f, pan, 0xFFFFFFFF);// "lights" strips
    DrawWorld(218, 637.0f,   0.0f,  899.0f, 389.0f, pan, worldCol);  // "Active 20" wall piece
    DrawWorld( 90, 105.0f, 361.0f,   92.0f, 141.0f, pan, 0xFFFFFFFF);// LEFT light button
    DrawWorld( 98, 1395.0f, 359.0f,  92.0f, 141.0f, pan, 0xFFFFFFFF);// RIGHT light button

    // layer 1 — the desk (plushie row, fan, mic)
    DrawWorld(293, 568.0f, 332.0f, 851.0f, 435.0f, pan, worldCol);   // table fan
    DrawWorld(601, 656.0f, 480.0f, 113.0f, 149.0f, pan, worldCol);   // freddy plush
    DrawWorld(604, 675.0f, 512.0f, 131.0f, 144.0f, pan, worldCol);   // chica
    DrawWorld(603, 754.0f, 472.0f, 116.0f, 173.0f, pan, worldCol);   // bonnie
    DrawWorld(606, 810.0f, 515.0f, 133.0f, 154.0f, pan, worldCol);   // foxy
    DrawWorld(612, 870.0f, 613.0f, 236.0f,  86.0f, pan, worldCol);   // mic
    DrawWorld(610, 991.0f, 517.0f, 122.0f, 147.0f, pan, worldCol);   // BB
    DrawWorld(608, 1038.0f, 476.0f, 117.0f, 172.0f, pan, worldCol);  // toy bonnie
    DrawWorld(555, 1122.0f, 509.0f,  84.0f, 147.0f, pan, worldCol);  // cupcake
    DrawWorld(611, 1218.0f, 522.0f, 129.0f, 156.0f, pan, worldCol);  // golden fred
    // v2.66: the monitor-flip arrow strips (dump: 119 white @ (251,725),
    // 95 pink @ (737,724) — the bottom-center raise zone)
    DrawWorld(119, 251.0f, 725.0f, 500.0f, 40.0f, pan, worldCol);
    DrawWorld( 95, 737.0f, 724.0f, 500.0f, 40.0f, pan, worldCol);
    }   // end of the panned room (skipped during hall views)

    // layer 2 — v2.59: office sprites from the encounter pipeline
    if (game.GetFreddyOfficeView())
        DrawWorld(512, 620.0f, 320.0f, 480.0f, 300.0f, pan, 0xFFFFFFFF);  // Freddy under the table (pos approx)
    if (game.IsToyBonnieScare())
        DrawWorld(187, 620.0f, 260.0f, 520.0f, 340.0f, pan, 0xFFFFFFFF);  // toy Bonnie office pose
    if (game.HasBBInOffice())
        DrawWorld(221, 700.0f, 300.0f, 380.0f, 320.0f, pan, 0xFFFFFFFF);  // BB at the desk
    // v2.66: Mangle crawls the ceiling (dump "Active 20" anims: 0=218,
    // 12=217, 13=219, 14=78 — the picks show her hull overhead); the game
    // state drives which cell; the entry point = the office frame's
    // wall-piece position
    if (game.GetMangleOfficeView() > 0) {
        static const i32 kMangleCells[3] = { 217, 219, 78 };  // hanging frames
        const i32 pick = (game.GetMangleOfficeView() - 1) % 3;
        DrawWorld(kMangleCells[pick], 460.0f, 60.0f, 480.0f, 240.0f, pan, 0xFF909090);
    }
    // danger darkening ("blackout" img_225): alpha ramps over the 300-frame window
    {
        const f32 df = game.GetDangerDark();
        if (df > 0.0f) {
            f32 a = df / 300.0f; if (a > 0.85f) a = 0.85f;
            const u32 A = (u32)(a * 255.0f);
            DrawWorld(225, 0.0f, 0.0f, 1600.0f, 768.0f, pan, (A << 24) | 0x00FFFFFFu);
        }
    }

    // layer 3 — v2.71: NO idle static over the office. The office "static"
    // object's ink coefficient is event-driven in the dump (grp 29/30 re-roll
    // alt[0]=125+Random(50)/alt[1], act #65 sets coeff=(alt0+alt1)*alt2*alt3,
    // and alt[3] stays 0 in the plain office view (grp 363/364) -> the idle
    // office has NO noise overlay. The old title-recipe alpha (90..150
    // always on) drowned the room. The glitch windows (alt[4]/alt[5] rolls,
    // grp 365-369) are not wired into the game state yet — labeled
    // deviation until the cam-glitch wave.
    (void)time;

    // v2.59: the mask overlay (dump "mask come down"/"mask"/"mask up")
    {
        const i32 ms = game.GetMaskState();
        static const int kMaskDown[9] = { 140, 139, 138, 137, 136, 135, 134, 143, 88 };
        static const int kMaskUp[11]  = { 88, 143, 134, 135, 136, 137, 138, 139, 140, 133, 133 };
        if (ms == 1) {
            int fi = (int)(game.GetMaskT() * 45.0f); if (fi > 8) fi = 8;
            Draw(kMaskDown[fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        } else if (ms == 2) {
            Draw(125, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        } else if (ms == 3) {
            int fi = (int)(game.GetMaskT() * 45.0f); if (fi > 10) fi = 10;
            Draw(kMaskUp[fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }

    // v2.67 (layout-proven): the FNAF2 office DOES carry a HUD on layer 3 —
    // the dump's own pieces (un-panned): "Night" word img_120 @(863,34),
    // the night counter @(996,58), the hour counter ("time of the night")
    // @(942,102), the "AM" art img_327 @(956,78). The counters' CNTR strips
    // = [101,124,126,131,146,147,149,150,167,169,170,171,172,173]
    // (alphabet 0-9-+.e). The monitor sits on the same layer above them, so
    // these read only in the office view.
    {
        static const int kHudStrip[14] = { 101, 124, 126, 131, 146, 147, 149,
                                           150, 167, 169, 170, 171, 172, 173 };
        DrawNat(120, 863.0f, 34.0f, 0xFFFFFFFF);                    // "Night"
        DrawCounterNat(game.GetNight(), kHudStrip, 996.0f, 58.0f, 0xFFFFFFFF);
        DrawNat(327, 956.0f, 78.0f, 0xFFFFFFFF);                    // "AM"
        DrawCounterNat(game.GetHour() == 0 ? 12 : game.GetHour(),
                       kHudStrip, 942.0f, 102.0f, 0xFFFFFFFF);
    }

    // v2.60: the office-side danger face ("danger 1", img_494/495, angry 496/497)
    if (game.IsMusicBoxDanger()) {
        const bool angry = game.GetMusicGauge() <= 200.0f;
        Draw(angry ? 497 : 495, 40.0f, 120.0f, 220.0f, 220.0f, 0xFFFFFFFF);
    }
}

// ---- camera monitor (viewing 1..12) ------------------------------------
// v2.59: the feed image comes from the game's scene selector (the dump's
// "Active 16" value = viewing + lit? + presence); value 0 ("no matching
// view") KEEPS the previous feed image, exactly like the dump's unmatched
// states. Fallback = the pinned empty feed. Wide (1600px) feeds pan with
// the office pan, 1024-wide ones pin.

void FNaF2Render::RenderMonitor(const FNaF2Game& game, f32 time, f32 sinceSwitch, f32 pan,
                                i32 sceneValue, i32 lastSceneValue) {
    if (!m_batch || !m_pak) return;
    static const int kFeedImg[13] = {
        0, 174, 80, 83, 43, 38, 32, 51, 37, 117, 41, 76, 50
    };
    const i32 v = game.GetViewing();
    if (v < 1 || v > 12) return;

    i32 value = (sceneValue != 0) ? sceneValue : lastSceneValue;
    i32 img = (value > 0) ? FNaF2Game::SceneValueImg(value) : 0;
    if (img <= 0) img = kFeedImg[v];   // fallback: the pinned empty feed
    if (img > 0) {
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", img);
        PakLoadedTexture* t = m_pak->FindTexture(name);
        if (t && t->texture) {
            if (t->origWidth > 1024)
                DrawWorld(img, 0.0f, 0.0f, (f32)t->origWidth, 768.0f, pan, 0xFFFFFFFF);
            else
                Draw(img, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }

    // feed-switch interference burst
    if (sinceSwitch < 0.12f)
        Draw(StaticFrame(time), 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);

    // ---- the MAP (layer 3, screen-fixed): panel img_2 (416x312) at its
    // instance (727,499) + 12 buttons img_16/img_17 (60x40) at their
    // instance positions; the selected cam shows the pressed look.
    // v2.67: the per-cam name plates are the dump's OWN images 19..30
    // ("Active 3".."Active 14" objects on the same layer, each riding its
    // button) — the debug-font numbers are gone.
    Draw(2, 727.0f, 499.0f, 416.0f, 312.0f, 0xFFFFFFFF);
    {
        static const struct { int cam; float x, y; } kButtons[12] = {
            { 1,  602.0f, 557.0f }, { 2,  734.0f, 558.0f },
            { 3,  602.0f, 491.0f }, { 4,  737.0f, 491.0f },
            { 5,  608.0f, 651.0f }, { 6,  723.0f, 651.0f },
            { 7,  758.0f, 431.0f }, { 8,  603.0f, 420.0f },
            { 9,  915.0f, 390.0f }, { 10, 847.0f, 509.0f },
            { 11, 950.0f, 465.0f }, { 12, 933.0f, 558.0f },
        };
        for (int i = 0; i < 12; ++i) {
            Draw(kButtons[i].cam == v ? 17 : 16,
                 kButtons[i].x, kButtons[i].y, 60.0f, 40.0f, 0xFFFFFFFF);
        }
        // the label plates at their dump instance points (camera-matched):
        static const struct { int img; float x, y; } kLabel[12] = {
            { 19, 579.0f, 544.0f }, { 20, 711.0f, 545.0f },
            { 21, 579.0f, 478.0f }, { 22, 714.0f, 478.0f },
            { 23, 585.0f, 638.0f }, { 24, 700.0f, 638.0f },
            { 25, 735.0f, 416.0f }, { 26, 580.0f, 407.0f },
            { 27, 892.0f, 376.0f }, { 28, 824.0f, 495.0f },
            { 29, 927.0f, 452.0f }, { 30, 910.0f, 546.0f },
        };
        for (int i = 0; i < 12; ++i)
            DrawNat(kLabel[i].img, kLabel[i].x, kLabel[i].y, 0xFFFFFFFF);
    }

    // CAM 11 (Prize Corner): the MUSIC BOX at its dump spots — the button
    // img_251 (357,566), the "Wind Up Music Box" plate img_472 (364,577)
    // and the "click & hold" plate img_486 (357,635) — all SHOWN only while
    // viewing cam 11 (dump groups 488/489 hide them otherwise). The gauge
    // is the dump's icon counter ("music box counter", 22 cells at
    // (292,590): the strip runs from the full-moon to the empty shell —
    // value scale 0..2000 -> cell 0..20). The danger faces ("danger 1"/
    // angry pair) at their dump point (884,449) once the Puppet is loose.
    if (v == 11) {
        Draw(251, 357.0f, 566.0f, 156.0f, 65.0f, 0xFFFFFFFF);
        DrawNat(472, 364.0f, 577.0f, 0xFFFFFFFF);
        DrawNat(486, 357.0f, 635.0f, 0xFFFFFFFF);
        static const int kGauge[22] = {
            502, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534,
            535, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545
        };
        int cell = (int)(game.GetMusicGauge() / 2000.0f * 21.0f);
        if (cell < 0) cell = 0; if (cell > 21) cell = 21;
        DrawNat(kGauge[cell], 292.0f, 590.0f, 0xFFFFFFFF);
        if (game.IsMusicBoxDanger()) {
            const bool angry = game.GetMusicGauge() <= 200.0f;
            DrawNat(angry ? 490 : 308, 884.0f, 449.0f, 0xFFFFFFFF);
        }
    }

    // v2.59: movement static burst — a character moved in the watched feed
    if (game.GetMoveStatic() > 0.0f)
        Draw(20, 0.0f, 0.0f, 1024.0f, 768.0f,
             (u32)((int)(game.GetMoveStatic() * 255.0f) << 24) | 0x00FFFFFFu);

    // v2.66: the FNAF2 monitor shows no text in the original — no cam
    // label, no clock, no battery (the map's own buttons carry the state;
    // the per-cam labels are dumped images to wire in the label pass).
}
// v2.59: the jumpscare — "attack animation" values 12..21 (anim per attacker,
// dump g440-449) drawn full-screen over whatever is underneath. Frame lists
// verbatim from the FNAF2 application.json (h=156).
void FNaF2Render::DrawAttack(const FNaF2Game& game) {
    if (!m_batch) return;
    static const int kFrames[10][16] = {
        { 377,365,379,367,368,369,370,371,372,373,374,375,376,0,0,0 },
        { 744,729,746,731,732,733,734,735,736,737,738,739,740,741,742,743 },
        { 458,447,460,449,450,451,452,453,454,455,456,457,0,0,0,0 },
        { 401,385,386,388,389,390,391,392,393,394,395,396,397,398,0,0 },
        { 712,700,714,702,703,704,705,706,707,708,709,710,711,0,0,0 },
        { 757,745,759,747,748,749,750,751,752,753,754,755,756,0,0,0 },
        { 446,414,448,415,416,419,420,421,422,423,424,445,0,0,0,0 },
        { 812,387,814,404,785,792,802,803,804,805,806,807,808,809,810,811 },
        { 515,195,517,313,315,316,317,318,324,340,342,343,345,347,514,0 },
        { 320,246,516,247,248,249,250,306,310,311,312,314,319,0,0,0 },
    };
    static const i32 kCount[10] = { 13, 16, 12, 14, 13, 13, 12, 16, 15, 13 };
    static const i32 kSpd[10]   = { 50, 50, 40, 60, 50, 40, 40, 50, 50, 50 };
    const i32 anim = game.GetScareAnim();
    if (anim < 12 || anim > 21) return;
    const i32 idx = anim - 12;
    const f32 fps = (f32)kSpd[idx] * 0.6f;
    int fi = (int)(game.GetScareTimer() * fps);
    if (fi < 0) fi = 0;
    if (fi >= kCount[idx]) fi = kCount[idx] - 1;
    Draw(kFrames[idx][fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

// ===========================================================================
// v2.62 — the rest of the dump's frame flow + the 8-bit minigames.
// Coordinates from the frame layouts; the anim-cell art that the dumper
// could not split into separate images (night names, the 5->6 roll, walk
// cycles) stands in as labeled debug-font/approximation stops.
// ===========================================================================

void FNaF2Render::RenderAd() {
    // frame 8: the HELP WANTED newspaper, fullscreen
    Draw(272, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderCard(const FNaF2Game& game) {
    // frame 2: the "12:00 AM / Nst Night" card (429 @ 512,374) — the
    // per-night cells ARE in application.json (objInfo 45 anims 0/12-17):
    // night 1->429, 2->430, 3->431, 4->436, 5->437, 6->426, 7->425.
    // v2.66: the dump position verbatim (hot 117,58 → topleft 395,316,
    // native 235x116) and the stray debug line is gone.
    static const i32 kNightCell[7] = { 429, 430, 431, 436, 437, 426, 425 };
    const i32 n = game.GetNight();
    const i32 cell = kNightCell[(n < 1) ? 0 : (n > 7 ? 6 : n - 1)];
    Draw(cell, 395.0f, 316.0f, 235.0f, 116.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderStatic() {
    // frame 4: the post-night static (img 361 fullscreen)
    Draw(361, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderNextDay(const FNaF2Game& game) {
    // frame 5: the "5 -> 6" roll IS in application.json (objInfo 224 anim 0,
    // 18 cells @ speed 10 = 6 fps = 3 s), ending on the "6"; the AM plate
    // (295 @ 595,363) sits beside it. The crowd cheer rides the anim end.
    static const i32 kRoll[18] = { 297, 297, 297, 290, 300, 302, 304, 309, 309,
                                   309, 309, 309, 433, 336, 335, 427, 364, 432 };
    Draw(295, 595.0f, 363.0f, 179.0f, 84.0f, 0xFFFFFFFF);
    int fi = (int)(game.GetCardTimer() * 6.0f);          // speed 10 -> 6 fps
    if (fi < 0) fi = 0;
    if (fi > 17) fi = 17;                                // hold the last cell
    Draw(kRoll[fi], 415.0f, 363.0f, 57.0f, 86.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderDream(const FNaF2Game& game) {
    // frame 13: the 2500x768 panning room (616), the slumped pair
    // (609 Bonnie @243 / 614 Chica @2348), the night props, the static
    // flicker and the slow blackout. v2.66: the dump's OWN perspective
    // wraps this layer (frame 13 "Perspective" obj 300 @ (0,0): object
    // 1024x768, EDATA zoom=200 curve=4) — the dream's mild bend.
    m_batch->BeginSceneCapture(0xFF000000);
    const f32 pan = game.GetDreamPan();
    DrawWorld(616, 0.0f, 0.0f, 2500.0f, 768.0f, pan, 0xFFFFFFFF);
    DrawWorld(609, 243.0f, 300.0f, 400.0f, 460.0f, pan, 0xFFFFFFFF);   // Bonnie
    DrawWorld(614, 2348.0f, 300.0f, 400.0f, 460.0f, pan, 0xFFFFFFFF);  // Chica + balloons
    if (game.GetNight() == 4)
        DrawWorld(624, 1514.0f, 340.0f, 220.0f, 300.0f, pan, 0xFFFFFFFF);  // golden Freddy
    if (game.GetNight() == 5) {
        // the Puppet slides toward the anchor (dump groups 26/27)
        const f32 px = 2170.0f - pan;
        if (px > -300.0f && px < 1330.0f)
            DrawWorld(626, 2170.0f, 330.0f, 260.0f, 420.0f, pan, 0xFFFFFFFF);
    }
    m_batch->EndSceneCapture();
    {   // vc100 (XDK) has no braced-init args: explicit local
        SpriteBatch::PerspGeom g;
        g.objW = 1024.0f; g.objH = 768.0f; g.orgX = 0.0f; g.orgY = 0.0f;
        g.winW = 1280.0f; g.winH = 720.0f;
        m_batch->SetPerspectiveGeometry(g);
    }
    m_batch->DrawPerspective(200.0f, 384.0f, 4.0f);
    // the static flicker (alpha tiers per the re-roll)
    {
        const i32 roll = game.GetRareRoll();
        const u32 a = (u32)(roll <= 6 ? 250 : (roll <= 9 ? 225 : 100));
        Draw(361, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }
    // the blackout fade
    if (game.GetBlackout() > 0.5f) {
        const u32 a = (u32)game.GetBlackout();
        Draw(225, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }
}

void FNaF2Render::RenderError(bool second) {
    Draw(second ? 630 : 628, second ? 33.0f : 20.0f, second ? 19.0f : 16.0f,
         512.0f, 384.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderEnd(i32 which) {
    const i32 img = (which == 5) ? 587 : (which == 6) ? 590 : 622;
    Draw(img, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderCustomize(const FNaF2Game& game) {
    // v2.67: frame 12 "customize" rebuilt 1:1 from the dump layout — the
    // 2x5 icon grid, per-column arrows, the TEN AI counters (CNTR strip
    // [547,548,549,550,552,553,554,580..586], alphabet 0-9-+.e native cells)
    // at their instance points, the mode row ("mode" is the frame's Text
    // object — the preset names come from the dump's own set-text actions),
    // READY and the beaten-challenge star (img 593; "did challenge" gets
    // alt[0] := Ini["c"+mode] in group 46 — shown here when the flag is 1).
    // The debug-font number/mode/ALL-20/1987 texts are gone ("1987" and
    // "all are 20" are parked counters in the dump — never drawn).
    static const i32 kIcons[10]  = { 551, 339, 338, 341, 344, 352, 346, 576, 577, 578 };
    static const f32 kIconX[10]  = { 68, 252, 436, 622, 814,  64, 250, 432, 616, 806 };
    static const f32 kIconY[10]  = { 78,  78,  78,  80,  80, 342, 340, 342, 340, 340 };
    static const f32 kNumX[10]   = { 153, 337, 520, 714, 904, 154, 337, 522, 708, 900 };
    static const f32 kNumY[10]   = { 315, 315, 316, 315, 316, 603, 602, 603, 600, 601 };
    static const f32 kArrowL[10][2] = {
        { 57, 262 }, { 242, 262 }, { 427, 263 }, { 620, 262 }, { 812, 263 },
        { 62, 552 }, { 246, 552 }, { 428, 552 }, { 614, 552 }, { 806, 552 }
    };
    static const f32 kArrowR[10][2] = {
        { 164, 262 }, { 347, 262 }, { 531, 263 }, { 723, 262 }, { 915, 263 },
        { 165, 552 }, { 349, 552 }, { 531, 552 }, { 717, 552 }, { 909, 552 }
    };
    static const int kAIStrip[14] = { 547, 548, 549, 550, 552, 553, 554,
                                      580, 581, 582, 583, 584, 585, 586 };
    Draw(524, 68.0f, 18.0f, 384.0f, 44.0f, 0xFFFFFFFF);    // "Customize Night"
    Draw(558, 97.0f, 631.0f, 822.0f, 29.0f, 0xFFFFFFFF);   // difficulty legend
    const i32 sel = game.GetOptionSelected();
    for (i32 i = 0; i < 10; ++i) {
        const u32 tint = (sel == i) ? 0xFF50A0FF : 0xFFFFFFFF;
        DrawNat(kIcons[i], kIconX[i], kIconY[i], tint);
        DrawNat(557, kArrowL[i][0], kArrowL[i][1], 0xFFFFFFFF);
        DrawNat(556, kArrowR[i][0], kArrowR[i][1], 0xFFFFFFFF);
        DrawCounterNat(game.GetCustomAI(i), kAIStrip, kNumX[i], kNumY[i], tint);
    }
    // v2.68: the ten NAME plates (backdrop strips 575/348/349/350/351 +
    // 353/354/355/356/573). They are Backdrop-type objects — the dump's
    // left/top for the bottom row is junk (hotX ~ -519 parks them past the
    // frame edge; the frame replay visibly misplaces them); Backdrop cells
    // sit at plain x,y and those match the icon columns. Backdrops take no
    // ink effects in the runtime — always full white, no selection tint.
    static const struct { int img; f32 x, y; } kNames[10] = {
        { 575,  76, 225 }, { 348, 262, 227 }, { 349, 450, 227 },
        { 350, 648, 228 }, { 351, 864, 231 },
        { 353,  76, 479 }, { 354, 260, 485 }, { 355, 452, 487 },
        { 356, 630, 489 }, { 573, 818, 483 },
    };
    for (i32 i = 0; i < 10; ++i)
        DrawNat(kNames[i].img, kNames[i].x, kNames[i].y, 0xFFFFFFFF);
    // the challenge mode row: arrows (dump 595/602) + the Text object
    // "mode" at (69,676) — the preset strings are the dump's act-#88 text.
    DrawNat(602, 159.0f, 700.0f, 0xFFFFFFFF);
    DrawNat(595, 696.0f, 700.0f, 0xFFFFFFFF);
    {
        static const char* const kModes[10] = {
            "20/20/20/20", "New and Shiny", "Double Trouble", "Night of Misfits",
            "Foxy Foxy", "Ladies Night", "Freddy's Circus", "Cupcake Challenge",
            "Fazbear Fever", "Golden Freddy"
        };
        const i32 mode = game.GetCustomMode();
        if (mode >= 1 && mode <= 10 && m_text)
            m_text->DrawText((int)(69.0f * kScaleX), (int)(676.0f * kScaleY),
                             kModes[mode - 1], 0xFFB0B0B0);
        if (mode >= 1 && mode <= 10 && game.GetChallengeBeat(mode - 1)) {
            // dump: "did challenge" + "did challenge 2" toggle TOGETHER —
            // a beaten preset wears a star on BOTH sides of the mode row
            DrawNat(593, 90.0f, 697.0f, 0xFFFFFFFF);
            DrawNat(593, 741.0f, 697.0f, 0xFFFFFFFF);
        }
    }
    Draw(546, 823.0f, 696.0f, 186.0f, 58.0f, 0xFFFFFFFF);  // READY
}

void FNaF2Render::RenderRare(i32 which) {
    const i32 img = (which == 1) ? 625 : (which == 2) ? 623 : 627;
    Draw(img, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderGameOver() {
    // frame 6: the withered Freddy face + the "Game Over" plate
    Draw(226, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(228, 180.0f, 700.0f, 320.0f, 48.0f, 0xFFFFFFFF);
}

// ---- the 8-bit minigames ------------------------------------------------

void FNaF2Render::RenderEightBit(const FNaF2Game& game) {
    // frame 19 SAVETHEM: the fixed-screen room re-dressed per grid cell
    const FNaF2MgState& mg = game.Mg();
    Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    // the border walls (dump: 639/640 strips on all four edges)
    Draw(639,  -2.0f,  -8.0f, 1024.0f, 84.0f, 0xFFFFFFFF);
    Draw(639,  -2.0f, 680.0f, 1024.0f, 84.0f, 0xFFFFFFFF);
    Draw(640,  -2.0f,  80.0f, 30.0f, 570.0f, 0xFFFFFFFF);
    Draw(640, 926.0f,  79.0f, 30.0f, 570.0f, 0xFFFFFFFF);
    // the per-room dressing (the dump's fire-once groups, keyed (h,v))
    const i32 h = mg.h, v = mg.v;
    if ((h == 3 && v == 1) || (h == 4 && v == 1) || (h == 4 && v == 3) ||
        (h == 3 && v == 3) || (h == 2 && v == 4)) {
        Draw(652, 294.0f, 190.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(652, 737.0f, 190.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(652, 737.0f, 410.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(652, 294.0f, 410.0f, 90.0f, 60.0f, 0xFFFFFFFF);
    }
    if (h == 5 && v == 2) Draw(655, 720.0f, 420.0f, 60.0f, 70.0f, 0xFFFFFFFF);
    if (h == 1 && v == 4) Draw(660, 523.0f, 296.0f, 190.0f, 100.0f, 0xFFFFFFFF);
    if (h == 2 && v == 1) {
        Draw(656, 184.0f, 524.0f, 70.0f, 70.0f, 0xFFFFFFFF);   // dead chica
        Draw(657, 280.0f, 288.0f, 70.0f, 70.0f, 0xFFFFFFFF);   // dead bonnie
        Draw(658, 832.0f, 190.0f, 70.0f, 70.0f, 0xFFFFFFFF);
    }
    if ((h == 2 && v == 3) || (h == 4 && v == 2) || (h == 2 && v == 4))
        Draw(659, 500.0f, 600.0f, 50.0f, 50.0f, 0xFFFFFFFF);   // trash
    if ((h == 3 && v == 2) || (h == 4 && v == 2) || (h == 2 && v == 4) || (h == 3 && v == 4))
        Draw(662, 700.0f, 250.0f, 60.0f, 60.0f, 0xFFFFFFFF);   // checker
    if (h == 3 && v == 5) Draw(664, 704.0f, 400.0f, 44.0f, 50.0f, 0xFFFFFFFF);  // gift
    if (h == 4 && v == 2) Draw(665, (f32)mg.heX, (f32)mg.heY, 60.0f, 80.0f, 0xFFFFFFFF);
    // the blood spots (group 40 shows them every room after the first change)
    Draw(380, 316.0f, 308.0f, 30.0f, 30.0f, 0xFFFFFFFF);
    Draw(380, 711.0f, 468.0f, 30.0f, 30.0f, 0xFFFFFFFF);
    // the NPCs
    if (mg.youCantOn) Draw(673, (f32)mg.youCantX, (f32)mg.youCantY, 40.0f, 56.0f, 0xFFFFFFFF);
    if (mg.manOn)     Draw(671, (f32)mg.manX, (f32)mg.manY, 46.0f, 70.0f, 0xFFFFFFFF);
    if (mg.gfOn)      Draw(669, (f32)mg.gfX, (f32)mg.gfY, 70.0f, 90.0f, 0xFFFFFFFF);
    if (mg.chaserOn)  Draw(675, (f32)mg.chX, (f32)mg.chY, 56.0f, 80.0f, 0xFFFFFFFF);
    // the player (8-bit Freddy 245 glued to the hit box)
    Draw(245, (f32)mg.px - 50.0f, (f32)mg.py - 55.0f, 99.0f, 100.0f, 0xFFFFFFFF);
    // the WASD hint rides above the player for the first 5 s (group 104)
    if (mg.t < 5.0f) Draw(646, (f32)mg.px - 40.0f, (f32)mg.py - 130.0f, 84.0f, 30.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderMgLoad() {
    // frame 21: black + the two loading bars
    Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(678, 0.0f, 300.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
    Draw(680, 0.0f, 420.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
}

void FNaF2Render::RenderMinigame(const FNaF2Game& game) {
    const FNaF2MgState& mg = game.Mg();
    const bool attack = (mg.attackT >= 0.0f);
    // the walk cycles (application.json, the player objects' anim 0):
    // mg1 [412,417]@speed5, mg2 [760], mg3 [722,726] — 3 fps waddles
    int walkFrame = 0;
    if (mg.game == 2)      walkFrame = ((int)(mg.t * 3.0f)) % 2;   // 412/417
    else if (mg.game == 4) walkFrame = ((int)(mg.t * 3.0f)) % 2;   // 722/726
    if (mg.game == 2) {
        // frame 23 TAKE CAKE
        Draw(400, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(698, 77.0f, 659.0f, 870.0f, 60.0f, 0xFFFFFFFF);
        // the kids (application.json objInfo 393): 0=[692,685]@3fps content,
        // 12=[519,482]@6fps crying, 13=[693,696]@18fps past-20 — the real
        // sadness cells, picked per the kid's counter
        static const f32 kKidX[6] = { 290,290,290,776,770,766 };
        static const f32 kKidY[6] = { 280,416,548,278,410,538 };
        for (i32 i = 0; i < 6; ++i) {
            i32 img = 692; f32 fps = 3.0f;
            if (mg.kidSad[i] >= 20)      { img = ((int)(mg.t * 18.0f) % 2) ? 696 : 693; fps = 18.0f; }
            else if (mg.kidSad[i] >= 10) { img = ((int)(mg.t * 6.0f)  % 2) ? 482 : 519; }
            else                         { img = ((int)(mg.t * 3.0f)  % 2) ? 685 : 692; }
            Draw(img, kKidX[i], kKidY[i], 56.0f, 64.0f, 0xFFFFFFFF);
        }
        // the crying kid 2 (objInfo 394): idle [683,697]@1.5fps; the murder
        // reaction anim 14 = [683,713,716,717,718,701,720,719] @ speed 1
        if (mg.carStage >= 2) {
            static const i32 kReact[8] = { 683, 713, 716, 717, 718, 701, 720, 719 };
            int fi = (int)(mg.manStageT * 0.6f);
            if (fi < 0) fi = 0;
            if (fi > 7) fi = 7;
            Draw(kReact[fi], 404.0f, 118.0f, 56.0f, 64.0f, 0xFFFFFFFF);
        } else {
            Draw(((int)(mg.t * 1.5f) % 2) ? 697 : 683, 404.0f, 118.0f, 56.0f, 64.0f, 0xFFFFFFFF);
        }
        if (mg.murder && mg.carStage >= 1)
            Draw(715, (f32)mg.carX, 67.0f, 150.0f, 70.0f, 0xFFFFFFFF);
        if (mg.carStage == 2) Draw(699, 467.0f, 300.0f, 46.0f, 72.0f, 0xFFFFFFFF);
        Draw(walkFrame ? 417 : 412, (f32)mg.px - 45.0f, (f32)mg.py - 55.0f,
             90.0f, 100.0f, 0xFFFFFFFF);
    } else if (mg.game == 3) {
        // frame 24 GIVE GIFTS, GIVE LIFE
        Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(723, 391.0f, 8.0f, 250.0f, 50.0f, 0xFFFFFFFF);
        static const f32 kHeadX[4] = { 235, 200, 814, 818 };
        static const f32 kHeadY[4] = { 204, 594, 190, 580 };
        static const i32 kHeadImg[4] = { 728, 763, 762, 765 };
        for (i32 i = 0; i < 4; ++i) {
            if (!mg.phaseB || !mg.headGifted[i] || mg.kidSad[i] == 0)
                Draw(kHeadImg[i], kHeadX[i], kHeadY[i], 52.0f, 52.0f, 0xFFFFFFFF);
        }
        for (i32 i = 0; i < 4; ++i) {
            static const f32 kHelpX[4] = { 228, 204, 830, 830 };
            static const f32 kHelpY[4] = { 239, 635, 239, 623 };
            Draw(761, kHelpX[i], kHelpY[i], 46.0f, 60.0f, 0xFFFFFFFF);
        }
        if (!mg.phaseB) Draw(779, 526.0f, 388.0f, 79.0f, 57.0f, 0xFFFFFFFF);
        Draw(760, (f32)mg.px - 45.0f, (f32)mg.py - 60.0f, 90.0f, 110.0f, 0xFFFFFFFF);
    } else {
        // frame 25 FOXY'S PARTY: the 2048-wide world, the camera snaps at
        // the x=1024 line (dump groups 44/45)
        const f32 cam = (mg.px >= 1024) ? -1024.0f : 0.0f;
        Draw(778, cam + 0.0f,    0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(382, cam + 1024.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        Draw(784, cam + 273.0f, 268.0f, 120.0f, 140.0f, 0xFFFFFFFF);
        Draw(784, cam + 784.0f, 268.0f, 120.0f, 140.0f, 0xFFFFFFFF);
        static const f32 kKidsX[5] = { 1608, 1778, 1550, 1766, 1602 };
        static const f32 kKidsY[5] = { 217, 322, 404, 508, 594 };
        for (i32 i = 0; i < 5; ++i) {
            // the kids go silent on the second visit (group 57)
            const u32 tint = (mg.cycles >= 2) ? 0xFF808080 : 0xFFFFFFFF;
            Draw(794, cam + kKidsX[i], (f32)kKidsY[i], 56.0f, 64.0f, tint);
        }
        if (mg.cycles >= 2) Draw(799, cam + 1560.0f, 430.0f, 46.0f, 72.0f, 0xFFFFFFFF);
        if (mg.phase == 1 && mg.cycles < 2)
            Draw(787, cam + 819.0f, 566.0f, 90.0f, 60.0f, 0xFFFFFFFF);
        Draw(walkFrame ? 726 : 722, (f32)mg.px + cam - 45.0f, (f32)mg.py - 55.0f,
             90.0f, 100.0f, 0xFFFFFFFF);
    }
    // the scripted attack — the REAL frame lists from the "attack animation"
    // object (application.json): 20 = the car/man scare, 21 = the Puppet,
    // 15 = Foxy; fullscreen cells at speed*0.6 fps, then the load jump
    if (attack) {
        static const i32 kFrames[3][16] = {
            { 401, 385, 386, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 0, 0 },  // 15
            { 515, 195, 517, 313, 315, 316, 317, 318, 324, 340, 342, 343, 345, 347, 514, 0 },// 20
            { 320, 246, 516, 247, 248, 249, 250, 306, 310, 311, 312, 314, 319, 0, 0, 0 }     // 21
        };
        static const i32 kCount[3] = { 14, 15, 13 };
        static const i32 kSpd[3]   = { 60, 50, 40 };
        int idx = -1;
        if      (mg.attackAnim == 15) idx = 0;
        else if (mg.attackAnim == 20) idx = 1;
        else if (mg.attackAnim == 21) idx = 2;
        if (idx >= 0) {
            const f32 fps = (f32)kSpd[idx] * 0.6f;
            int fi = (int)(mg.attackT * fps);
            if (fi < 0) fi = 0;
            if (fi >= kCount[idx]) fi = kCount[idx] - 1;
            Draw(kFrames[idx][fi], 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        } else {
            Draw(361, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
        }
    }
    // the CRT scanline flicker (the dump's scanline groups: every 500 ms the
    // alpha coefficient := 200+Random(100) -> alpha 0.02..0.22)
    {
        u32 h = (u32)(int)(mg.t / 0.5f) * 2654435761u + 21u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const u32 coeff = 200u + h % 100u;
        const u32 a = 255u - coeff;
        if (a > 0u)
            Draw(649, 0.0f, 0.0f, 1024.0f, 768.0f, (a << 24) | 0x00FFFFFFu);
    }
}

void FNaF2Render::RenderEndBars() {
    // frames 20/22: black + the two CRT bars
    Draw(636, 0.0f, 0.0f, 1024.0f, 768.0f, 0xFFFFFFFF);
    Draw(678, 0.0f, 300.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
    Draw(680, 0.0f, 420.0f, 1076.0f, 40.0f, 0xFFFFFFFF);
}

} // namespace fnaf
