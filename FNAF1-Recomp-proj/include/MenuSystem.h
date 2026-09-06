/**
 * Five Nights at Freddy's 1 — Recompilation
 * MenuSystem.h: Main menu with navigation and placeholder screens
 *
 * Xbox 360: XInput + D3D9 TextRenderer
 * PC: GetAsyncKeyState + printf fallback
 */

#ifndef FNAF_MENU_SYSTEM_H
#define FNAF_MENU_SYSTEM_H

#include "Types.h"

namespace fnaf {

class TextRenderer;

// ============================================================
//  Menu screens
// ============================================================
enum MenuScreen {
    MENU_MAIN = 0,
    MENU_NIGHT_SELECT,
    MENU_EXTRAS,
    MENU_OPTIONS
};

enum MainMenuOption {
    MENU_OPT_NEW_GAME = 0,
    MENU_OPT_CONTINUE,
    MENU_OPT_NIGHT_SELECT,
    MENU_OPT_EXTRAS,
    MENU_OPT_OPTIONS,
    MENU_OPT_EXIT,
    MENU_OPT_DEV,        // v2.17: hidden debug/dev entry (no text on screen)
    MENU_OPT_COUNT
};

// ============================================================
//  Input snapshot for menu (edge-triggered)
// ============================================================
struct MenuInput {
    bool up;
    bool down;
    bool left;
    bool right;
    bool confirm; // A / START / Enter
    bool back;    // B / ESC
    MenuInput() : up(false), down(false), left(false), right(false), confirm(false), back(false) {}
};

// Result after Update
enum MenuAction {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_START_NIGHT, // start selected night
    MENU_ACTION_OPEN_DEV,    // v2.17: hidden debug/dev menu entry
    MENU_ACTION_EXIT
};

// ============================================================
//  MenuSystem
// ============================================================
class MenuSystem {
public:
    MenuSystem();

    void Init(i32 unlockedNight = 1, i32 lastCompletedNight = 0);
    void Reset();

    // Feed input each tick (should be edge-triggered, caller handles debounce)
    MenuAction Update(const MenuInput& input);

    // Render current screen via TextRenderer (Xbox) or printf (PC)
    void Render(TextRenderer* renderer, i32 screenW, i32 screenH) const;

    // Queries
    i32 GetSelectedNight() const { return m_selectedNight; }
    MenuScreen GetScreen() const { return m_screen; }
    i32 GetMainSelection() const { return m_mainSelection; }

    // For save emulation (placeholder)
    void SetUnlockedNight(i32 night) { m_unlockedNight = night; if(m_unlockedNight<1) m_unlockedNight=1; if(m_unlockedNight>7) m_unlockedNight=7; }
    void SetHasSave(bool has) { m_hasSave = has; }
    i32  GetUnlockedNight() const { return m_unlockedNight; }
    bool HasSave() const { return m_hasSave; }
    // v2.7.13: true when the last START_NIGHT came from "New Game"
    bool LastStartWasNewGame() const { return m_lastStartWasNewGame; }

private:
    MenuScreen m_screen;
    i32 m_mainSelection;      // 0 .. MENU_OPT_COUNT-1
    i32 m_nightSelection;     // 1 .. 7
    i32 m_selectedNight;      // night to start when ACTION_START_NIGHT
    i32 m_unlockedNight;      // max selectable night (1..7)
    i32 m_lastCompletedNight;
    bool m_hasSave;
    bool m_lastStartWasNewGame;   // v2.7.13: last START_NIGHT was "New Game" 

    // Helpers
    void MoveMainSelection(int dir);
    void MoveNightSelection(int dir);
    const char* GetMainOptionLabel(int idx) const;
    bool IsMainOptionEnabled(int idx) const;
};

} // namespace fnaf

#endif // FNAF_MENU_SYSTEM_H
