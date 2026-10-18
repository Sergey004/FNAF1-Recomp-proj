/**
 * SLRender.cpp: v2.65 — the Sister Location renderer (wave 1).
 * The window is 1280x720 native (no scale). Room art rides the dump's own
 * Backdrop/Active cells (the wider rooms pan); the text captions are the
 * debug font until the gfx pinboards land in wave 2 (labeled).
 */

#include "SLRender.h"
#include "SLGame.h"
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

void SLRender::RenderWarning() {
    // frame 0: the legal splash (the dump's warning cell run 414-417).
    // The dump frame has NO hint caption — the old "press A" debug line is
    // gone (img 366 is the thin rule under the legal block, drawn at its
    // layout point 550,687).
    if (m_batch) {
        static f32 s_t = 0.0f;
        s_t += 1.0f / 60.0f;
        D(m_pak, m_batch, 414 + ((int)(s_t * 4.0f) % 4), 0.0f, 0.0f,
          1280.0f, 720.0f, 0xFFFFFFFF);
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
    // frame 2: the cabin cells pan over the 1900x1000 room
    if (m_batch) {
        static const int kCab[4] = { 819, 810, 823, 813 };
        const int cell = kCab[(int)(time * 2.0f) % 4];
        D(m_pak, m_batch, cell, -game.GetPan(), 0.0f, 1900.0f, 1000.0f * (720.0f / 1000.0f),
          0xFFFFFFFF);
    }
    if (m_text) {
        char b[64];
        Snprintf(b, sizeof(b), "NIGHT %d — the ride", game.GetNight());
        m_text->DrawText(40, 40, b, 0xFF909090);
        m_text->DrawText(40, 680, "A: skip the ride", 0xFF808080);
    }
}

void SLRender::RenderVent(const SLGame& game) {
    // frame 3: the vent interior (the dark cells), the notch meter
    if (m_batch) {
        static const int kV[4] = { 18, 50, 73, 75 };
        const f32 x = -(f32)((600.0f * (game.GetNotch() / 10.0f)));
        D(m_pak, m_batch, kV[game.GetNotch() % 4], x, 0.0f, 1700.0f, 720.0f, 0xFFFFFFFF);
    }
    if (m_text)
        m_text->DrawText(40, 40, "Hold W to crawl", 0xFF909090);
}

void SLRender::RenderHub(const SLGame& game) {
    // frame 5 "Main Hub": the room art = img 2085 (1900x1000 @0,0 — the
    // hub's own anim cell; the wide room pans with the camera). The old
    // draw stretched the spark sprite (56) over the screen.
    (void)game;
    if (m_batch)
        D(m_pak, m_batch, 2085, -game.GetPan() * 0.3f, -80.0f, 1900.0f, 1000.0f,
          0xFFFFFFFF);
    if (m_text) {
        m_text->DrawText(40, 40, "CIRCUS CONTROL", 0xFFE0E0E0);
        m_text->DrawText(40, 660, "A: the day route   D: shock test", 0xFF909090);
    }
}

void SLRender::RenderBaby(const SLGame& game) {
    // v2.73: the room art from the dump's frame_6 layout — img 829
    // (1700x1000 @ (-1,-1), Baby's gallery), the rooms' height-squash
    // convention (1000 -> 720). The "go back" plate 182 is parked
    // off-screen in the layout (event-gated) — not drawn.
    (void)game;
    if (m_batch)
        D(m_pak, m_batch, 829, 0.0f, 0.0f, 1700.0f, 720.0f, 0xFFFFFFFF);
    if (m_text)
        m_text->DrawText(40, 660, "B: end the night", 0xFF909090);
}

void SLRender::RenderBallora(const SLGame& game, f32 time) {
    if (m_batch) {
        static const int kFloor[4] = { 1955, 1954, 1261, 1956 };
        static const int kDoor[4] = { 194, 195, 196, 197 };
        const f32 pan = game.GetProgress() * (720.0f / 850.0f) * (2200.0f - 1280.0f);
        D(m_pak, m_batch, kFloor[(int)(time * 4.0f) % 4], -pan, 0.0f, 2200.0f, 720.0f,
          0xFFFFFFFF);
        D(m_pak, m_batch, kDoor[(int)(time * 4.0f) % 4], 1037.0f - pan, 300.0f,
          120.0f, 200.0f, 0xFFFFFFFF);
        // Ballora herself appears when the dance starts (progress 400)
        if (game.GetDistance() > 0) {
            static const int kBal[4] = { 1955, 1956, 1954, 1261 };
            D(m_pak, m_batch, kBal[0], 640.0f + (f32)game.GetLeftPan() * 3.0f, 200.0f,
              200.0f, 330.0f, 0xFFFFFFFF);
        }
    }
    if (m_text) {
        char b[64];
        Snprintf(b, sizeof(b), "HALL %d / danger %d", game.GetProgress(), game.GetDistance());
        m_text->DrawText(40, 40, b, 0xFF909090);
        m_text->DrawText(40, 660, "Hold W: walk   Shift: quick   (walk = noise)", 0xFF909090);
    }
}

void SLRender::RenderFuntime(const SLGame& game, f32 time) {
    if (m_batch) {
        static const int kFloor[4] = { 261, 252, 288, 253 };
        const int thw[3] = { 80, 120, 160 };
        D(m_pak, m_batch, kFloor[(int)(time * 4.0f) % 4], -game.GetProgress() * 0.4f, 0.0f,
          1900.0f, 720.0f, 0xFFFFFFFF);
        // the Foxy silhouettes by distance bands (hidden > 1300 progress)
        if (game.GetFoxyDist() > 100 && game.GetProgress() < 1300) {
            static const int kSil[3] = { 279, 252, 261 };
            const int s = game.GetFoxyDist() > 400 ? 2 : (game.GetFoxyDist() > 200 ? 1 : 0);
            D(m_pak, m_batch, kSil[s], (f32)(500 + thw[s] + 0.0f), 220.0f,
              260.0f, 420.0f, 0xFFFFFFFF);
        }
    }
    if (m_text) {
        char b[80];
        Snprintf(b, sizeof(b), "HALL %d   energy %.0f   danger %d",
                 game.GetProgress(), game.GetFlashCharge(), game.GetFoxyDist());
        m_text->DrawText(40, 40, b, 0xFF909090);
        m_text->DrawText(40, 660, "W: walk   X: flash (attracts!)", 0xFF909090);
    }
}

void SLRender::RenderBreaker(const SLGame& game) {
    // v2.73: the room art from the dump's frame_9 layout — img 594
    // (1900x1000 @ (0,0), the dark breaker hall with the Funtime silhouettes),
    // drawn with the rooms' height-squash convention (1000 -> 720). The task
    // label 579 "jack in" (606x55 @ (310,930), layer 4) is the REST-state
    // strip the frame replay proves visible — with 1889 "flip it up"
    // stacked on it (the replay shows the magenta plate + the chevrons);
    // 360/580 are the task-beat prompts (event-gated, not yet drawn).
    (void)game;
    if (m_batch) {
        D(m_pak, m_batch, 594, 0.0f, 0.0f, 1900.0f, 720.0f, 0xFFFFFFFF);
        const f32 stripY = 930.0f * (720.0f / 1000.0f);
        DN(m_pak, m_batch, 579, 310.0f, stripY, 0xFFFFFFFF);
        DN(m_pak, m_batch, 1889, 340.0f, stripY, 0xFFFFFFFF);
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

void SLRender::RenderTvShow() {
    if (m_text)
        m_text->DrawText(460, 340, "the tv show plays", 0xFF909090);
}

void SLRender::RenderGirlVoice() {
    if (m_text)
        m_text->DrawText(440, 340, "... she's still there", 0xFFE0E0E0);
}

void SLRender::RenderDeath() {
    // v2.73: the dump's frame_15 rest state is a BLACK screen (the frame
    // replay proves it — the GAME OVER art is event-gated). The invented
    // big caption is gone; the retry hint stays as labeled pad-only UI.
    if (m_text)
        m_text->DrawText(440, 420, "Press A to retry the night", 0xFF606060);
}

void SLRender::RenderGameOver() {
    // v2.73: black, per the frame_15 rest state (was an invented ". . .").
}

void SLRender::RenderHold(const char* caption) {
    if (m_text)
        m_text->DrawText(440, 340, caption ? caption : "the wave-2 room", 0xFF909090);
}

void SLRender::RenderPS(const SLGame& game) {
    // the face-button task (frames 11/19): the nine picks with the current
    // highlight, the Script Event beat, the exit hint. The dump's button
    // art rows are not frame-dumped whole — labeled block labels stand in.
    static const char* const kBtn[9] = {
        "R.CHEEK", "L.CHEEK", "R.EYE", "NOSE", "CHIN", "MODULE", "UNIT 1", "UNIT 2", "-"
    };
    if (m_text) {
        char b[80];
        Snprintf(b, sizeof(b), "PARTS AND SERVICE   event %d", game.GetScriptEvent());
        m_text->DrawText(40, 40, b, 0xFFE0E0E0);
        for (int i = 0; i < 9; ++i) {
            const u32 col = (i == game.GetPick()) ? 0xFFFFFFFF : 0xFF808080;
            m_text->DrawText(120 + (i % 3) * 340, 420 + (i / 3) * 60, kBtn[i], col);
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
    // the night-4/5 end chain (frames 20/21/22/23)
    if (m_text) {
        const char* cap = "SCOOPING ROOM";
        if (game.GetScreen() == SLGame::SCR_REDFADE)  cap = "RED FADE";
        if (game.GetScreen() == SLGame::SCR_BATHROOM) cap = "THE BATHROOM";
        if (game.GetScreen() == SLGame::SCR_CREDITS)  cap = "CREDITS";
        m_text->DrawText(480, 320, cap, 0xFFE0E0E0);
        m_text->DrawText(440, 420, "A/B: continue", 0xFF909090);
    }
}

void SLRender::RenderExtrasMenu(const SLGame& game) {
    static const char* const kRows[4] = { "CHARACTERS", "JUMPSCARES", "SCENES", "EXIT" };
    if (m_text) {
        for (int i = 0; i < 4; ++i) {
            m_text->DrawText(420, 240 + i * 64, kRows[i],
                             (game.GetOptionSelected() == i) ? 0xFFFFFFFF : 0xFF909090);
        }
    }
}

} // namespace fnaf
