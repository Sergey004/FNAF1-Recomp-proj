/**
 * Five Nights at Freddy's 1 — Recompilation
 * MenuSystem.cpp: Main menu logic + rendering
 */

#include "MenuSystem.h"
#include "TextRenderer.h"
#include <cstdio>
#include <cstring>

namespace fnaf {

MenuSystem::MenuSystem()
    : m_screen(MENU_MAIN)
    , m_mainSelection(0)
    , m_nightSelection(1)
    , m_selectedNight(1)
    , m_unlockedNight(1)
    , m_lastCompletedNight(0)
    , m_hasSave(false)
{
}

void MenuSystem::Init(i32 unlockedNight, i32 lastCompletedNight)
{
    m_unlockedNight = unlockedNight;
    if (m_unlockedNight < 1) m_unlockedNight = 1;
    if (m_unlockedNight > 7) m_unlockedNight = 7;
    m_lastCompletedNight = lastCompletedNight;
    m_hasSave = (lastCompletedNight > 0);
    m_screen = MENU_MAIN;
    m_mainSelection = 0;
    m_nightSelection = 1;
    m_selectedNight = 1;
}

void MenuSystem::Reset()
{
    m_screen = MENU_MAIN;
    m_mainSelection = 0;
    m_nightSelection = 1;
}

const char* MenuSystem::GetMainOptionLabel(int idx) const
{
    switch (idx) {
        case MENU_OPT_NEW_GAME:     return "New Game";
        case MENU_OPT_CONTINUE:     return "Continue";
        case MENU_OPT_NIGHT_SELECT: return "Select Night";
        case MENU_OPT_EXTRAS:       return "Extras";
        case MENU_OPT_OPTIONS:      return "Options";
        case MENU_OPT_EXIT:         return "Exit";
        default:                    return "???";
    }
}

bool MenuSystem::IsMainOptionEnabled(int idx) const
{
    if (idx == MENU_OPT_CONTINUE) return m_hasSave;
    return true;
}

void MenuSystem::MoveMainSelection(int dir)
{
    // dir: -1 up, +1 down, skip disabled
    for (int i = 0; i < MENU_OPT_COUNT; ++i) {
        m_mainSelection += dir;
        if (m_mainSelection < 0) m_mainSelection = MENU_OPT_COUNT - 1;
        if (m_mainSelection >= MENU_OPT_COUNT) m_mainSelection = 0;
        if (IsMainOptionEnabled(m_mainSelection)) break;
    }
}

void MenuSystem::MoveNightSelection(int dir)
{
    m_nightSelection += dir;
    if (m_nightSelection < 1) m_nightSelection = m_unlockedNight;
    if (m_nightSelection > m_unlockedNight) m_nightSelection = 1;
}

MenuAction MenuSystem::Update(const MenuInput& in)
{
    if (m_screen == MENU_MAIN) {
        if (in.up) MoveMainSelection(-1);
        else if (in.down) MoveMainSelection(1);

        if (in.confirm) {
            switch (m_mainSelection) {
                case MENU_OPT_NEW_GAME:
                    m_selectedNight = 1;
                    return MENU_ACTION_START_NIGHT;
                case MENU_OPT_CONTINUE:
                    if (m_hasSave) {
                        // continue from next night
                        m_selectedNight = m_lastCompletedNight + 1;
                        if (m_selectedNight > 7) m_selectedNight = 7;
                        if (m_selectedNight < 1) m_selectedNight = 1;
                        return MENU_ACTION_START_NIGHT;
                    }
                    break;
                case MENU_OPT_NIGHT_SELECT:
                    m_screen = MENU_NIGHT_SELECT;
                    m_nightSelection = 1;
                    break;
                case MENU_OPT_EXTRAS:
                    m_screen = MENU_EXTRAS;
                    break;
                case MENU_OPT_OPTIONS:
                    m_screen = MENU_OPTIONS;
                    break;
                case MENU_OPT_EXIT:
                    return MENU_ACTION_EXIT;
                default: break;
            }
        }
        // back does nothing on main
    }
    else if (m_screen == MENU_NIGHT_SELECT) {
        if (in.up) MoveNightSelection(-1);
        else if (in.down) MoveNightSelection(1);
        else if (in.left) MoveNightSelection(-1);
        else if (in.right) MoveNightSelection(1);

        if (in.confirm) {
            m_selectedNight = m_nightSelection;
            return MENU_ACTION_START_NIGHT;
        }
        if (in.back) {
            m_screen = MENU_MAIN;
        }
    }
    else if (m_screen == MENU_EXTRAS || m_screen == MENU_OPTIONS) {
        if (in.back || in.confirm) {
            m_screen = MENU_MAIN;
        }
    }
    return MENU_ACTION_NONE;
}

void MenuSystem::Render(TextRenderer* renderer, i32 screenW, i32 screenH) const
{
    if (!renderer || !renderer->IsInitialized()) return;

    if (m_screen == MENU_MAIN) {
        const i32 x = 120;
        // Title like original game
        renderer->DrawText(x, 100, "Five", TextColor::WHITE);
        renderer->DrawText(x, 135, "Nights", TextColor::WHITE);
        renderer->DrawText(x, 170, "at", TextColor::WHITE);
        renderer->DrawText(x, 205, "Freddy's", TextColor::WHITE);
        // v2.7.4: the old stub "Demo" text here is removed -- the running
        // title is the pak-art GameRender::RenderTitle; this stub block is
        // not even reached for MENU_MAIN (main.cpp skips it)

        // Options left aligned
        const i32 startY = 340;
        const i32 lineH = 40;
        // Show only New Game and Continue like original; others hidden
        struct Opt { int id; const char* label; };
        Opt opts[] = {
            { MENU_OPT_NEW_GAME, "New Game" },
            { MENU_OPT_CONTINUE, "Continue" }
        };
        for (int i = 0; i < 2; ++i) {
            int id = opts[i].id;
            bool selected = (m_mainSelection == id);
            bool enabled = IsMainOptionEnabled(id);
            u32 color = enabled ? (selected ? TextColor::WHITE : TextColor::DIM_WHITE) : TextColor::DARK_GRAY;
            char buf[64];
            if (selected) {
                sprintf(buf, ">> %s", opts[i].label);
            } else {
                sprintf(buf, "   %s", opts[i].label);
            }
            renderer->DrawText(x, startY + i * lineH, buf, color);
            // Continue subtitle
            if (id == MENU_OPT_CONTINUE && enabled && selected) {
                char sub[32];
                sprintf(sub, "Night %d", m_lastCompletedNight + 1);
                renderer->DrawText(x + 50, startY + i * lineH + 22, sub, TextColor::DIM_WHITE);
            }
        }

        // Version / copyright like original
        renderer->DrawText(20, screenH - 30, "v 1.13", TextColor::GRAY);
        char cr[64];
        sprintf(cr, "©2014 Scott Cawthon");
        renderer->DrawText(screenW - 300, screenH - 30, cr, TextColor::GRAY);
        return;
    }

    // common title for other screens
    renderer->DrawTextCentered(120, "FIVE NIGHTS AT FREDDY'S", TextColor::WHITE, screenW);
    renderer->DrawTextCentered(150, "Recompilation Build", TextColor::GRAY, screenW);
    renderer->DrawTextCentered(170, "Xbox 360", TextColor::DARK_GRAY, screenW);

    if (m_screen == MENU_MAIN) {
    } else if (m_screen == MENU_NIGHT_SELECT) {
        renderer->DrawTextCentered(220, "SELECT NIGHT", TextColor::YELLOW, screenW);
        // Grid 1-7
        const i32 startY = 280;
        for (int n = 1; n <= 7; ++n) {
            bool sel = (n == m_nightSelection);
            bool unlocked = (n <= m_unlockedNight);
            u32 col;
            if (!unlocked) col = TextColor::DARK_GRAY;
            else if (sel) col = TextColor::YELLOW;
            else col = TextColor::WHITE;
            char buf[32];
            if (sel) sprintf(buf, "> NIGHT %d <", n);
            else sprintf(buf, "  NIGHT %d  ", n);
            if (!unlocked) sprintf(buf, "  NIGHT %d [LOCKED]  ", n);
            renderer->DrawTextCentered(startY + (n-1)*32, buf, col, screenW);
        }
        renderer->DrawTextCentered(screenH - 80, "A : Start   B : Back", TextColor::DARK_GRAY, screenW);
    }
    else if (m_screen == MENU_EXTRAS) {
        renderer->DrawTextCentered(240, "EXTRAS", TextColor::YELLOW, screenW);
        renderer->DrawTextCentered(300, "[ ZAGLUSHKA ]", TextColor::DIM_WHITE, screenW);
        renderer->DrawTextCentered(330, "Jumpscares / Gallery / Animations", TextColor::GRAY, screenW);
        renderer->DrawTextCentered(360, "Coming soon...", TextColor::DARK_GRAY, screenW);
        renderer->DrawTextCentered(420, "Assets ready: use ctf_extractor", TextColor::DARK_GRAY, screenW);
        renderer->DrawTextCentered(screenH - 80, "B / A : Back", TextColor::DARK_GRAY, screenW);
    }
    else if (m_screen == MENU_OPTIONS) {
        renderer->DrawTextCentered(240, "OPTIONS", TextColor::YELLOW, screenW);
        renderer->DrawTextCentered(300, "[ ZAGLUSHKA ]", TextColor::DIM_WHITE, screenW);
        renderer->DrawTextCentered(330, "Volume  Brightness  Controls", TextColor::GRAY, screenW);
        renderer->DrawTextCentered(360, "Coming soon...", TextColor::DARK_GRAY, screenW);
        renderer->DrawTextCentered(420, "Placeholder — no save yet", TextColor::DARK_GRAY, screenW);
        renderer->DrawTextCentered(screenH - 80, "B / A : Back", TextColor::DARK_GRAY, screenW);
    }
}

} // namespace fnaf
