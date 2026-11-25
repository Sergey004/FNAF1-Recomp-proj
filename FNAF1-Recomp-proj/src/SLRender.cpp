/**
 * SLRender.cpp: v2.65 — the Sister Location renderer (wave 1).
 * The window is 1280x720 native (no scale). Room art rides the dump's own
 * Backdrop/Active cells (the wider rooms pan); the text captions are the
 * debug font until the gfx pinboards land in wave 2 (labeled).
 */

#include "SLRender.h"
#include "SLGame.h"
#include "SLRooms.h"
#include "PakLoader.h"
#include "SpriteBatch.h"
#include "TextRenderer.h"
#include "XdkCompat.h"
#include <cstdio>

namespace fnaf {

void SLRender::Init(PakLoader* pak, SpriteBatch* batch, TextRenderer* text) {
    m_pak = pak; m_batch = batch; m_text = text;
}

static void D(PakLoader* pak, SpriteBatch* batch, int handle,
              float x, float y, float w, float h, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = pak ? pak->FindTexture(name) : 0;
    if (!t || !t->texture || !batch) return;
    // v2.72: clamp UVs to the ORIGINAL size — pak textures are GPU-padded to
    // 32px-aligned dims and the padding rows decode to junk; plain 0..1
    // sampling pulled that junk into the quad.
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    batch->Draw(t->texture, x, y, w, h, 0.0f, 0.0f, u1, v1, color);
}

// native-size draw (the layout tables' left/top = origin minus the hot spot)
static void DN(PakLoader* pak, SpriteBatch* batch, int handle,
               float x, float y, u32 color) {
    char name[32];
    Snprintf(name, sizeof(name), "img_%d", handle);
    PakLoadedTexture* t = pak ? pak->FindTexture(name) : 0;
    if (!t || !t->texture || !batch) return;
    const f32 u1 = t->alignedWidth  ? (f32)t->origWidth  / (f32)t->alignedWidth  : 1.0f;
    const f32 v1 = t->alignedHeight ? (f32)t->origHeight / (f32)t->alignedHeight : 1.0f;
    batch->Draw(t->texture, x, y, (f32)t->origWidth, (f32)t->origHeight,
                0.0f, 0.0f, u1, v1, color);
}

// ------------------------------------------------------------
// v2.124: the rooms come from the dump's own frame-start state.
//
// The generated SLRooms.h tables carry, per room, the sprites the dump
// leaves visible at the first tick (the room art, the plates, the icons)
// and the animated objects with their cell sequences (every cell with its
// own size and hotspot). The room is drawn in the dump's world space and
// scaled to the 1280x720 window - the height-squash convention the port
// uses for the wide rooms - and the game's pan shifts it sideways.
// ------------------------------------------------------------
static f32 RoomScale(int roomH) { return 720.0f / (f32)(roomH > 0 ? roomH : 1); }

static f32 RoomOffsetX(int roomW, int roomH, f32 panX) {
    return (1280.0f - (f32)roomW * RoomScale(roomH)) * 0.5f - panX;
}

static void DrawRoom(PakLoader* pak, SpriteBatch* batch,
                     int roomW, int roomH,
                     const SLRoomSprite* sprites, int spriteCount,
                     const SLRoomAnim* anims, int animCount,
                     f32 time, f32 panX, int skipAnim) {
    if (!batch) return;
    const f32 scale = RoomScale(roomH);
    const f32 offX = RoomOffsetX(roomW, roomH, panX);
    for (int i = 0; i < spriteCount; ++i) {
        const SLRoomSprite& s = sprites[i];
        D(pak, batch, s.img, offX + (f32)s.x * scale, (f32)s.y * scale,
          (f32)s.w * scale, (f32)s.h * scale, 0xFFFFFFFF);
    }
    for (int i = 0; i < animCount; ++i) {
        if (i == skipAnim) continue;
        const SLRoomAnim& a = anims[i];
        if (!a.cells || a.count <= 0) continue;
        int idx = (int)(time * a.fps);
        idx %= a.count;
        if (idx < 0) idx += a.count;
        const SLRoomAnimCell& c = a.cells[idx];
        D(pak, batch, c.img, offX + (f32)c.x * scale, (f32)c.y * scale,
          (f32)c.w * scale, (f32)c.h * scale, 0xFFFFFFFF);
    }
}

// one cell of an animated object, placed by the gameplay (the Ballora's
// dancer stands on the gallery floor, not on the dump's event position)
static void DrawRoomAnim(PakLoader* pak, SpriteBatch* batch, const SLRoomAnim& a,
                         f32 time, f32 x, f32 y, f32 scale) {
    if (!batch || !a.cells || a.count <= 0) return;
    int idx = (int)(time * a.fps);
    idx %= a.count;
    if (idx < 0) idx += a.count;
    const SLRoomAnimCell& c = a.cells[idx];
    D(pak, batch, c.img, x, y, (f32)c.w * scale, (f32)c.h * scale, 0xFFFFFFFF);
}

void SLRender::RenderWarning(const SLGame& game) {
    // frame 0: the legal splash (the dump's warning cell run 414-417).
    // The dump frame has NO hint caption — the old "press A" debug line is
    // gone (img 366 is the thin rule under the legal block, drawn at its
    // layout point 550,687).
    //
    // v2.74: the dump's frame 0 group 1 re-rolls the splash's alpha
    // coefficient to 100 + Random(150) every 100 ms — the legal text
    // shimmers. The port drew it at full alpha.
    if (m_batch) {
        const f32 t = game.GetTime();
        const int slot = (int)(t / 0.1f);
        unsigned h = (unsigned)slot * 2654435761u + 17u;
        h ^= h >> 13;  h *= 3266489917u;  h ^= h >> 16;
        const int coeff = 100 + (int)(h % 150u);        // 100..249
        const u32 alpha = (u32)(255 - coeff);           // 6..155 (0.02..0.61)
        static const int kCells[4] = { 414, 415, 416, 417 };
        D(m_pak, m_batch, kCells[(int)(t * 4.0f) % 4], 0.0f, 0.0f,
          1280.0f, 720.0f, (alpha << 24) | 0x00FFFFFFu);
        DN(m_pak, m_batch, 366, 550.0f, 687.0f, 0xFFFFFFFF);
    }
}

void SLRender::RenderTitle(const SLGame& game, f32 time) {
    // frame 1 "title screen" per the dump's FrameLayout table:
    //   layer 0: Backdrop 100 @(0,0) — the only bg cell the dump lists
    //            (the wave-1 267-269 cycle was a guess, corrected);
    //            Active13/14/15 (402/406/410) are the flicker alternates —
    //            the dump swaps them via act #17/#31 groups, schedule not
    //            decoded yet, so the rest state (402) stands (DEVIATION).
    //   layer 1: the menu rows are BAKED plates at left/top from the table
    //            (hotY=11 folded in): new game 398 @(849,239), continue 399
    //            @(849,289), extras 400 @(849,339), custom night 2095
    //            @(849,392); the underline Active 12 (img 401, 1280x10)
    //            follows the selected row (act #3; snapshot sits it +12
    //            under the row top); the bottom bar Active 7 = 394
    //            (750x40) @ (364,648).
    // Rows tint by the groups 23-26/65-72 state machine: selected / normal /
    // locked(beat1||beat3 gate). act #65's exact coefficients are undecoded
    // — the three dump STATES are kept, the tint values are ours (DEVIATION).
    (void)time;
    if (m_batch) {
        D(m_pak, m_batch, 100, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
        D(m_pak, m_batch, 402, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
        static const struct { int img; float top; } kRows[4] = {
            { 398, 239.0f }, { 399, 289.0f }, { 400, 339.0f }, { 2095, 392.0f }
        };
        const int sel = game.GetOptionSelected();
        for (int i = 0; i < 4; ++i) {
            const bool lock = (i == 2 && !game.IsBeat1()) ||
                              (i == 3 && !game.IsBeat3());
            u32 col = (sel == i) ? 0xFFFFFFFF : 0xFF909090;
            if (lock) col = 0xFF404040;
            DN(m_pak, m_batch, kRows[i].img, 849.0f, kRows[i].top, col);
        }
        if (sel >= 0 && sel < 4)
            DN(m_pak, m_batch, 401, 0.0f, kRows[sel].top + 12.0f, 0xFFFFFFFF);
        DN(m_pak, m_batch, 394, 364.0f, 648.0f, 0xFFFFFFFF);
    }
    // the dump's own text ends there; the night-number counter (img 698
    // @(1030,309), one dumped digit frame) waits for the counter-art pass.
}

void SLRender::RenderElevator(const SLGame& game, f32 time) {
    // v2.124: the dump's frame 2 (the elevator ride): the cabin's own
    // 10-cell sequence at 54 fps (the dump's animation speed 90), the
    // shaft plate (img 69) above and below the cabin, the "NIGHT 1"
    // plate and the mouse prompt - the objects the dump leaves visible
    // at the frame start. The old build drew four stretched cells and
    // invented captions.
    (void)game;
    DrawRoom(m_pak, m_batch, SLElevRoomW, SLElevRoomH,
             SLElevSprites, SLElevSpriteCount, SLElevAnims, SLElevAnimCount,
             time, 0.0f, -1);
}

void SLRender::RenderVent(const SLGame& game) {
    // v2.124: the dump's frame 3 (the vent crawl): the tunnel's own
    // 16-cell sequence at 30 fps (the dump's speed 50) plus the crawl and
    // mouse prompts. The old build panned the cell by the notch counter
    // (up to 60000 px - the art left the screen long before); the dump
    // scrolls the room's own sequence instead.
    DrawRoom(m_pak, m_batch, SLVentRoomW, SLVentRoomH,
             SLVentSprites, SLVentSpriteCount, SLVentAnims, SLVentAnimCount,
             game.GetTime(), 0.0f, -1);
}

void SLRender::RenderHub(const SLGame& game) {
    // v2.124: the dump's frame 5 (the Main Hub): the room art (the 16-cell
    // "Active" sequence), the two vent mouths, the sparks, the map icons
    // and the mouse prompt - the objects the dump leaves visible.
    DrawRoom(m_pak, m_batch, SLHubRoomW, SLHubRoomH,
             SLHubSprites, SLHubSpriteCount, SLHubAnims, SLHubAnimCount,
             game.GetTime(), 0.0f, -1);
}

void SLRender::RenderBaby(const SLGame& game) {
    // v2.124: the dump's frame 6 (Baby's Room): the gallery art (img 829),
    // the backdrops, the sparks, the Bidybab and the mouse prompts.
    DrawRoom(m_pak, m_batch, SLBabyRoomW, SLBabyRoomH,
             SLBabySprites, SLBabySpriteCount, SLBabyAnims, SLBabyAnimCount,
             game.GetTime(), 0.0f, -1);
}

void SLRender::RenderBallora(const SLGame& game, f32 time) {
    // v2.124: the dump's frame 8 (the Ballora Gallery): the floor's 11-cell
    // sequence at 15 fps, the sliding door (11 cells at 30 fps), the gallery
    // plates and the prompts. Ballora herself is the dump's "dancer" object
    // (32 cells at 24 fps) - the dump places her by the events, so the
    // gameplay puts her on the gallery floor (the port's own approach).
    const f32 scale = RoomScale(SLBalloraRoomH);
    const f32 offX = RoomOffsetX(SLBalloraRoomW, SLBalloraRoomH, game.GetPan());
    DrawRoom(m_pak, m_batch, SLBalloraRoomW, SLBalloraRoomH,
             SLBalloraSprites, SLBalloraSpriteCount,
             SLBalloraAnims, SLBalloraAnimCount, time, game.GetPan(),
             SLBalloraAnimDancer);
    if (game.GetDistance() > 0 && SLBalloraAnimCount > SLBalloraAnimDancer) {
        // she stands on the floor (the room's floor top at y=768)
        const f32 w = (f32)SLBalloraAnims[SLBalloraAnimDancer].cells[0].w * scale;
        const f32 h = (f32)SLBalloraAnims[SLBalloraAnimDancer].cells[0].h * scale;
        const f32 x = offX + 1100.0f * scale - w * 0.5f +
                      (f32)game.GetLeftPan() * 3.0f;
        const f32 y = 768.0f * scale - h;
        DrawRoomAnim(m_pak, m_batch, SLBalloraAnims[SLBalloraAnimDancer],
                     time, x, y, scale);
    }
}

void SLRender::RenderFuntime(const SLGame& game, f32 time) {
    // v2.124: the dump's frame 10 (the Funtime Auditorium): the floor and
    // the door (the dump's own sequences), the plates and the prompts.
    // The Foxy silhouettes are event-gated in the dump (they appear with
    // his approach), so the gameplay draws them - at the dump's own
    // positions and sizes (frame 10: "close Foxy 3" 730,573 250x315,
    // "close Foxy 2" 841,490 370x450, "close Foxy" 1028,349 550x650).
    const f32 scale = RoomScale(SLFuntimeRoomH);
    const f32 offX = RoomOffsetX(SLFuntimeRoomW, SLFuntimeRoomH, 0.0f);
    DrawRoom(m_pak, m_batch, SLFuntimeRoomW, SLFuntimeRoomH,
             SLFuntimeSprites, SLFuntimeSpriteCount,
             SLFuntimeAnims, SLFuntimeAnimCount, time, 0.0f, -1);
    if (game.GetFoxyDist() > 100 && game.GetProgress() < 1300) {
        static const int kSil3[4] = { 1306, 2024, 2026, 2036 };
        static const int kSil2[4] = { 1827, 1455, 2010, 1457 };
        static const int kSil1[4] = { 1454, 356, 1456, 363 };
        const int s = game.GetFoxyDist() > 400 ? 2 : (game.GetFoxyDist() > 200 ? 1 : 0);
        const int fr = (int)(time * 6.0f) % 4;
        if (s == 0)
            D(m_pak, m_batch, kSil3[fr], offX + 730.0f * scale, 573.0f * scale,
              250.0f * scale, 315.0f * scale, 0xFFFFFFFF);
        else if (s == 1)
            D(m_pak, m_batch, kSil2[fr], offX + 841.0f * scale, 490.0f * scale,
              370.0f * scale, 450.0f * scale, 0xFFFFFFFF);
        else
            D(m_pak, m_batch, kSil1[fr], offX + 1028.0f * scale, 349.0f * scale,
              550.0f * scale, 650.0f * scale, 0xFFFFFFFF);
    }
}

void SLRender::RenderBreaker(const SLGame& game) {
    // v2.124: the dump's frame 9 (the Breaker Room): the hall art (img 594),
    // the breaker panel, the screen, the map icons, the restart plate and
    // the prompts. The task label ("jack in", img 579) and the raised
    // lever ("flip it up", img 1889) are event-gated in the dump, so the
    // gameplay adds them while the task runs.
    const f32 scale = RoomScale(SLBreakerRoomH);
    const f32 offX = RoomOffsetX(SLBreakerRoomW, SLBreakerRoomH, 0.0f);
    DrawRoom(m_pak, m_batch, SLBreakerRoomW, SLBreakerRoomH,
             SLBreakerSprites, SLBreakerSpriteCount,
             SLBreakerAnims, SLBreakerAnimCount, game.GetTime(), 0.0f, -1);
    if (m_batch) {
        const f32 y = 930.0f * scale;
        D(m_pak, m_batch, 579, offX + 310.0f * scale, y, 606.0f * scale, 55.0f * scale,
          0xFFFFFFFF);
        if (game.GetBrkFill() > 0.0f)
            D(m_pak, m_batch, 1889, offX + 340.0f * scale, y, 557.0f * scale, 38.0f * scale,
              0xFFFFFFFF);
    }
}

// v2.73: the win screen composed from the dump's frame_12 layout — the
// 45-cell firework grid (img 873, 32x32, layer 0) behind the "night
// complete" plate 1223 (400x120 @ (441,228), layer 1) and the "Enjoy"
// strip 1189 (283x25 @ (501,328)). Labeled: the dump shows these
// event-gated (the rest replay is black); the port draws the composed
// screen when the night ends.
void SLRender::RenderWinNight(const SLGame& game) {
    (void)game;
    if (!m_batch) return;
    static const f32 kFx[45][2] = {
        {179,113},{291,113},{403,113},{515,113},{627,113},{739,113},{851,113},{963,113},
        {1105,181},
        {205,255},{317,255},{429,255},{541,255},{653,255},{765,255},{877,255},{989,255},
        {1101,255},
        {179,337},{291,337},{403,337},{515,337},{627,337},{739,337},{851,337},{963,337},
        {1075,337},
        {133,191},{233,173},{345,173},{457,173},{569,173},{681,173},{793,173},{905,173},
        {1017,173},
        {219,397},{331,397},{443,397},{555,397},{667,397},{779,397},{891,397},{1003,397},
        {1115,397},
    };
    for (int i = 0; i < 45; ++i)
        DN(m_pak, m_batch, 873, kFx[i][0], kFx[i][1], 0xFFFFFFFF);
    DN(m_pak, m_batch, 1223, 441.0f, 228.0f, 0xFFFFFFFF);
    DN(m_pak, m_batch, 1189, 501.0f, 328.0f, 0xFFFFFFFF);
}

void SLRender::RenderTvShow(const SLGame& game) {
    // v2.74: the show's own art (frame 24: the 1900x900 "Active" cells
    // 2037/2034/2039/2035, the 114x112 "Active 5" 2138.., the static
    // 265x240 cells 2134.., "make show" 2221).
    const f32 t = game.GetTime();
    if (m_batch) {
        static const int kShow[4] = { 2037, 2034, 2039, 2035 };
        D(m_pak, m_batch, kShow[(int)(t * 4.0f) % 4], 0.0f, 0.0f, 1900.0f, 900.0f,
          0xFFFFFFFF);
        static const int kHead[6] = { 2138, 2139, 2140, 2141, 2142, 2143 };
        D(m_pak, m_batch, kHead[(int)(t * 6.0f) % 6], -28.0f, 26.0f, 114.0f, 112.0f,
          0xFFFFFFFF);
        static const int kStatic[4] = { 2134, 2135, 2136, 2137 };
        D(m_pak, m_batch, kStatic[(int)(t * 8.0f) % 4], 0.0f, 0.0f, 265.0f, 240.0f,
          0xFFFFFFFF);
        D(m_pak, m_batch, 2221, 640.0f, 382.0f, 32.0f, 32.0f, 0xFFFFFFFF);
    }
}

void SLRender::RenderGirlVoice() {
    if (m_text)
        m_text->DrawText(440, 340, "... she's still there", 0xFFE0E0E0);
}

void SLRender::RenderDeath(const SLGame& game, f32 time) {
    // v2.74: the death frame's own art (frame 14: obj 361 "Active" cells
    // 1491/1482/1493/1484 full screen + obj 362 "white" 1494). The old build
    // showed a black screen with a caption.
    (void)game;
    if (m_batch) {
        static const int kDeath[4] = { 1491, 1482, 1493, 1484 };
        D(m_pak, m_batch, kDeath[(int)(time * 6.0f) % 4], 0.0f, 0.0f, 1280.0f, 720.0f,
          0xFFFFFFFF);
    }
    if (m_text)
        m_text->DrawText(440, 420, "Press A to retry the night", 0xFF606060);
}

void SLRender::RenderGameOver() {
    // v2.73: black, per the frame_15 rest state (was an invented ". . .").
}

// ------------------------------------------------------------
// v2.91 finale-wave overlays: the Wave2 frame art stays underneath; the
// text panels carry the new screens' live state until the exe frame art
// is wired into the pak renders.
// ------------------------------------------------------------
void SLRender::RenderCustMenuOver(const SLGame& game) {
    if (!m_text) return;
    static const char* kRows[10] = {
        "Ballora", "Funtime Freddy", "Funtime Foxy", "Bidybab", "Minireena",
        "Minireena 2", "Yenndo", "Lolbit", "Electrobab", "Bonnet"
    };
    m_text->DrawText(480, 40, "CUSTOM NIGHT", 0xFFFFFFFF);
    for (int i = 0; i < 10; ++i) {
        const bool sel = (i == game.GetCustRow());
        char line[96];
        Snprintf(line, sizeof(line), "%s%s  %d", kRows[i],
                 sel ? "  <" : "   ", game.GetCustDiff(i));
        m_text->DrawText(420, 90 + i * 44, line, sel ? 0xFF60FF60 : 0xFFB0B0B0);
    }
    m_text->DrawText(420, 560, "left/right = level   A = start   B = back", 0xFF808080);
}

void SLRender::RenderCustLevelOver(const SLGame& game) {
    if (!m_text) return;
    static const char* kLanes[3] = { "Ballora  [LB]", "F.Foxy  [RB]", "Lolbit  [Y]" };
    char hour[32];
    Snprintf(hour, sizeof(hour), "%d AM", game.GetCustHour() == 12 ? 12 : game.GetCustHour());
    m_text->DrawText(590, 30, hour, 0xFFFFFFFF);
    for (int i = 0; i < 3; ++i) {
        const bool danger = game.GetCustLane(i);
        const bool closed = game.GetCustDoor(i);
        char line[96];
        Snprintf(line, sizeof(line), "%s  %s", kLanes[i],
                 closed ? "CLOSED" : (danger ? "!! AT THE DOOR" : "open"));
        m_text->DrawText(60, 520 + i * 44, line,
                         danger ? 0xFFFF5050 : (closed ? 0xFF60A0FF : 0xFF909090));
    }
}

void SLRender::RenderFinalOver(const SLGame& game) {
    if (!m_text) return;
    char hour[32];
    Snprintf(hour, sizeof(hour), "%d AM", game.GetFinalHour() == 12 ? 12 : game.GetFinalHour());
    m_text->DrawText(590, 30, hour, 0xFFE0E0E0);
    if (game.GetFinalCue())
        m_text->DrawText(430, 340, "HE IS AT THE VENT - HOLD A", 0xFFFF5050);
    else
        m_text->DrawText(500, 660, "wait for the cue...", 0xFF808080);
}

void SLRender::RenderHold(const char* caption) {
    if (m_text)
        m_text->DrawText(440, 340, caption ? caption : "the wave-2 room", 0xFF909090);
}

// v2.74: the wave-2 screens get their own art (the frames' own instances):
// 16 night-4 cutscene, 17 intro card, 26 the 8-bit game, 27 final encounter,
// 29 custom menu, 30 custom please wait, 31 custom level, 32 custom death,
// 33 custom night win, 34 custom 8-bit cutscene, 35 final cutscene.
// Frame 28 (the keypad) exports NO instances — it stays black (labeled).
void SLRender::RenderWave2(const SLGame& game) {
    const f32 t = game.GetTime();
    if (!m_batch) return;
    switch (game.GetScreen()) {
        case SLGame::SCR_CUTN4: {                    // frame 16
            static const int kAct[5] = { 299, 302, 305, 305, 305 };
            D(m_pak, m_batch, kAct[(int)(t * 6.0f) % 5], 1376.0f, 256.0f, 100.0f, 100.0f,
              0xFFFFFFFF);
            D(m_pak, m_batch, 313, 1762.0f, -27.0f, 800.0f, 800.0f, 0xFFFFFFFF);  // Ballora
            D(m_pak, m_batch, 874, 999.0f, 666.0f, 2000.0f, 630.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 842, 1248.0f, 480.0f, 1600.0f, 600.0f, 0xFFFFFFFF); // scooper
            break;
        }
        case SLGame::SCR_INTRO4:                    // frame 17
            D(m_pak, m_batch, 1252, 0.0f, 0.0f, 1280.0f, 3000.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 296, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
            break;
        case SLGame::SCR_GAME8BIT:                   // frame 26
            D(m_pak, m_batch, 1692, 0.0f, 0.0f, 300.0f, 300.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 1571, 2657.0f, 127.0f, 250.0f, 250.0f, 0xFFFFFFFF);
            break;
        case SLGame::SCR_FINAL: {                    // frame 27
            static const int kRoom[5] = { 2023, 2009, 2025, 2011, 2012 };
            D(m_pak, m_batch, kRoom[(int)(t * 4.0f) % 5], 0.0f, 0.0f, 1900.0f, 800.0f,
              0xFFFFFFFF);
            D(m_pak, m_batch, 1876, 780.0f, -2.0f, 317.0f, 215.0f, 0xFFFFFFFF);  // vent door
            D(m_pak, m_batch, 1866, 1874.0f, 6.0f, 339.0f, 800.0f, 0xFFFFFFFF);  // right door
            D(m_pak, m_batch, 1472, 0.0f, 0.0f, 339.0f, 800.0f, 0xFFFFFFFF);      // left door
            D(m_pak, m_batch, 1464, 405.0f, 468.0f, 51.0f, 56.0f, 0xFFFFFFFF);   // button
            break;
        }
        case SLGame::SCR_CUSTMENU:                   // frame 29
            for (int i = 0; i < 8; ++i)
                D(m_pak, m_batch, 2046, -34.0f + 75.0f * i, -13.0f, 75.0f, 75.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 2253, 1025.0f, 599.0f, 197.0f, 50.0f, 0xFFFFFFFF); // ready
            break;
        case SLGame::SCR_CUSTWAIT:                   // frame 30
            D(m_pak, m_batch, 2300, 1229.0f, 670.0f, 50.0f, 50.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 1462, 610.0f, 326.0f, 25.0f, 27.0f, 0xFFFFFFFF);
            break;
        case SLGame::SCR_CUSTLEVEL: {                // frame 31
            static const int kRoom[5] = { 2023, 2009, 2025, 2011, 2012 };
            D(m_pak, m_batch, kRoom[(int)(t * 4.0f) % 5], 0.0f, 0.0f, 1900.0f, 800.0f,
              0xFFFFFFFF);
            D(m_pak, m_batch, 1876, 780.0f, -2.0f, 317.0f, 215.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 1866, 1874.0f, 6.0f, 339.0f, 800.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 1472, 0.0f, 0.0f, 339.0f, 800.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 1464, 1465.0f, 448.0f, 51.0f, 56.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 39, 48.0f, 46.0f, 1175.0f, 630.0f, 0xFFFFFFFF);      // the room
            break;
        }
        case SLGame::SCR_CUSTDEATH:                  // frame 32
            D(m_pak, m_batch, 1491, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
            break;
        case SLGame::SCR_CUSTWIN: {                  // frame 33
            static const int kFx[45] = { 873 };
            for (int i = 0; i < 45; ++i) {
                static const f32 kX[45] = { 179,291,403,515,627,739,851,963,1105,
                                             205,317,429,541,653,765,877,989,1101,
                                             179,291,403,515,627,739,851,963,1075,
                                             133,233,345,457,569,681,793,905,1017,
                                             219,331,443,555,667,779,891,1003,1115 };
                static const f32 kY[45] = { 113,113,113,113,113,113,113,113,181,
                                             255,255,255,255,255,255,255,255,255,
                                             337,337,337,337,337,337,337,337,337,
                                             191,173,173,173,173,173,173,173,173,
                                             397,397,397,397,397,397,397,397,397 };
                DN(m_pak, m_batch, kFx[0], kX[i], kY[i], 0xFFFFFFFF);
            }
            DN(m_pak, m_batch, 1223, 441.0f, 228.0f, 0xFFFFFFFF);
            DN(m_pak, m_batch, 1189, 501.0f, 328.0f, 0xFFFFFFFF);
            break;
        }
        case SLGame::SCR_CUST8BIT:                   // frame 34
            D(m_pak, m_batch, 3254, 0.0f, 0.0f, 600.0f, 300.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 3279, 0.0f, 0.0f, 250.0f, 40.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 3250, 1116.0f, 289.0f, 46.0f, 226.0f, 0xFFFFFFFF);  // peek 1
            D(m_pak, m_batch, 3251, 1470.0f, 316.0f, 54.0f, 238.0f, 0xFFFFFFFF);  // peek 2
            break;
        case SLGame::SCR_FINALCS:                    // frame 35
            D(m_pak, m_batch, 2799, 1076.0f, 10.0f, 1280.0f, 720.0f, 0xFFFFFFFF);
            D(m_pak, m_batch, 1585, 694.0f, 634.0f, 32.0f, 32.0f, 0xFFFFFFFF);
            break;
        default:
            break;
    }
}

void SLRender::RenderPS(const SLGame& game) {
    // v2.73: the room art from the dump's frame_11 layout — img 455
    // (1700x1500 @ (0,32), the P&S hall with Bonnie's face), drawn 1:1
    // (the window shows the face area; the module at y=896 is below it).
    // The five face buttons: 447 (10x10 dot) at the four cheeks/eye/nose
    // spots + 463 (60x60) chin — layout coords, current pick highlighted.
    // The dump's button anims are single-cell; the lit-state tint is ours
    // (DEVIATION). Picks 5-8 (module/units) stay text — their plate sits
    // off-window.
    static const struct { int img; f32 x, y; } kPlates[5] = {
        { 447,  816.0f, 530.0f },   // right cheek
        { 447, 1128.0f, 538.0f },   // left cheek
        { 447,  969.0f, 381.0f },   // right eye
        { 447,  989.0f, 451.0f },   // nose
        { 463,  900.0f, 632.0f },   // chin
    };
    if (m_batch) {
        D(m_pak, m_batch, 455, 0.0f, 0.0f, 1700.0f, 1500.0f, 0xFFFFFFFF);
        for (int i = 0; i < 5; ++i) {
            const u32 col = (game.GetPick() == i) ? 0xFFFFFFFF : 0xFF909090;
            DN(m_pak, m_batch, kPlates[i].img, kPlates[i].x, kPlates[i].y, col);
        }
        if (game.GetPick() == 5)   // the module plate, pinned to the window's bottom edge
            DN(m_pak, m_batch, 619, 990.0f, 660.0f, 0xFFFFFFFF);
    }
    if (m_text) {
        char b[80];
        Snprintf(b, sizeof(b), "PARTS AND SERVICE   event %d", game.GetScriptEvent());
        m_text->DrawText(40, 40, b, 0xFFE0E0E0);
        static const char* const kSub[4] = { "MODULE", "UNIT 1", "UNIT 2", "-" };
        for (int i = 0; i < 4; ++i) {
            const u32 col = (game.GetPick() == i + 5) ? 0xFFFFFFFF : 0xFF808080;
            m_text->DrawText(120 + (i32)i * 340, 610, kSub[i], col);
        }
        m_text->DrawText(40, 660, "L/R: pick   A: click   B: exit (event 168)", 0xFF909090);
    }
}

void SLRender::RenderDesk(const SLGame& game) {
    // Under Desk (frame 13): the dark under-desk hold; the eye-match fill
    if (m_text) {
        char b[64];
        Snprintf(b, sizeof(b), "UNDER DESK   hold %.1f", game.GetDeskHold());
        m_text->DrawText(40, 40, b, 0xFFE0E0E0);
        m_text->DrawText(40, 660, "A(hold): keep the eye-match", 0xFF909090);
    }
}

void SLRender::RenderChain(const SLGame& game) {
    // v2.74: the chain screens get their own art (frame 20 scooping room,
    // 21 red fade, 22 bathroom, 23 credits) instead of a caption on black.
    if (m_batch) {
        const f32 t = game.GetTime();
        switch (game.GetScreen()) {
            case SLGame::SCR_SCOOPING: {
                static const int kRoom[4] = { 1328, 1329, 1330, 1331 };
                D(m_pak, m_batch, kRoom[(int)(t * 4.0f) % 4], 0.0f, 0.0f, 1900.0f, 800.0f,
                  0xFFFFFFFF);
                D(m_pak, m_batch, 1404, 952.0f, 64.0f, 333.0f, 400.0f, 0xFFFFFFFF); // Ennard
                D(m_pak, m_batch, 1403, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);  // drops
                D(m_pak, m_batch, 1402, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);  // haze
                break;
            }
            case SLGame::SCR_REDFADE:
                D(m_pak, m_batch, 1437, 0.0f, 0.0f, 1280.0f, 720.0f, 0xFFFFFFFF);  // fade to red
                D(m_pak, m_batch, 1401, 642.0f, 355.0f, 25.0f, 27.0f, 0xFFFFFFFF);
                break;
            case SLGame::SCR_BATHROOM:
                D(m_pak, m_batch, 451, 448.0f, 0.0f, 2000.0f, 720.0f, 0xFFFFFFFF);
                D(m_pak, m_batch, 384, 0.0f, 0.0f, 2000.0f, 720.0f, 0xFFFFFFFF);
                D(m_pak, m_batch, 383, 0.0f, 0.0f, 2000.0f, 720.0f, 0xFFFFFFFF);
                D(m_pak, m_batch, 36, 656.0f, -64.0f, 32.0f, 32.0f, 0xFFFFFFFF);  // mouse hide
                break;
            case SLGame::SCR_CREDITS: {
                static const int kEnd[6] = { 1496, 1495, 1496, 1466, 1498, 1468 };
                D(m_pak, m_batch, kEnd[(int)(t * 3.0f) % 6], 470.0f, 220.0f, 320.0f, 200.0f,
                  0xFFFFFFFF);                                                   // "the end"
                D(m_pak, m_batch, 1313, 30.0f, 610.0f, 448.0f, 77.0f, 0xFFFFFFFF); // good ending
                D(m_pak, m_batch, 1481, 30.0f, 610.0f, 454.0f, 77.0f, 0xFFFFFFFF); // bad ending
                // the credits scroll (a 189x2451 strip) drifting down
                D(m_pak, m_batch, 640, 638.0f, 300.0f + (f32)((int)t % 30) * 60.0f,
                  189.0f, 2451.0f, 0xFFFFFFFF);
                break;
            }
            default: break;
        }
    }
    if (m_text) {
        const char* cap = "SCOOPING ROOM";
        if (game.GetScreen() == SLGame::SCR_REDFADE)  cap = "RED FADE";
        if (game.GetScreen() == SLGame::SCR_BATHROOM) cap = "THE BATHROOM";
        if (game.GetScreen() == SLGame::SCR_CREDITS)  cap = "CREDITS";
        m_text->DrawText(480, 320, cap, 0xFF606060);
        m_text->DrawText(440, 420, "A/B: continue", 0xFF606060);
    }
}

void SLRender::RenderExtrasMenu(const SLGame& game) {
    // v2.74: the extras menu is the dump's own art (frame 18): the row plates
    // ANIMATRONICS (1169) / MAKING BABY (1170) / MAKING FREDDY (1171) /
    // MAKING FOXY (1173) / BLUEPRINT (1235) at x=147, plus the full-width
    // backing strips and the header. The invented CHARACTERS/JUMPSCARES/...
    // text rows are gone. Labeled: the lit state is our tint (the dump's row
    // anims are single-cell).
    if (m_batch) {
        static const int kRows[5] = { 1169, 1170, 1171, 1173, 1235 };
        static const f32 kRowY[5] = { 73.0f, 123.0f, 173.0f, 234.0f, 286.0f };
        // the backing strips (layer 0) + the header
        DN(m_pak, m_batch, 393, 0.0f, 10.0f, 0xFFFFFFFF);
        DN(m_pak, m_batch, 354, 0.0f, 150.0f, 0xFFFFFFFF);
        DN(m_pak, m_batch, 352, 0.0f, 209.0f, 0xFFFFFFFF);
        DN(m_pak, m_batch, 355, 0.0f, 434.0f, 0xFFFFFFFF);
        DN(m_pak, m_batch, 389, 0.0f, 484.0f, 0xFFFFFFFF);
        DN(m_pak, m_batch, 397, 0.0f, 653.0f, 0xFFFFFFFF);
        DN(m_pak, m_batch, 1172, -72.0f, 84.0f, 0xFFFFFFFF);
        for (int i = 0; i < 5; ++i) {
            const u32 col = (game.GetOptionSelected() == i) ? 0xFFFFFFFF : 0xFF909090;
            DN(m_pak, m_batch, kRows[i], 147.0f, kRowY[i], col);
        }
        DN(m_pak, m_batch, 1236, 523.0f, 341.0f, 0xFFFFFFFF);   // left arrow
        DN(m_pak, m_batch, 1237, 1187.0f, 341.0f, 0xFFFFFFFF);  // right arrow
    }
}

} // namespace fnaf
