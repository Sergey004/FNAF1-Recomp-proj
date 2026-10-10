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
    batch->Draw(t->texture, x, y, w, h, color);
}

void SLRender::RenderWarning() {
    // frame 0: the legal splash (the dump's warning cell run 414-417)
    if (m_batch) {
        static f32 s_t = 0.0f;
        s_t += 1.0f / 60.0f;
        D(m_pak, m_batch, 414 + ((int)(s_t * 4.0f) % 4), 0.0f, 0.0f,
          1280.0f, 720.0f, 0xFFFFFFFF);
    }
    if (m_text) {
        m_text->DrawText(430, 620, "WARNING - press A", 0xFFC0C0C0);
    }
}

void SLRender::RenderTitle(const SLGame& game, f32 time) {
    // frame 1: the title scene (anim cell by the dump table) + the menu
    if (m_batch) {
        static const int kBg[4] = { 100, 267, 268, 269 };
        D(m_pak, m_batch, kBg[(int)(time * 4.0f) % 4], 0.0f, 0.0f,
          1280.0f, 720.0f, 0xFFFFFFFF);
    }
    static const char* kRows[4] = { "NEW GAME", "CONTINUE", "EXTRAS", "CUSTOM NIGHT" };
    if (m_text) {
        for (int i = 0; i < 4; ++i) {
            const bool lock = (i == 2 && !game.IsBeat1()) ||
                              (i == 3 && !game.IsBeat3());
            u32 col = (game.GetOptionSelected() == i) ? 0xFFFFFFFF : 0xFF909090;
            if (lock) col = 0xFF404040;
            m_text->DrawText(80, 300 + i * 56, kRows[i], col);
        }
    }
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
    (void)game;
    if (m_text) {
        m_text->DrawText(40, 40, "BABY'S GALLERY", 0xFFE0E0E0);
        m_text->DrawText(40, 660, "B: end the night", 0xFF909090);
    }
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
    (void)game;
    if (m_text) {
        m_text->DrawText(40, 40, "BREAKER ROOM", 0xFFE0E0E0);
        m_text->DrawText(40, 660, "hold on while the task sits (wave 1)", 0xFF909090);
    }
}

void SLRender::RenderWinNight(const SLGame& game) {
    if (m_text) {
        char b[48];
        Snprintf(b, sizeof(b), "NIGHT %d complete", game.GetNight());
        m_text->DrawText(480, 320, b, 0xFFFFFFFF);
    }
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
    if (m_text) {
        m_text->DrawText(480, 320, "G A M E   O V E R", 0xFFC0C0C0);
        m_text->DrawText(440, 420, "Press A to retry the night", 0xFF909090);
    }
}

void SLRender::RenderGameOver() {
    if (m_text)
        m_text->DrawText(480, 320, ". . .", 0xFFC0C0C0);
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
