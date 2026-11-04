/**
 * FNaFWorldGame.h: v2.90 — FNAF World module game logic.
 *
 * Clean-room port of the original FNAF World flow (dump: ctfak full dump of
 * fnaf-world.exe) with the FNAF-World-C-Edition source as the logic reference:
 * state machine, constants and timings mirror it 1:1 (title slide-ins 3/3/2 s,
 * world speed 2 tiles/s @ zoom 9, tile 50 px, zone ids 32/33/46/89), but every
 * system is re-written for our core: C++03 (VS2010 XDK), no std::string/vector,
 * screen-space pixels 1280x720, save through Progress (fnfw_save container).
 *
 * Engine-free like FNaF2Game/SLGame: input arrives as plain fields (the module
 * translates GameInput), audio runs through callbacks owned by the module.
 */

#ifndef FNAF_FNAFWORLD_GAME_H
#define FNAF_FNAFWORLD_GAME_H

#include "Types.h"
#include "Progress.h"
#include "ScreenFade.h"

namespace fnaf {

// ------------------------------------------------------------
// Input snapshot — translated from GameInput by the module.
// C-Edition input map (input.c/main.c): WASD = move, mouse = UI;
// ours: LS = move, A = confirm, B = back, Y = menu, D-pad = menus.
// ------------------------------------------------------------
struct FNaFWorldInputs {
    bool aPressed;        // A edge   — confirm / interact (C-Ed: mouse click)
    bool aHeld;           // A level
    bool bPressed;        // B edge   — back (C-Ed: Escape)
    bool yPressed;        // Y edge   — the world menu (exe: the menu button)
    bool menuPressed;     // START edge — the world menu too
    bool upPressed, downPressed, leftPressed, rightPressed;  // D-pad edges
    f32  lookDir;         // LS X (-1..1) — WASD A/D
    f32  lookDirY;        // LS Y (-1..1) — WASD W/S (+1 up)
    bool xHeld;           // X level — dash (C-Ed: Shift fast/loud)
    FNaFWorldInputs() : aPressed(false), aHeld(false), bPressed(false),
                        yPressed(false), menuPressed(false),
                        upPressed(false), downPressed(false),
                        leftPressed(false), rightPressed(false),
                        lookDir(0), lookDirY(0), xHeld(false) {}
};

// Audio hooks the module fills (all optional). Volumes 0..100 Clickteam-style.
struct FNaFWorldAudioHooks {
    void (*play)(const char* sample, bool loop, i32 channel, i32 volume);
    void (*stop)(const char* sample);
    void (*channelVolume)(i32 channel, i32 volume);
    FNaFWorldAudioHooks() : play(0), stop(0), channelVolume(0) {}
};

// ------------------------------------------------------------
// v2.91 step 3: the battle (Battle_Rework.c semantics).
// Battle space = 854x480 px virtual screen; entity positions use the
// C-Edition battle-space units (x*66.667 + 427 px), slots L_U..R_D.
// charId 0..3 = the party (Freddy/Bonnie/Chica/Foxy, same order as the
// overworld leader table); enemies carry their own name.
// ------------------------------------------------------------
struct FwBattleMember {
    i32  charId;        // 0..3 party index, -1 enemy
    i32  hp, maxHp;
    f32  cd;            // the attack cooldown remaining (s)
    bool dead;
    bool isEnemy;
    FwBattleMember() : charId(-1), hp(0), maxHp(0), cd(0), dead(false), isEnemy(false) {}
};

struct FwBattleState {
    FwBattleMember p[4];
    FwBattleMember e[4];
    i32  enemyCount;    // 1..4
    i32  result;        // 0 running, 1 victory, 2 defeat
    f32  endT;          // the result hold clock
    FwBattleState() : enemyCount(0), result(0), endT(0) {}
};

class FNaFWorldGame {
public:
    // Screens = the C-Edition GameStateTypes we implement, in boot order.
    enum Screen {
        SCR_DISCLAIMER = 0,   // main.c: the fan-recreation note (A -> title)
        SCR_TITLE      = 1,   // Title_Screen.c
        SCR_WORLD      = 2,   // World.c
        SCR_BATTLE     = 3,   // Battle_Rework.c (step 3)
        SCR_DIALOGUE   = 4,   // Dialogue.c (step 4)
        SCR_GAMEOVER   = 5,   // exe frame 6: defeat plate, 5 s -> title
        // v2.91: exe frame 3 "file setup" — the exe's own slot/new-game flow
        // (stage 0: 3 slots -> stage 1: new game / continue -> stage 2: the
        // YES/NO confirm -> the overworld)
        SCR_FILESETUP  = 6
    };

    FNaFWorldGame();

    void ResetToTitle();
    void Tick(f32 dt, const FNaFWorldInputs& in);

    // The CFT screen change (our core's shared model, same as F2/SL).
    void RequestScreen(i32 target);
    f32  GetFadeAlpha() const { return m_fade.Alpha(); }

    FNaFWorldAudioHooks audio;

    // v2.78-style streaming preload hook (the module wires it to
    // PakLoader::PreloadIndices; fnafworld.pak is ~437 MB -> streaming).
    void (*preloadFrame)(i32 frame);

    // ---- state for the renderer (screen-space px, 1280x720) ----
    Screen GetScreen() const { return m_screen; }
    f32    GetTime()   const { return m_time; }

    // Title (Title_Screen.c): three slide-ins, timings verbatim.
    f32 GetTitleT()     const { return m_titleT; }     // seconds since title
    i32 GetTitleChoice() const { return m_titleChoice; } // 0 = play (only row)
    // slide positions in DUMP space (800x480, frame 2 layout) — the renderer
    // scales x*1.6 / y*1.5 to 1280x720:
    f32 GetLogoLeft()  const { return -673.0f + m_logoShift; }   // g4 target 127
    f32 GetLogoTop()   const { return   10.0f; }
    f32 GetPartyTop()  const { return  480.0f - m_partyShift; }  // g3 target 64
    f32 GetPartyLeft() const { return   -2.0f; }
    f32 GetStartLeft() const { return  810.0f - m_startShift; }  // g6 target 260
    f32 GetStartTop()  const { return  409.0f; }
    bool TitleSlidesDone() const { return m_startShift >= 550.0f; }

    // World (World.c): tiles, camera, zones.
    f32 GetFreddyX()    const { return m_freddyX; }   // tile coords (float)
    f32 GetFreddyY()    const { return m_freddyY; }
    i32 GetFreddyDir()  const { return m_freddyDir; } // 0 idle 1 up 2 left 3 right 4 down
    f32 GetCamX()       const { return m_camX; }      // tile coords (camera centre)
    f32 GetCamY()       const { return m_camY; }
    f32 GetCamZoom()    const { return 9.0f; }        // World.c ResetWorld: zoom 9
    i32 GetZone()       const { return m_zone; }      // 0..3, 0xff = unknown
    i32 GetZoneLevel()  const { return m_zoneLevel; } // unlocked zones 1..3 (Save.c caps 3)
    u16 GetFazTokens()  const { return m_fazTokens; }

    // ---- v2.91 step 2: the exe overworld (frame 5, 6000x4000) ----
    // The exe 'area' counter value for the band Freddy stands in. Bands are
    // the quadrant layout of the dump map (minewalls south y>=2000, snow
    // north y<700, lake east x>=3000, woods west x<1500) — the exact
    // per-area markers are alterable values the ctfak JSON does not carry,
    // so this is labeled an approximation pending an event-level mine.
    i32 GetAreaId()   const { return m_areaId; }
    // exe Group 3..11: area -> the looping overworld theme ("travel1B" is
    // the Hills/Woods jingle-theme; the rest are named samples).
    static const char* AreaTheme(i32 areaId);
    static const char* AreaName(i32 areaId);    // the zone header text
    // The coin chests (exe "spawn chest" positions, FNaFWorldMap.h).
    i32 GetChestCount() const;
    // Position in TILE coords, centre; big = 25 tokens, else 5.
    void GetChest(i32 i, f32& tx, f32& ty, bool& big, bool& opened) const;
    // The party leader's overworld animation (one anim per character, all
    // front-facing): 0 Freddy, 12 Bonnie, 13 Chica, 14 Foxy — the exe
    // "character" object's animation numbers (application.json).
    i32 GetLeaderAnim() const { return m_party[0]; }   // the lead walks first
    f32 GetWalkT()      const { return m_walkT; }  // walk cycle clock, frames

    // ---- v2.91 step 3: battle (Battle_Rework.c) ----
    const FwBattleState& GetBattle() const { return m_battle; }
    // one floating damage number (renderer draws them), newest last
    struct FwDamage { f32 x, y; i32 amount; f32 born; bool enemyHit; };
    i32  GetDamageCount() const { return 8; }
    void GetDamage(i32 i, f32& x, f32& y, i32& amount, f32& age, bool& enemyHit) const;

    // ---- v2.91: the exe's file setup (frame 3) ----
    i32  GetFsStage()    const { return m_fsStage; }  // 0 slots / 1 new-cont / 2 YES-NO
    i32  GetFsSlot()     const { return m_fsSlot; }   // the selected slot 0..2
    i32  GetFsRow()      const { return m_fsRow; }    // stage 1/2 row
    bool GetFsSaveLive() const { return m_fsSaveLive; }
    i32  GetFsLevel()    const { return m_zoneLevel; }

    // ---- v2.91: the world menus (exe frames 4/7/12) ----
    // The exe models chips/bytes as INI arrays; the port keeps the same
    // shape: the party = four character anim ids, chips/bytes = bitmasks
    // of owned ids + per-member equipped chip.
    i32  GetParty(i32 i) const { return (i >= 0 && i < 4) ? m_party[i] : 0; }
    u32  GetChipsOwned() const { return m_chipsOwned; }
    i32  GetChipEq(i32 i) const {  // the member's equipped chip (-1 none)
        return (i >= 0 && i < 4) ? (i32)((m_chipsEq >> (i * 8)) & 0xFF) - 1 : -1;
    }
    u32  GetBytesOwned() const { return m_bytesOwned; }
    // menu state: 0 closed, 1 root, 2 party, 3 chips, 4 bytes
    i32  GetMenuScreen() const { return m_menuScreen; }
    i32  GetMenuRow()    const { return m_menuRow; }     // root row / grid cursor
    i32  GetMenuCol()    const { return m_menuCol; }     // grid column
    i32  GetMenuMember() const { return m_menuMember; }  // chips: the member row

    // the fnafworld save bridge (the module writes when this flips)
    void ApplyProgress(const Progress::GameProgressFW& p);
    void FillProgress(Progress::GameProgressFW& p) const;
    bool ConsumeSaveDirty() { const bool d = m_saveDirty; m_saveDirty = false; return d; }
    void WipeSave();     // the title X-hold wipe (2.62-convention)

    bool ExitRequested() const { return m_exitRequested; }

private:
    void TickDisclaimer(f32 dt, const FNaFWorldInputs& in);
    void TickTitle(f32 dt, const FNaFWorldInputs& in);
    void TickFileSetup(f32 dt, const FNaFWorldInputs& in);  // exe frame 3
    void TickWorld(f32 dt, const FNaFWorldInputs& in);
    void TickMenu(f32 dt, const FNaFWorldInputs& in);       // exe frames 4/7/12
    void TickBattle(f32 dt, const FNaFWorldInputs& in);  // step-3 placeholder
    void TickGameOver(f32 dt, const FNaFWorldInputs& in);   // exe frame 6
    void EnterArea(i32 areaId);              // theme switch + header reset
    void OpenChest(i32 i);
    bool CircleBlocked(f32 px, f32 py) const; // 4-corner hitbox vs the grid
    void Sfx(const char* s, bool loop, i32 ch, i32 vol);
    void SfxStop(const char* s);

    Screen m_screen;
    ScreenFade m_fade;         // our core's CFT transition (v2.76 model)
    f32    m_time;
    f32    m_cardT;            // per-screen timer
    bool   m_exitRequested;
    bool   m_saveDirty;

    // Title (Title_Screen.c): StartTime clock; the three elements slide for
    // TITLE_SECONDS_TO_CENTRE=3, PARTY=3, START=2. Our m_titleT = that clock.
    f32    m_titleT;
    i32    m_titleChoice;
    bool   m_titleThemeStarted;
    f32    m_xHoldT;           // the 360-convention X-hold wipe on the title
    f32    m_logoShift;        // g4: 0..800 (5/frame)
    f32    m_partyShift;       // g3: 0..416 (4/frame)
    f32    m_startShift;       // g6: 0..550 (5/frame)

    // World (World.c): Freddy 0.7x0.45 tiles @ scale 0.95, depth 2; speed 2
    // tiles/s; camera = Freddy centre, zoom 9; zone ids {32,33,46,89}.
    f32    m_freddyX, m_freddyY;    // tile coords (top-left of the hitbox)
    f32    m_freddyVX, m_freddyVY;
    i32    m_freddyDir;             // 0 idle, 1 up, 2 left, 3 right, 4 down
    f32    m_camX, m_camY;          // camera centre, tile coords
    i32    m_zone;
    i32    m_zoneLevel;
    u16    m_fazTokens;
    f32    m_stepSndT;              // the walk sound tick

    // v2.91 step 2 overworld state (exe frame 5):
    i32    m_areaId;                // the exe 'area' counter (0/3/5/8/11)
    f32    m_walkT;                 // walk cycle clock in frames (15 fps)
    f32    m_encT;                  // the wild-area encounter countdown
    u8     m_leaderAnim;            // the leader's anim number (0/12/13/14)
    bool   m_worldInit;             // collision grid decoded + spawn placed
    bool   m_themeOn;               // the area theme is currently playing

    // v2.91 step 3 battle state
    FwBattleState m_battle;
    bool   m_battleInit;            // populated on battle entry
    f32    m_dmgX[8];  f32 m_dmgY[8]; f32 m_dmgBorn[8];
    i32    m_dmgAmt[8]; bool m_dmgEnemy[8];
    i32    m_dmgNext;

    // v2.91 file-setup flow (exe frame 3)
    i32    m_fsStage;               // 0 slots, 1 new game/continue, 2 YES/NO
    i32    m_fsSlot;                // the picked slot 0..2
    i32    m_fsRow;                 // the row highlight on stages 1/2
    i32    m_fsPick;                // the stage-1 pick: 0 new game, 1 continue
    bool   m_fsSaveLive;            // the slot holds a game (the port's save)

    // v2.91 world menus (exe frames 4/7/12) + the inventory
    i32    m_party[4];              // the active four (character anim ids)
    u32    m_chipsOwned;            // bitmask: chest i -> chip i owned
    u32    m_chipsEq;               // 4 x 8-bit (chip id + 1; 0 = none)
    u32    m_bytesOwned;
    i32    m_menuScreen;            // 0 closed, 1 root, 2 party, 3 chips, 4 bytes
    i32    m_menuRow, m_menuCol, m_menuMember;

    // the fnafworld save (Progress::GameProgressFW keys, C-Edition Save.c)
    f32    m_lastX, m_lastY;        // Last_Location
    i32    m_lastZone;
};

} // namespace fnaf

#endif // FNAF_FNAFWORLD_GAME_H