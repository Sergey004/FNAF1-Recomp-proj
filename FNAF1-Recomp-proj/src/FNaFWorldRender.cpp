/**
 * FNaFWorldRender.cpp: v2.90 — FNAF World module renderer (see the header).
 *
 * Dump-scaled drawing: frame space 800x480 -> screen 1280x720 (x*1.6, y*1.5).
 * Sprite handles are the RecompPack indices (Dumps/FNaF World/Images/N.png):
 *   4573 title bg   4597 "FNAF WORLD" logo   4574 party cast
 *   4569 START      4830 "Demo"              1207 sparkle
 *   2119 splash art (frame 0)
 */

#include "FNaFWorldRender.h"
#include "FNaFWorldGame.h"
#include "FNaFWorldMap.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <stdlib.h>

namespace fnaf {

// dump-space -> screen-space
static const f32 kSx = 1280.0f / 800.0f;   // 1.6
static const f32 kSy =  720.0f / 480.0f;   // 1.5

void FNaFWorldRender::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

// SLRender's D() verbatim: index -> "img_N", UV clamp to the original size
// (pak textures are 32-aligned; padding rows decode to junk), quad in px.
void FNaFWorldRender::D(int handle, f32 x, f32 y, f32 w, f32 h, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x, y, w, h, 0.0f, 0.0f, u1, v1, color);
}

// native-size draw (layout left/top = origin minus the hot spot)
void FNaFWorldRender::DN(int handle, f32 x, f32 y, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, x, y, (f32)t->origWidth, (f32)t->origHeight,
                  0.0f, 0.0f, u1, v1, color);
}

// dump-space scaled draw: native dims x*1.6 / y*1.5 at a dump-space position.
// Every title-screen element moves/scales in the 800x480 frame space, so all
// of its art goes through this (the splash too).
void FNaFWorldRender::DNS(int handle, f32 dx, f32 dy, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->Draw(t->texture, dx * kSx, dy * kSy,
                  (f32)t->origWidth * kSx, (f32)t->origHeight * kSy,
                  0.0f, 0.0f, u1, v1, color);
}

// point-sampled D: same UV clamp, but D3DTEXF_POINT (SpriteBatch::DrawPoint).
// Tiny bright details (the title-sky stars) stay crisp dots instead of DXT
// 4x4 blocks smeared into squares by the linear upscale.
void FNaFWorldRender::DP(int handle, f32 x, f32 y, f32 w, f32 h, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    m_batch->DrawPoint(t->texture, x, y, w, h, 0.0f, 0.0f, u1, v1, color);
}

// ------------------------------------------------------------
// g7-11 sparkle system (frame 2): 8 "sparkle 2" (img 1207) spawn every
// 100 ms inside the invisible "invid" zone (4568 @ 255,292 size 135x90
// dump-space), wander briefly and fade — purely cosmetic, renderer-local.
// ------------------------------------------------------------
struct FwSparkle { f32 x, y, born; bool alive; };
static FwSparkle s_sparks[32] = { 0 };
static f32 s_lastSpawnT = -1.0f;

void FNaFWorldRender::TickSparkles(f32 time) {
    // g7: the 100 ms timer, 8 per burst (ring buffer; oldest slots reused)
    if (s_lastSpawnT < 0.0f || time - s_lastSpawnT >= 0.1f) {
        s_lastSpawnT = time;
        int spawned = 0;
        for (int i = 0; i < 32 && spawned < 8; ++i) {
            if (s_sparks[i].alive && time - s_sparks[i].born < 0.6f) continue;
            s_sparks[i].x = 255.0f + (f32)(rand() % 136);   // invid 255..390
            s_sparks[i].y = 292.0f + (f32)(rand() % 91);    // invid 292..382
            s_sparks[i].born = time;
            s_sparks[i].alive = true;
            ++spawned;
        }
    }
}

void FNaFWorldRender::RenderSparkles(f32 time) {
    if (!m_pak || !m_batch) return;
    PakLoadedTexture* t = m_pak->FindTexture("img_1207");
    if (!t || !t->texture) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    const f32 w = (f32)t->origWidth * kSx;
    const f32 h = (f32)t->origHeight * kSy;

    for (int i = 0; i < 32; ++i) {
        if (!s_sparks[i].alive) continue;
        const f32 age = time - s_sparks[i].born;
        if (age < 0.0f || age >= 0.6f) { s_sparks[i].alive = false; continue; }
        // g9/g10: the sparkle alpha ramps in then out over its ~0.6 s life
        const f32 a = (age < 0.3f) ? (age / 0.3f) : (1.0f - (age - 0.3f) / 0.3f);
        const u32 alpha = (u32)(a * 255.0f) << 24;
        m_batch->Draw(t->texture,
                      s_sparks[i].x * kSx - w * 0.5f,
                      s_sparks[i].y * kSy - h * 0.5f,
                      w, h, 0.0f, 0.0f, u1, v1, alpha | 0x00FFFFFF);
    }
}

// ------------------------------------------------------------
// frame 0 "Frame 32": the splash art at its layout point (234,165),
// full-frame black behind it. Fade rides on top (module DrawFade).
// ------------------------------------------------------------
void FNaFWorldRender::RenderSplash() {
    if (m_batch) m_batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f, 0xFF000000);
    DNS(2119, 234.0f, 165.0f, 0xFFFFFFFF);
}

// ------------------------------------------------------------
// frame 2 "title screen" (800x480 -> 1280x720):
//   layer 0: bg 4573 @(0,0) full frame
//   layer 1: logo 4597 sliding from left (top 10), party 4574 from bottom
//   layer 2: START 4569 from right (top 409), Demo 4830 @(18,18), texts
//   sparkles 1207 (step 2: the g7-11 wander; here the logo sits alone)
// ------------------------------------------------------------
void FNaFWorldRender::RenderTitle(const FNaFWorldGame& game, f32 time) {
    if (!m_batch) return;

    // layer 0: the background, full frame — POINT-sampled: the tiny sky
    // stars survive as dots (linear turns their DXT blocks into squares)
    DP(4573, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);

    // layer 1: the logo (g4) and the party cast (g3) mid-slide
    DNS(4597, game.GetLogoLeft(),  game.GetLogoTop(),  0xFFFFFFFF);
    DNS(4574, game.GetPartyLeft(), game.GetPartyTop(), 0xFFFFFFFF);

    // g7-11: the sparkles twinkling around the cast
    TickSparkles(time);
    RenderSparkles(time);

    // layer 2: START (g6). The "Demo" plate (4830) is NOT drawn: dump g14
    // shows it only when the "demo?" counter == 1 — the free Game Jolt demo.
    // This exe is the full v1.24 Update 2, the label stays hidden.
    DNS(4569, game.GetStartLeft(), game.GetStartTop(), 0xFFFFFFFF);

    // the frame's String texts (bottom corners, per the rendered layout)
    if (m_text) {
        m_text->DrawText((i32)(12 * kSx), (i32)(438 * kSy), "v 1.24", 0xFFFFFFFF);
        m_text->DrawText((i32)(12 * kSx), (i32)(458 * kSy), "Update 2", 0xFFFFFFFF);
        m_text->DrawText((i32)(600 * kSx), (i32)(458 * kSy),
                         "Copyright (c) 2016 Scott Cawthon", 0xFFFFFFFF);
    }
}

// ------------------------------------------------------------
// frame 5 "overworld" — v2.91 step 2: the exe 6000x4000 map.
//   * camera window = the dump frame 800x480 px, centred on Freddy,
//     clamped to the map (World.c GetCameraView); everything scales
//     x*1.6 / y*1.5 like every other FW screen
//   * the 2207 generated instances draw in painter's order (layer, top):
//     pass 1 layer 0 (ground/decor), pass 2 chests + Freddy, pass 3
//     layer 1 (trees/rocks) so foreground scenery covers the actors
//   * PakLoader::FindTexture is a linear scan; scenery repeats the same
//     img in runs (83 tree rocks in a row), so a 1-entry cache absorbs
//     most lookups (visible instances are also culled before lookup)
// ------------------------------------------------------------
void FNaFWorldRender::RenderWorld(const FNaFWorldGame& game) {
    if (!m_batch) return;
    static int              s_lastImg = -1;
    static PakLoadedTexture* s_lastTex = 0;

    // the zone ground: the exe attaches a full-screen backdrop sprite to
    // the camera and swaps it per zone (the "background N" family, all
    // instances at (0,0) in the layout — camera objects, not map scenery).
    // The 850x530 zone grounds: Hills 2383 (plain grass), Woods 4312
    // (grass + pond), Dusting 3454 (snow), Lilygear 2693 (water), Mine
    // 2713 (stone). ("make background" 1952 is a 32x32 editor-checker
    // placeholder — the dump has no exported grass tile.)
    {
        const i32 area = game.GetAreaId();
        const int bgImg = (area == 3)  ? 4312
                        : (area == 5)  ? 3454
                        : (area == 8)  ? 2693
                        : (area == 11) ? 2713
                        : 2383;                     // Hills + fallback
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", bgImg);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (t && t->texture) {
            const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
            const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
            m_batch->Draw(t->texture, 0.0f, 0.0f, 1280.0f, 720.0f,
                          0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
        } else {
            m_batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f, 0xFF3E7A2A);
        }
    }

    // camera window in world px (800x480 dump px, clamped to the map)
    f32 camL = game.GetCamX() * (f32)kFwTile - 400.0f;
    f32 camT = game.GetCamY() * (f32)kFwTile - 240.0f;
    if (camL < 0.0f) camL = 0.0f;
    if (camT < 0.0f) camT = 0.0f;
    if (camL > (f32)(kFwMapW * kFwTile) - 800.0f) camL = (f32)(kFwMapW * kFwTile) - 800.0f;
    if (camT > (f32)(kFwMapH * kFwTile) - 480.0f) camT = (f32)(kFwMapH * kFwTile) - 480.0f;
    const f32 camR = camL + 800.0f, camB = camT + 480.0f;
    // the leader's top px — layer-1 scenery with a smaller top draws behind
    // the actors, the rest in front (painter's by top)
    const f32 freddyTopPx = game.GetFreddyY() * (f32)kFwTile;

    // pass 1: layer 0 (ground/decor) + layer 1 scenery BEHIND the actors
    for (int i = 0; i < kFwMapInstCount; ++i) {
        const FwMapInst& it = kFwMapInsts[i];
        if (it.img == 0xFFFF) continue;                       // no-image marker
        if (it.left > (i32)camR + 64 || it.top > (i32)camB + 160) continue;   // sorted: cheap reject
        const bool behind = (it.layer == 0) || (it.top <= (i32)freddyTopPx);
        if (!behind) continue;
        PakLoadedTexture* t;
        if ((int)it.img == s_lastImg) {
            t = s_lastTex;
        } else {
            char name[32];
            Snprintf(name, sizeof(name), "img_%d", (int)it.img);
            t = m_pak ? m_pak->FindTexture(name) : 0;
            s_lastImg = it.img; s_lastTex = t;
        }
        if (!t || !t->texture) continue;
        const f32 w = (f32)t->origWidth, h = (f32)t->origHeight;
        if (it.left + w < camL - 64 || it.top + h < camT - 64) continue;
        const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
        const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
        if (it.layer == 0) {
            m_batch->Draw(t->texture,
                          ((f32)it.left - camL) * kSx, ((f32)it.top - camT) * kSy,
                          w * kSx, h * kSy, 0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
        } else {
            // layer 1 scenery (trees): point sampling keeps the DXT
            // canopies from smearing into blocks (the title-bg rule)
            m_batch->DrawPoint(t->texture,
                               ((f32)it.left - camL) * kSx, ((f32)it.top - camT) * kSy,
                               w * kSx, h * kSy, 0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
        }
    }

    // pass 2: coin chests (exe "spawn chest" sprite 3340; opened -> dark)
    for (int c = 0; c < game.GetChestCount(); ++c) {
        f32 tx, ty; bool big, opened;
        game.GetChest(c, tx, ty, big, opened);
        const f32 px = tx * (f32)kFwTile, py = ty * (f32)kFwTile;
        if (px < camL - 32 || px > camR + 32 || py < camT - 32 || py > camB + 32) continue;
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", 3340);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (!t || !t->texture) continue;
        const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
        const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
        const u32 tint = opened ? 0xFF606060 : 0xFFFFFFFF;
        m_batch->Draw(t->texture, (px - camL) * kSx, (py - camT) * kSy,
                      (f32)t->origWidth * kSx, (f32)t->origHeight * kSy,
                      0.0f, 0.0f, u1, v1, tint);
    }

    // pass 3: the party leader — exe 'character' anims, front-facing, 15 fps
    // walk cycle, idle = frame 0 (application.json). Sprite 60x60 px world
    // space (C-Edition scale 0.95) anchored on the hitbox.
    {
        const int lead = game.GetLeaderAnim();
        int ci = 0;
        for (int k = 0; k < 4; ++k) if (kFwLeaderAnim[k] == lead) ci = k;
        int frame = 0;
        if (game.GetFreddyDir() != 0)
            frame = 1 + ((int)game.GetWalkT() % 10);
        const int img = kFwCharFrames[ci][frame];
        char name[32];
        Snprintf(name, sizeof(name), "img_%d", img);
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
        if (t && t->texture) {
            const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
            const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
            // hitbox centre-x; sprite feet a bit below the hitbox bottom
            const f32 cx = (game.GetFreddyX() + 0.35f) * (f32)kFwTile;
            const f32 by = (game.GetFreddyY() + 0.45f) * (f32)kFwTile + 12.0f;
            const f32 w = 57.0f, h = 57.0f;      // 60 px * scale 0.95
            m_batch->Draw(t->texture, (cx - camL) * kSx - w * kSx * 0.5f,
                          (by - camT) * kSy - h * kSy,
                          w * kSx, h * kSy, 0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
        }
    }

    // pass 4: layer 1 scenery IN FRONT of the actors
    for (int i = 0; i < kFwMapInstCount; ++i) {
        const FwMapInst& it = kFwMapInsts[i];
        if (it.layer != 1) continue;
        if (it.img == 0xFFFF) continue;                       // no-image marker
        if (it.top <= (i32)freddyTopPx) continue;
        if (it.left > (i32)camR + 64 || it.top > (i32)camB + 160) continue;   // sorted: cheap reject
        PakLoadedTexture* t;
        if ((int)it.img == s_lastImg) {
            t = s_lastTex;
        } else {
            char name[32];
            Snprintf(name, sizeof(name), "img_%d", (int)it.img);
            t = m_pak ? m_pak->FindTexture(name) : 0;
            s_lastImg = it.img; s_lastTex = t;
        }
        if (!t || !t->texture) continue;
        const f32 w = (f32)t->origWidth, h = (f32)t->origHeight;
        if (it.left + w < camL - 64 || it.top + h < camT - 64) continue;
        const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
        const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
        m_batch->DrawPoint(t->texture,
                           ((f32)it.left - camL) * kSx, ((f32)it.top - camT) * kSy,
                           w * kSx, h * kSy, 0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
    }

    // the zone header + token counter (the exe draws zone name plates)
    if (m_text) {
        m_text->DrawText(16, 12, FNaFWorldGame::AreaName(game.GetAreaId()), 0xFFFFFFFF);
        char tok[64];
        Snprintf(tok, sizeof(tok), "Faz Tokens: %u", (unsigned)game.GetFazTokens());
        m_text->DrawText(1040, 12, tok, 0xFFFFFF00);
    }
}

// centred native draw: the plate centred on a dump-space point (the exe
// drives the stack with expression acts the event dump does not carry,
// so the menu is centred — labeled)
void FNaFWorldRender::DrawCentre(int img, f32 dcx, f32 dcy, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", img);
    PakLoadedTexture* t = m_pak ? m_pak->FindTexture(name) : 0;
    if (!t || !t->texture || !m_batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    const f32 w = (f32)t->origWidth * kSx, h = (f32)t->origHeight * kSy;
    m_batch->Draw(t->texture, dcx * kSx - w * 0.5f, dcy * kSy - h * 0.5f,
                  w, h, 0.0f, 0.0f, u1, v1, color);
}

// ------------------------------------------------------------
// exe frame 3 "file setup" (v2.91): bg 3407, the three slot plates
// (3338/3343/3344), the picked-slot frame (4234), then the stage rows
// (new game 3352 / continue 3351, YES 3353 / NO 3354). The summary
// lines mirror the exe's "Name: X : Level: N" shape; the port's single
// backing save shows in slot 1 (labeled deviation).
// ------------------------------------------------------------
void FNaFWorldRender::RenderFileSetup(const FNaFWorldGame& game) {
    if (!m_batch) return;
    // the backdrop (800x480 -> full screen)
    {
        PakLoadedTexture* t = m_pak ? m_pak->FindTexture("img_3407") : 0;
        if (t && t->texture) {
            const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
            const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
            m_batch->Draw(t->texture, 0.0f, 0.0f, 1280.0f, 720.0f,
                          0.0f, 0.0f, u1, v1, 0xFFFFFFFF);
        } else {
            m_batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f, 0xFF101418);
        }
    }

    const i32 stage = game.GetFsStage();
    // stage 0: the three slot plates, centred
    if (stage == 0) {
        static const int kSlotImg[3] = { 3338, 3343, 3344 };
        // the picked-slot frame first (it slides under the plate)
        DrawCentre(4234, 400.0f, 130.0f + game.GetFsSlot() * 80.0f, 0xFFFFFFFF);
        for (int i = 0; i < 3; ++i) {
            DrawCentre(kSlotImg[i], 400.0f, 130.0f + i * 80.0f, 0xFFFFFFFF);
            // the exe's summary shape: "Name: X : Level: N" — beside the stack
            char line[96];
            if (i == 0 && game.GetFsSaveLive())
                Snprintf(line, sizeof(line), "Freddy : Level %d", game.GetFsLevel());
            else
                Snprintf(line, sizeof(line), "New Game");
            if (m_text) m_text->DrawText(880, (i32)((130.0f + i * 80.0f) * kSy) - 8, line, 0xFFE8E8E8);
        }
        if (m_text) m_text->DrawText(540, 600, "A: pick   B: back", 0xFFD0D0D0);
        return;
    }
    // stage 1: new game / continue
    if (stage == 1) {
        DrawCentre(3352, 400.0f, 200.0f, game.GetFsRow() == 0 ? 0xFFFFFFFF : 0xFF808080);
        DrawCentre(3351, 400.0f, 290.0f, game.GetFsRow() == 1 ? 0xFFFFFFFF : 0xFF808080);
        if (m_text) {
            m_text->DrawText(540, 600, "A: pick   B: back", 0xFFD0D0D0);
            if (!game.GetFsSaveLive())
                m_text->DrawText(540, 630, "(no save to continue)", 0xFF909090);
        }
        return;
    }
    // stage 2: YES / NO
    DrawCentre(3353, 400.0f, 230.0f, game.GetFsRow() == 0 ? 0xFFFFFFFF : 0xFF808080);
    DrawCentre(3354, 400.0f, 320.0f, game.GetFsRow() == 1 ? 0xFFFFFFFF : 0xFF808080);
    if (m_text) m_text->DrawText(540, 600, "A: confirm   B: back", 0xFFD0D0D0);
}

// ------------------------------------------------------------
// the world menus (v2.91 WIP): the interaction is fully wired (Y/START ->
// root -> party/chips/bytes); the frame art (4/7/12) lands next — the
// text panels carry the state meanwhile.
// ------------------------------------------------------------
void FNaFWorldRender::RenderMenu(const FNaFWorldGame& game) {
    if (!m_batch || !m_text) return;
    static const char* kOwnedNames[8] = { "Freddy", "Bonnie", "Chica", "Foxy",
                                          "Toy Bonnie", "Char 5", "Char 6", "Char 7" };
    static const i32 kOwnedIds[8] = { 0, 12, 13, 14, 15, 16, 17, 18 };
    // the party member's display name (match the anim id against the list)
    struct NameOf {
        static const char* Get(i32 anim) {
            static const char* kN[8] = { "Freddy", "Bonnie", "Chica", "Foxy",
                                         "Toy Bonnie", "Char 5", "Char 6", "Char 7" };
            static const i32 kId[8] = { 0, 12, 13, 14, 15, 16, 17, 18 };
            for (int i = 0; i < 8; ++i) if (kId[i] == anim) return kN[i];
            return "?";
        }
    };
    (void)kOwnedNames; (void)kOwnedIds;

    m_batch->DrawSolid(160.0f, 60.0f, 960.0f, 600.0f, 0xD8101418);
    const i32 scr = game.GetMenuScreen();
    if (scr == 1) {
        static const char* kRows[3] = { "PARTY", "CHIPS", "BYTES" };
        for (int i = 0; i < 3; ++i)
            m_text->DrawText(560, 200 + i * 90, kRows[i],
                             game.GetMenuRow() == i ? 0xFF60FF60 : 0xFFC0C0C0);
        return;
    }
    if (scr == 2) {   // PARTY: the four members + the owned list
        m_text->DrawText(220, 100, "PARTY  (L/R member, U/D list, A swap)", 0xFF90EE90);
        for (int i = 0; i < 4; ++i) {
            char line[64];
            Snprintf(line, sizeof(line), "%d: %s", i + 1, NameOf::Get(game.GetParty(i)));
            m_text->DrawText(220, 160 + i * 50, line,
                             game.GetMenuMember() == i ? 0xFF60FF60 : 0xFFC0C0C0);
        }
        for (int i = 0; i < 8; ++i)
            m_text->DrawText(620, 160 + i * 50, NameOf::Get(kOwnedIds[i]),
                             game.GetMenuRow() == i ? 0xFF60FF60 : 0xFFC0C0C0);
        return;
    }
    if (scr == 3) {   // CHIPS: the member row + the 8 chip slots
        m_text->DrawText(220, 100, "CHIPS  (U/D member, L/R chip, A equip)", 0xFF90EE90);
        m_text->DrawText(220, 160 + game.GetMenuMember() * 50,
                         NameOf::Get(game.GetParty(game.GetMenuMember())), 0xFF60FF60);
        for (int i = 0; i < 8; ++i) {
            char line[64];
            const bool owned = (game.GetChipsOwned() & (1u << i)) != 0;
            Snprintf(line, sizeof(line), "chip %d%s", i, owned ? "" : " (locked)");
            m_text->DrawText(560 + (i % 4) * 160, 180 + (i / 4) * 70, line,
                             game.GetMenuCol() == i ? 0xFF60FF60 : 0xFFC0C0C0);
        }
        return;
    }
    // BYTES: the owned list (the deploy model is a later mine)
    m_text->DrawText(220, 100, "BYTES", 0xFF90EE90);
    m_text->DrawText(220, 170, "(none collected yet)", 0xFFC0C0C0);
}

// ------------------------------------------------------------
// exe frame 6 "game over" (v2.91): the single instance — img 3405 at its
// layout point (153,134 in the 800x480 frame) over black.
// ------------------------------------------------------------
void FNaFWorldRender::RenderGameOver() {
    if (m_batch) m_batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f, 0xFF000000);
    DNS(3405, 153.0f, 134.0f, 0xFFFFFFFF);
}

// ------------------------------------------------------------
// SCR_BATTLE — v2.91 step 3: the Battle_Rework.c layout.
// 854x480 virtual screen (scaled x1.5 to 1280x720); entity slots in
// battle-space units *66.667 + (427,240) px: enemies L_U/L_L/L_R/L_D,
// party R_U/R_L/R_R/R_D. Health bars ride the right edge (DisplayHUD),
// graves stand on dead party slots, damage floats rise and fade.
// Enemy/party sprites are drawn as labeled placeholders until the exe
// battle-event mining names their pak indices (the battle runs inside
// the overworld frame's 'battle' counter in the exe).
// ------------------------------------------------------------
static void FwBattleSlotPx(bool enemy, int slot, f32& px, f32& py) {
    // L_U(-3.5,-3) L_L(-5.5,-2) L_R(-2.5,-1) L_D(-4.5,0)
    // R_U( 2,-3)   R_L( 4,-2)   R_R( 1,-1)   R_D( 3, 0)
    static const f32 kSlotXY[8][2] = {
        { -3.5f, -3.0f }, { -5.5f, -2.0f }, { -2.5f, -1.0f }, { -4.5f, 0.0f },
        {  2.0f, -3.0f }, {  4.0f, -2.0f }, {  1.0f, -1.0f }, {  3.0f, 0.0f }
    };
    const int i = enemy ? slot : slot + 4;
    px = kSlotXY[i][0] * 66.6667f + 427.0f;
    py = kSlotXY[i][1] * 66.6667f + 240.0f;
}

static const char* FwCharName(int charId) {
    static const char* kNames[4] = { "Freddy", "Bonnie", "Chica", "Foxy" };
    return (charId >= 0 && charId < 4) ? kNames[charId] : "Enemy";
}

void FNaFWorldRender::RenderBattle(const FNaFWorldGame& game) {
    if (!m_batch) return;
    const FwBattleState& b = game.GetBattle();

    // the virtual screen: 854x480 -> 1280x720
    const f32 sx = 1280.0f / 854.0f, sy = 720.0f / 480.0f;

    // background + the ground line (zone-tinted; exe bg art pending mining)
    m_batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f, 0xFF1A2416);
    m_batch->DrawSolid(0.0f, 430.0f, 1280.0f, 290.0f, 0xFF22301A);
    m_batch->DrawSolid(0.0f, 428.0f, 1280.0f, 4.0f, 0xFF0D120A);

    // entities
    for (int i = 0; i < b.enemyCount; ++i) {
        const FwBattleMember& e = b.e[i];
        f32 px, py; FwBattleSlotPx(true, i, px, py);
        const f32 dx = px * sx, dy = py * sy;
        if (!e.dead) {
            m_batch->DrawSolid(dx, dy, 66.0f, 66.0f, 0xFFB04030);
            m_batch->DrawSolid(dx + 3.0f, dy + 3.0f, 60.0f, 60.0f, 0xFF8C3024);
        }
        // HP strip above + the name
        const f32 frac = e.maxHp ? (f32)e.hp / (f32)e.maxHp : 0.0f;
        m_batch->DrawSolid(dx, dy - 14.0f, 66.0f, 8.0f, 0xFF000000);
        m_batch->DrawSolid(dx + 1.0f, dy - 13.0f, 64.0f * frac, 6.0f, 0xFFE04040);
        if (m_text) {
            m_text->DrawText((i32)dx, (i32)(dy + 70.0f), "MechRab", 0xFFC0C0C0);
        }
    }
    for (int i = 0; i < 4; ++i) {
        const FwBattleMember& p = b.p[i];
        f32 px, py; FwBattleSlotPx(false, i, px, py);
        const f32 dx = px * sx, dy = py * sy;
        if (p.dead) {
            // the grave (RenderBattleEntity: a dead party slot draws one)
            m_batch->DrawSolid(dx + 8.0f, dy + 10.0f, 50.0f, 56.0f, 0xFF888888);
            m_batch->DrawSolid(dx + 14.0f, dy + 18.0f, 38.0f, 40.0f, 0xFF666666);
            if (m_text) m_text->DrawText((i32)(dx + 20.0f), (i32)(dy + 66.0f), "RIP", 0xFF909090);
            continue;
        }
        static const u32 kCharTint[4] = { 0xFF503820, 0xFF204070, 0xFF705018, 0xFF702020 };
        m_batch->DrawSolid(dx, dy, 66.0f, 66.0f, kCharTint[p.charId & 3]);
        m_batch->DrawSolid(dx + 3.0f, dy + 3.0f, 60.0f, 60.0f,
                           p.cd <= 0.0f ? 0xFFFFFFFF : 0xFFB8B8B8);
        // the ready flash: a full-white inner box means A will strike now
        const f32 frac = p.maxHp ? (f32)p.hp / (f32)p.maxHp : 0.0f;
        m_batch->DrawSolid(dx, dy - 14.0f, 66.0f, 8.0f, 0xFF000000);
        m_batch->DrawSolid(dx + 1.0f, dy - 13.0f, 64.0f * frac, 6.0f, 0xFF40E040);
        if (m_text) {
            m_text->DrawText((i32)dx, (i32)(dy + 70.0f), FwCharName(p.charId), 0xFFFFFFFF);
        }
    }

    // floating damage numbers
    for (int i = 0; i < game.GetDamageCount(); ++i) {
        f32 x, y, age; i32 amt; bool enemyHit;
        game.GetDamage(i, x, y, amt, age, enemyHit);
        if (amt <= 0 || age < 0.0f || age > 0.9f) continue;
        const int slot = (int)x;
        f32 px, py; FwBattleSlotPx(enemyHit, slot, px, py);
        const f32 rise = age * 60.0f;
        const u32 a = (u32)((age < 0.45f ? 1.0f : 1.0f - (age - 0.45f) / 0.45f) * 255.0f);
        char num[16];
        Snprintf(num, sizeof(num), "%d", (int)amt);
        if (m_text) {
            m_text->DrawText((i32)(px * sx + 20.0f), (i32)(py * sy - 24.0f - rise),
                             num, (a << 24) | (enemyHit ? 0xFF5050 : 0xFFFFFF));
        }
    }

    // HUD: enemy name top-left, party health bars right (DisplayHUD)
    if (m_text) {
        m_text->DrawText(20, 16, "MechRab", 0xFFFFFFFF);
        m_text->DrawText(1050, 660, "A = attack", 0xFF90EE90);
    }
    for (int i = 0; i < 4; ++i) {
        const FwBattleMember& p = b.p[i];
        const f32 bx = 1020.0f, by = 16.0f + i * 34.0f;
        const f32 frac = p.maxHp ? (f32)p.hp / (f32)p.maxHp : 0.0f;
        m_batch->DrawSolid(bx, by, 200.0f, 20.0f, 0xFF000000);
        m_batch->DrawSolid(bx + 2.0f, by + 2.0f, 196.0f * frac, 16.0f, 0xFFE03030);
        if (m_text) {
            char hp[40];
            Snprintf(hp, sizeof(hp), "%s %d", FwCharName(p.charId), (int)p.hp);
            m_text->DrawText((i32)(bx - 90.0f), (i32)(by + 4.0f), hp, 0xFFFFFFFF);
        }
    }

    if (b.result == 1 && m_text) m_text->DrawText(540, 300, "VICTORY!", 0xFF60FF60);
    if (b.result == 2 && m_text) m_text->DrawText(540, 300, "GAME OVER", 0xFFFF6060);
}

// ------------------------------------------------------------
// the CFT frame transition overlay — our core's shared black model
// (v2.76), drawn LAST by the module.
// ------------------------------------------------------------
void FNaFWorldRender::DrawFade(f32 alpha) {
    if (alpha <= 0.0f || !m_batch) return;
    const u32 a = (u32)(alpha * 255.0f);
    m_batch->DrawSolid(0.0f, 0.0f, 1280.0f, 720.0f, (a << 24) | 0x00000000);
}

} // namespace fnaf