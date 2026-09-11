/**
 * Five Nights at Freddy's 1 -- Recompilation
 * main.cpp: Xbox 360 — D3D9 render + XAudio2 sound + full game flow
 *
 * Boot flow (mirrors the original frame graph):
 *   DISCLAIMER (title frame String obj 0 + static) -> TITLE (bg img_431,
 *   buttons img_448/449, static cycle) -> NIGHT_START card -> OFFICE with
 *   Phone Guy call (voiceover1..5, MUTE CALL img_481) -> gameplay ->
 *   JUMPSCARE (Active 3 anim 65/52/34/43 frames + XSCREAM) / POWER OUT
 *   (powerdown + circus loop) -> 6 AM (chimes + crowd) / GAME OVER.
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "fnaf.h"
#include "DebugConsole.h"
#include "MenuSystem.h"
#include "Progress.h"     // v2.7.13: persistent night-flow progress
#include "Achievements.h" // v2.14: in-game achievements
#include "SpriteBatch.h"
#include "PakLoader.h"
#include "InputSystem.h"
#include "GameRender.h"
#include "AudioSystem.h"
#include "asset_mapping.hpp"

#include <xtl.h>
#include <xinputdefs.h>  // XINPUT_KEYSTROKE / XINPUT_FLAG_KEYBOARD (USB-keyboard reset)
#include "XdkCompat.h"   // Snprintf — XDK CRT predates C99 snprintf
#define PlatformSleepMs(ms) Sleep(ms)

using namespace fnaf;

// ============================================================
//  D3D9 globals
// ============================================================
static LPDIRECT3D9       g_pD3D = NULL;
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
static TextRenderer      g_text;
static DebugConsole      g_debugConsole;
static SpriteBatch       g_batch;
static PakLoader         g_pak;
static GameRender        g_render;
static AudioSystem       g_audio;
static bool              g_pakLoaded = false;
static bool              g_showConsole = true;  // v2.17: DEV toggle for the on-screen debug console
static f32               g_goldenScareT = -1.0f;// v2.17: scare flash timer (-1 = off)
static int               g_scareFlashImg = -1;   // v2.17: image handle for the scare flash
static f32               g_itsmeT = -1.0f;       // v2.17: IT'S ME hallucination timer (-1 = off)
static f32               g_itsmeRollTimer = 0.0f; // v2.17: 20 s accumulator for the rare IT'S ME roll

// Game reference for callbacks needing state
static Game* g_gameRef = nullptr;

static const i32 SCREEN_W = 1280;
static const i32 SCREEN_H = 720;

// On the console printf goes nowhere (no stdout), so ANY fatal init failure
// must be reported through XShowMessageBoxUI -- otherwise the user just sees
// a silent black screen. ascii -> UTF-16 copy is manual (no CRT conversion
// needed for the plain-ASCII diagnostic strings we emit).
static void ShowFatalError(const char* titleA, const char* textA) {
    wchar_t wTitle[64];
    wchar_t wText[320];
    int i = 0;
    for (; titleA && titleA[i] && i < 63; ++i) wTitle[i] = (wchar_t)(unsigned char)titleA[i];
    wTitle[i] = 0;
    for (i = 0; textA && textA[i] && i < 319; ++i) wText[i] = (wchar_t)(unsigned char)textA[i];
    wText[i] = 0;
    LPCWSTR awszButtons[] = { L"OK" };
    MESSAGEBOX_RESULT result;
    XOVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(XOVERLAPPED));
    DWORD dwRet = XShowMessageBoxUI(XUSER_INDEX_ANY, wTitle, wText, 1, awszButtons,
                                    0, XMB_ERRORICON, &result, &overlapped);
    if (dwRet == ERROR_IO_PENDING) {
        while (!XHasOverlappedIoCompleted(&overlapped)) Sleep(16);
        XGetOverlappedResult(&overlapped, NULL, TRUE);
    }
    Sleep(2000);
}

static bool InitD3D() {
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_pD3D) return false;

    // Attempt 1: canonical 1280x720 config. Attempt 2: tolerant fallback
    // (A8R8G8B8 back buffer, 2 counts, refresh 0). On 360 Windowed=FALSE,
    // HARDWARE_VERTEXPROCESSING and DISCARD are what every XDK sample uses.
    struct Config { UINT w, h, count, refresh; D3DFORMAT fmt; };
    const Config cfgs[2] = {
        { SCREEN_W, SCREEN_H, 1, 60, D3DFMT_X8R8G8B8 },
        { SCREEN_W, SCREEN_H, 2, 0,  D3DFMT_A8R8G8B8 },
    };
    HRESULT hr = E_FAIL;
    for (int c = 0; c < 2 && !g_pd3dDevice; ++c) {
        D3DPRESENT_PARAMETERS d3dpp;
        ZeroMemory(&d3dpp, sizeof(d3dpp));
        d3dpp.BackBufferWidth   = cfgs[c].w;
        d3dpp.BackBufferHeight  = cfgs[c].h;
        d3dpp.BackBufferFormat  = cfgs[c].fmt;
        d3dpp.BackBufferCount   = cfgs[c].count;
        d3dpp.MultiSampleType   = D3DMULTISAMPLE_NONE;
        d3dpp.SwapEffect        = D3DSWAPEFFECT_DISCARD;
        // v2.8: depth-stencil DISABLED. The game is pure 2D (Z writes are
        // never enabled) and the Perspective post-process needs its own
        // 1280x720 EDRAM render target; back buffer + capture would overflow
        // the 10MB EDRAM if a full D24S8 depth buffer were also resident.
        d3dpp.EnableAutoDepthStencil = FALSE;
        d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
        d3dpp.Windowed = FALSE;
        d3dpp.hDeviceWindow = NULL;
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        d3dpp.FullScreen_RefreshRateInHz = cfgs[c].refresh;
        hr = g_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);
    }
    if (FAILED(hr) || !g_pd3dDevice) {
        char msg[128];
        Snprintf(msg, sizeof(msg), "CreateDevice failed, hr=0x%08X", (unsigned)hr);
        ShowFatalError("D3D init failed", msg);
        return false;
    }
    // Order matters: the text renderer queues glyphs through the sprite
    // batch, so the batch must come up first. Any failure is FATAL and
    // visible: silent init failures were the v2.1 black-screen mode.
    if (!g_batch.Init(g_pd3dDevice)) {
        char msg[256];
        Snprintf(msg, sizeof(msg), "SpriteBatch: %s", g_batch.GetInitError());
        ShowFatalError("Renderer init failed", msg);
        return false;
    }
    if (!g_text.Init(g_pd3dDevice, &g_batch, 26)) {
        ShowFatalError("TextRenderer init failed", "font atlas texture creation failed");
        return false;
    }
    g_debugConsole.Init(&g_text, 128, 18);
    return true;
}
static void ShutdownD3D() {
    g_audio.Shutdown();
    g_pak.Unload();
    g_batch.Shutdown();
    g_text.Shutdown();
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

// Common per-frame frame begin/end -------------------------------------------------
static u32 ColorForState(GameState state, f32 power) {
    switch (state) {
        case GAME_STATE_MENU: return D3DCOLOR_XRGB(10, 10, 10);
        case GAME_STATE_DISCLAIMER: return D3DCOLOR_XRGB(0, 0, 0);
        case GAME_STATE_INTRO_AD:   return D3DCOLOR_XRGB(0, 0, 0);   // v2.7.13
        case GAME_STATE_NIGHT_START: return D3DCOLOR_XRGB(5, 5, 20);
        case GAME_STATE_PLAYING: { BYTE g = (BYTE)((power/100.0f)*30.0f); return D3DCOLOR_XRGB(5, 5+g, 20); }
        case GAME_STATE_POWER_OUT: return D3DCOLOR_XRGB(2, 2, 4);
        case GAME_STATE_JUMPSCARE: return D3DCOLOR_XRGB(200, 200, 200);
        case GAME_STATE_NIGHT_COMPLETE: return D3DCOLOR_XRGB(5, 40, 5);
        case GAME_STATE_GAME_OVER: return D3DCOLOR_XRGB(40, 0, 0);
        default: return D3DCOLOR_XRGB(0,0,0);
    }
}
static bool FrameBegin(u32 clearColor) {
    if (!g_pd3dDevice) return false;
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, clearColor, 1.0f, 0);
    HRESULT hr = g_pd3dDevice->BeginScene();
    if (SUCCEEDED(hr)) {
        // reset the sprite queue for this frame; screens just queue quads,
        // FrameEnd flushes everything in one place
        g_batch.Begin();
    }
    return SUCCEEDED(hr);
}
static void FrameEnd() {
    if (!g_pd3dDevice) return;
    if (g_showConsole) g_debugConsole.Render(SCREEN_W, SCREEN_H);   // v2.17: DEV toggle
    if (g_goldenScareT >= 0.0f) g_render.RenderScareFlash(g_scareFlashImg, g_goldenScareT);   // scare flash
    if (g_itsmeT >= 0.0f) g_render.RenderItsmeFlash(g_itsmeT);   // IT'S ME hallucination
    // Flush ALL queued quads (sprites AND text) before ending the scene --
    // without this the last same-texture batch renders one frame late
    // (or not at all for static screens).
    g_batch.End();
    g_pd3dDevice->EndScene();
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

static bool ShowPakErrorScreen(){
    static bool shown = false;
    if (shown) { exit(0); return false; }
    shown = true;
    DWORD dwUserIndex = XUSER_INDEX_ANY;
    LPCWSTR wszTitle = L"fnaf1.pak NOT FOUND";
    LPCWSTR wszText  = L"The game requires fnaf1.pak\nPlace it next to the .xex";
    LPCWSTR awszButtons[] = { L"OK" };
    DWORD cButtons = 1;
    DWORD dwFocusButton = 0;
    DWORD dwFlags = XMB_ERRORICON;
    MESSAGEBOX_RESULT result;
    XOVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(XOVERLAPPED));

    DWORD dwRet = XShowMessageBoxUI(
        dwUserIndex,
        wszTitle,
        wszText,
        cButtons,
        awszButtons,
        dwFocusButton,
        dwFlags,
        &result,
        &overlapped
    );
    if (dwRet == ERROR_IO_PENDING) {
        while (!XHasOverlappedIoCompleted(&overlapped)) Sleep(16);
        DWORD dwRes = XGetOverlappedResult(&overlapped, NULL, TRUE);
        if (dwRes == ERROR_SUCCESS) {
            exit(0);
        } else {
            exit(0);
        }
    } else {
        exit(0);
    }
    return false;
}

// v2.23: ask whether to import a loose "freddy" save found next to the game
// into the XContent save container. Same async pattern as ShowPakErrorScreen.
static bool ShowImportSavePrompt(){
    DWORD dwUserIndex = XUSER_INDEX_ANY;
    LPCWSTR wszTitle = L"Import save";
    LPCWSTR wszText  = L"Do you want to import the save found in the game folder?";
    LPCWSTR awszButtons[] = { L"No", L"Yes" };
    DWORD cButtons = 2;
    DWORD dwFocusButton = 1;               // default highlight "Yes"
    DWORD dwFlags = XMB_QUESTIONICON;
    MESSAGEBOX_RESULT result;
    XOVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(XOVERLAPPED));

    DWORD dwRet = XShowMessageBoxUI(
        dwUserIndex, wszTitle, wszText, cButtons, awszButtons,
        dwFocusButton, dwFlags, &result, &overlapped);

    if (dwRet == ERROR_IO_PENDING) {
        while (!XHasOverlappedIoCompleted(&overlapped)) Sleep(16);
        DWORD dwRes = XGetOverlappedResult(&overlapped, NULL, TRUE);
        if (dwRes != ERROR_SUCCESS) return false;
    } else if (dwRet != ERROR_SUCCESS) {
        return false;
    }
    return result.dwButtonPressed == 1;    // 1 == "Yes"
}

// v2.23: informational box after a successful import — tell the player to
// restart so the imported save is picked up fresh.
static void ShowImportDonePrompt(){
    DWORD dwUserIndex = XUSER_INDEX_ANY;
    LPCWSTR wszTitle = L"Import complete";
    LPCWSTR wszText  = L"Import complete. Please restart the game.";
    LPCWSTR awszButtons[] = { L"OK" };
    DWORD cButtons = 1;
    DWORD dwFocusButton = 0;
    DWORD dwFlags = XMB_NOICON;
    MESSAGEBOX_RESULT result;
    XOVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(XOVERLAPPED));

    DWORD dwRet = XShowMessageBoxUI(
        dwUserIndex, wszTitle, wszText, cButtons, awszButtons,
        dwFocusButton, dwFlags, &result, &overlapped);

    if (dwRet == ERROR_IO_PENDING) {
        while (!XHasOverlappedIoCompleted(&overlapped)) Sleep(16);
        XGetOverlappedResult(&overlapped, NULL, TRUE);
    }
}

// ============================================================
//  Game -> platform callbacks (sound + state hooks)
// ============================================================
static i32  s_lastHour=-1;
static f32  s_lastPower=-1.0f;
static bool s_phonePlaying = false;   // voiceover active (for MUTE CALL blink)
static bool s_phoneMuted = false;     // player muted the call

// v2.7.13: persistent progress (fnaf_save.bin next to the XEX) + the
// title-menu refresh (Continue target, unlock caps, stars) built from it.
static GameProgress g_prog;
static Achievements g_ach;      // v2.14: in-game achievements (auto-loaded at boot)
static bool g_achScreen = false;

// ---- v2.15 frame fade transitions (docs/FRAME_TRANSITIONS.md) ----
// A black full-screen overlay whose alpha ramps 0..1 (fade-out) then 1..0
// (fade-in), gated by per-state millisecond durations extracted from the
// original's frame Transition blocks (STDT/FADE).
struct ScreenFade {
    int      phase;   // 0 = none, 1 = fade-out, 2 = fade-in
    float    alpha;   // 0 clear .. 1 fully black
    float    timer;   // seconds
    float    dur;     // seconds
    GameState target; // state to switch to after fade-out
};
static ScreenFade g_fade = { 0, 0.0f, 0.0f, 0.0f, GAME_STATE_MENU };
static bool g_officeAmb = false;   // office ambience already started (edge guard)
static bool g_menuAmb   = false;   // title ambience already started (edge guard)
static bool g_nightBlip = false;   // "what day" card blip already played (edge guard)

static int FadeOutMs(GameState s) {
    switch (s) {
        case GAME_STATE_DISCLAIMER:     return 1010;
        case GAME_STATE_NIGHT_START:    return 1010;
        case GAME_STATE_NIGHT_COMPLETE: return 900;
        case GAME_STATE_INTRO_AD:       return 2000;
        default: return 0;   // title / office / power-out / jumpscare: hard cut
    }
}
static int FadeInMs(GameState s) {
    switch (s) {
        case GAME_STATE_DISCLAIMER:     return 1010;
        case GAME_STATE_NIGHT_COMPLETE: return 1010;
        case GAME_STATE_GAME_OVER:      return 1010;
        case GAME_STATE_INTRO_AD:       return 2000;
        default: return 0;
    }
}

static void StartTransition(GameState& state, GameState to) {
    if (g_fade.phase != 0) return;   // a transition is already running
    int out = FadeOutMs(state);
    int in  = FadeInMs(to);
    if (out <= 0 && in <= 0) {
        state = to;                 // hard cut
        return;
    }
    if (out > 0) {
        g_fade.phase  = 1;
        g_fade.alpha  = 0.0f;
        g_fade.timer  = 0.0f;
        g_fade.dur    = (float)out / 1000.0f;
        g_fade.target = to;         // `state` does NOT change yet
    } else {
        state         = to;         // no fade-out: switch now, then fade in
        g_fade.phase  = 2;
        g_fade.alpha  = 1.0f;
        g_fade.timer  = 0.0f;
        g_fade.dur    = (float)in / 1000.0f;
        g_fade.target = to;
    }
}

static void TickFade(GameState& state, float dt) {
    if (g_fade.phase == 0) return;
    g_fade.timer += dt;
    if (g_fade.phase == 1) {                 // fading out
        if (g_fade.timer >= g_fade.dur) {
            state = g_fade.target;
            int in = FadeInMs(state);
            if (in > 0) {
                g_fade.phase = 2;
                g_fade.alpha = 1.0f;
                g_fade.timer = 0.0f;
                g_fade.dur   = (float)in / 1000.0f;
            } else {
                g_fade.phase = 0;
                g_fade.alpha = 0.0f;
            }
        } else {
            g_fade.alpha = g_fade.timer / g_fade.dur;   // 0 -> 1
        }
    } else {                                 // fading in
        if (g_fade.timer >= g_fade.dur) {
            g_fade.phase = 0;
            g_fade.alpha = 0.0f;
        } else {
            g_fade.alpha = 1.0f - g_fade.timer / g_fade.dur;  // 1 -> 0
        }
    }
}

static void DrawFadeOverlay() {
    if (g_fade.alpha > 0.0f) g_render.DrawFade(g_fade.alpha);
}

// ---- v2.16 audio mixer channels (match the original "Speaker" channels) ----
enum {
    CH_FAN          = 1,   // Buzz_Fan loop — base 25 (camera down) / 10 (camera up)
    CH_COLDPRESC    = 2,   // ColdPresc B loop — base 50
    CH_BALLAST      = 3,   // BallastHum loop — muted by camera-up / lights
    CH_CAMCORDER    = 6,   // MiniDV_Tape_Eject — monitor-up (100/0)
    CH_CAMWHIR      = 7,   // CAMERA_VIDEO_LOA — monitor flip-up
    CH_DEEPSTEPS    = 8,   // deep steps — distance 10..40
    CH_OVEN         = 10,  // kitchen oven drawer (Chica in kitchen)
    CH_PIRATE       = 13,  // pirate song2 — 15 watching cove / 5 otherwise
    CH_BREATHS      = 14,  // vocals breaths — base 50
    CH_CIRCUS       = 15,  // circus — base 5
    CH_FREDDY_LAUGH = 16,  // Freddy "got in" laugh — proximity ramp
    CH_EERIE        = 18,  // EerieAmbience loop — proximity (muted at start)
    CH_PHONE        = 19,  // voiceover phone — 100 office / 50 viewing / 0 mute
    CH_POUNDING     = 20,  // door pounding
    CH_ROBOTVOICE   = 21,  // robotvoice loop — proximity (muted at start)
    CH_MUSICBOX     = 22,  // music box (Freddy in kitchen) — base 25
    CH_RUNFAST      = 24,  // running fast3 — Freddy proximity ramp
    CH_WHISPER      = 25,  // whispering2 — Freddy "got in"
    CH_GF_GIGGLE    = 27,  // Golden Freddy laugh #38
    CH_XSCREAM2     = 29   // creepy-end scream #46
};

// v2.16: per-frame dynamic channel volumes (the "1:1" mixer). Runs while the
// office is active; matches the original's live per-channel volume events
// (camera up/down, lights, mute-call, animatronic proximity).
static void TickAudioMixer(const Game& game) {
    const bool monUp  = game.GetCameras().IsMonitorUp();
    const DoorSystem& doors = game.GetDoors();
    const bool lightL = doors.IsLightOn(DOOR_LEFT);
    const bool lightR = doors.IsLightOn(DOOR_RIGHT);

    // fan: 25 down / 10 up (groups 143/144)
    g_audio.SetChannelVolume(CH_FAN, CFVolumeToDb(monUp ? 10 : 25));
    // ballast hum: mute when camera up or a light is on (groups 114-129/326)
    g_audio.SetChannelVolume(CH_BALLAST, (monUp || lightL || lightR) ? -100.0f : CFVolumeToDb(50));
    // phone: 100 office / 50 viewing / 0 mute (groups 360/361/379)
    g_audio.SetChannelVolume(CH_PHONE, s_phoneMuted ? -100.0f : CFVolumeToDb(monUp ? 50 : 100));
    // pirate song2: 15 watching the cove (CAM 1C), 5 otherwise (groups 274/275)
    g_audio.SetChannelVolume(CH_PIRATE,
        CFVolumeToDb((game.GetCameras().GetCurrentCamera() == CAM_1C) ? 15 : 5));

    // proximity ambience (robotvoice ch21, EerieAmbience ch18)
    const AnimatronicAI& ai = game.GetAI();
    const RoomId bonnie = ai.GetAnimatronic(ANIM_BONNIE).currentRoom;
    const RoomId chica  = ai.GetAnimatronic(ANIM_CHICA).currentRoom;
    const RoomId freddy = ai.GetAnimatronic(ANIM_FREDDY).currentRoom;
    const RoomId foxy   = ai.GetAnimatronic(ANIM_FOXY).currentRoom;

    float watched = 0.0f;
    if (bonnie == ROOM_WEST_HALL_CORNER || bonnie == ROOM_LEFT_DOOR)  { if (watched < 0.6f) watched = 0.6f; }
    if (chica  == ROOM_EAST_HALL_CORNER || chica  == ROOM_RIGHT_DOOR) { if (watched < 0.6f) watched = 0.6f; }
    if (freddy == ROOM_EAST_HALL || freddy == ROOM_EAST_HALL_CORNER || freddy == ROOM_RIGHT_DOOR) { if (watched < 0.4f) watched = 0.4f; }
    if (foxy   == ROOM_LEFT_DOOR || foxy == ROOM_OFFICE) watched = 1.0f;
    if (freddy == ROOM_OFFICE) watched = 1.0f;

    g_audio.SetChannelVolume(CH_ROBOTVOICE, AmplitudeToDb(watched));
    g_audio.SetChannelVolume(CH_EERIE,      AmplitudeToDb(watched * 0.6f));
}

// v2.18: periodic random one-shot ambience (the "1:1" random events).
//   pirate song (group 269): every 80 s, 1/30, while Foxy is still in the cove;
//   circus (group 270):      every 100 s, 1/30, unconditional;
//   breaths (groups 276/278):every 100 s, 1/3, when Bonnie/Chica wait at a
//                            door while you watch a camera.
static void TickRandomEvents(const Game& game) {
    static f32 s_pirateT = 0.0f, s_circusT = 0.0f, s_breathT = 0.0f;
    const f32 dt = 1.0f/60.0f;
    const AnimatronicAI& ai = game.GetAI();
    const Animatronic& foxy   = ai.GetAnimatronic(ANIM_FOXY);
    const Animatronic& bonnie = ai.GetAnimatronic(ANIM_BONNIE);
    const Animatronic& chica  = ai.GetAnimatronic(ANIM_CHICA);
    const bool monUp = game.GetCameras().IsMonitorUp();

    s_pirateT += dt;
    if (s_pirateT >= 80.0f) {
        s_pirateT = 0.0f;
        if (foxy.foxyStage <= FOXY_STAGE_2 && (rand() % 30) == 0)
            g_audio.PlayOnChannel(&g_pak, Snd::PIRATE_SONG, false, CH_PIRATE);
    }

    s_circusT += dt;
    if (s_circusT >= 100.0f) {
        s_circusT = 0.0f;
        if ((rand() % 30) == 0)
            g_audio.PlayOnChannel(&g_pak, Snd::CIRCUS, false, CH_CIRCUS);
    }

    s_breathT += dt;
    if (s_breathT >= 100.0f) {
        s_breathT = 0.0f;
        if (monUp) {
            if (bonnie.currentRoom == ROOM_LEFT_DOOR  && (rand() % 3) == 0)
                g_audio.PlayOnChannel(&g_pak, Snd::BREATHS[rand() % 4], false, CH_BREATHS);
            if (chica.currentRoom  == ROOM_RIGHT_DOOR && (rand() % 3) == 0)
                g_audio.PlayOnChannel(&g_pak, Snd::BREATHS[rand() % 4], false, CH_BREATHS);
        }
    }

    // v2.22 kitchen oven (groups 245-250): Chica in the kitchen rattles the
    // oven drawer every ~80 s (random 1..10 gate -> 4 OVEN-DRA variants).
    static f32 s_ovenT = 0.0f;
    s_ovenT += dt;
    if (s_ovenT >= 80.0f) {
        s_ovenT = 0.0f;
        if (chica.currentRoom == ROOM_KITCHEN && (rand() % 10) < 5)
            g_audio.PlayOnChannel(&g_pak, Snd::OVEN_DRAW[rand() % 4], false, CH_OVEN);
    }
}

// v2.22: "Active 2" power-out face sounds (groups 219-222). During power-out
// phase 1, each NEW lit face flash advances the face state 1..4 and plays its
// distinct sound: computer-digital (1), garble1/2/3 (2/3/4).
static int  s_faceStatePrev = 0;
static void TickPowerOutFaceSound(const Game& game) {
    const i32 fs = game.GetPowerOutFaceState();
    if (fs != 0 && fs != s_faceStatePrev) {
        switch (fs) {
            case 1: g_audio.Play(&g_pak, Snd::COMPUTER_DIG, false, 0.8f); break;
            case 2: g_audio.Play(&g_pak, Snd::GARBLE[0], false, 0.8f); break;
            case 3: g_audio.Play(&g_pak, Snd::GARBLE[1], false, 0.8f); break;
            case 4: g_audio.Play(&g_pak, Snd::GARBLE[2], false, 0.8f); break;
        }
    }
    s_faceStatePrev = fs;
}

// v2.22: Golden Freddy ("yellow bear") state machine.
//   0 idle -> 1 poster rolled (1/100 on monitor drop) -> 2 giggle #38 played
//   (on viewing CAM 2B) -> appears in the office when the monitor drops ->
//   ~5 s to raise the monitor or he kills you (creepy start).
static int   s_goldState = 0;
static bool  s_goldInOffice = false;
static f32   s_goldTimer = 0.0f;

static void TickGoldenFreddy(Game& game, GameRender& render) {
    const bool monUp = game.GetCameras().IsMonitorUp();
    const CameraId cam = game.GetCameras().GetCurrentCamera();

    // 1. summon when the 1/100 "random for pic" roll hits (rolled on drop)
    if (s_goldState == 0 && render.GetGoldenRoll() == 0) s_goldState = 1;

    // 2. giggle #38 (Laugh_Giggle_Girl_1) when viewing CAM 2B poster once
    if (monUp && cam == CAM_2B && s_goldState == 1) {
        g_audio.Play(&g_pak, Snd::FREDDY_LAUGH_LONG, false, 0.9f);
        s_goldState = 2;
    }

    // 3. appear when the monitor drops back down
    if (!monUp && s_goldState == 2 && !s_goldInOffice) {
        s_goldInOffice = true;
        s_goldTimer = 0.0f;
    }

    // 4. hold: raising the monitor despawns him; ~5 s -> the kill
    if (s_goldInOffice) {
        s_goldTimer += 1.0f / 60.0f;
        if (monUp) {
            s_goldInOffice = false; s_goldState = 0; s_goldTimer = 0.0f;
        } else if (s_goldTimer >= 5.0f) {
            s_goldInOffice = false; s_goldState = 0; s_goldTimer = 0.0f;
            game.DebugTriggerGoldenFreddy();
        }
    }

    render.SetGoldenFreddyInOffice(s_goldInOffice);
}

static void RefreshMenuFromProgress(MenuSystem& menu) {
    Progress::Load(g_prog);
    i32 un = g_prog.nextNight;           if (un < 1) un = 1;       if (un > 7) un = 7;
    i32 lastDone = g_prog.nextNight - 1; if (lastDone < 0) lastDone = 0; if (lastDone > 7) lastDone = 7;
    menu.Init(un, lastDone);
    menu.SetHasSave(lastDone > 0);
}

// Original title events: pressing <Delete> on the title screen wipes the Ini
// (level=1, beatgame/beat6/beat7=0). On the console this arrives from a USB
// keyboard through XInputGetKeystroke (XINPUT_FLAG_KEYBOARD).
static void PollTitleKeyboardReset(MenuSystem& menu) {
    XINPUT_KEYSTROKE ks;
    while (XInputGetKeystroke(XUSER_INDEX_ANY, XINPUT_FLAG_KEYBOARD, &ks) == ERROR_SUCCESS) {
        if ((ks.Flags & XINPUT_KEYSTROKE_KEYDOWN) && ks.VirtualKey == VK_DELETE) {
            Progress::Reset(g_prog);
            if (Progress::Save(g_prog)) {
                RefreshMenuFromProgress(menu);
                g_debugConsole.Print("SAVE WIPED (Delete)");
            } else {
                g_debugConsole.Print("SAVE WIPE FAILED");
            }
        }
    }
}

void OnTimeUpdate(i32 hour){
    if(hour!=s_lastHour){
        s_lastHour=hour;
        // v2.17: no hourly laugh in the original — Freddy's giggle is tied to
        // his "got in" entry, not the clock (the old FREDDY_LAUGH[hour%3] was wrong).
    }
}
void OnPowerUpdate(f32 power){ s_lastPower=power; }

void OnJumpscare(AnimatronicId anim){
    const char* n[]={"Freddy","Bonnie","Chica","Foxy"};
    const char* nm = (anim >= ANIM_FREDDY && anim < ANIM_COUNT) ? n[anim] : "Golden Freddy";
    printf("*** JUMP SCARE by %s! ***\n", nm);
    // group 228/322/408: XSCREAM (voiceover/garble stop too)
    g_audio.Stop(Snd::VOICEOVER[0]); g_audio.Stop(Snd::VOICEOVER[1]);
    g_audio.Stop(Snd::VOICEOVER[2]); g_audio.Stop(Snd::VOICEOVER[3]);
    g_audio.Stop(Snd::VOICEOVER[4]);
    g_audio.Stop(Snd::AMBIENCE2); g_audio.Stop(Snd::CIRCUS);
    s_phonePlaying=false;
    g_audio.Play(&g_pak, Snd::XSCREAM, false, 1.0f);
    // v2.23: no "lives" system — the original has none. A death just returns
    // to the title and Continue retries the same (unlocked) night; progress is
    // only written on a 6 AM screen, never on a jumpscare.
    g_ach.OnJumpscare();   // v2.14: "No Hiding"
}
void OnPowerOut(){
    printf("*** POWER OUT! ***\n");
    // group 285: powerdown + stop office loops
    g_audio.Stop(Snd::COLD_PRESC); g_audio.Stop(Snd::BUZZ_FAN);
    g_audio.Stop(Snd::BALLAST_HUM); g_audio.Stop(Snd::ROBOT_VOICE);
    g_audio.Stop(Snd::EERIE_AMBIENCE); g_audio.Stop(Snd::STATIC_LOOP); g_audio.Stop(Snd::STATIC2);
    s_phonePlaying=false;
    g_audio.Play(&g_pak, Snd::POWERDOWN, false, 1.0f);
    // group 271/286: dark ambient drone (ambience2) alongside the music box
    g_audio.Play(&g_pak, Snd::AMBIENCE2, true, 0.5f);
    s_faceStatePrev = 0;   // v2.22: reset the "Active 2" face-sound cycle
}
void OnMusicBoxStart(){
    printf("(Music box starts...)\n");
    // group 269: circus loop
    g_audio.Play(&g_pak, Snd::CIRCUS, true, 0.95f);
}
void OnMusicBoxStop(){
    printf("(Music box stops...)\n");
    g_audio.Stop(Snd::CIRCUS);
}
void OnNightComplete(i32 night){
    printf("\n6 AM -- Night %d Complete!\n",night);
    g_audio.StopAll();
    // chimes start immediately (frame "next day" group 1); the kids cheer
    // fires on the roll-landing edge in main (v2.7.13)
    g_audio.Play(&g_pak, Snd::CHIMES, false, 1.0f);
    // nights 5/6/7: paycheck/overtime/pink slip hold under the music box
    if(night>=5) g_audio.Play(&g_pak, Snd::CIRCUS, true, 0.85f);
    // v2.7.13: persist progress (original Ini: level)
    Progress::Load(g_prog);
    g_prog.nextNight = (night<7)?(night+1):7;
    Progress::Save(g_prog);

    // v2.14: unlock night-scoped achievements. "perfect" gates "No Tampering"
    // (Custom Night 20/20/20/20); "No Running"/"No Laughing" are the
    // prevent-side night 4/5 achievements tracked during the night.
    bool perfect = false;
    if (g_gameRef) {
        const fnaf::AnimatronicAI& ai = g_gameRef->GetAI();
        perfect = (ai.GetAILevel(fnaf::ANIM_FREDDY) == 20) &&
                  (ai.GetAILevel(fnaf::ANIM_BONNIE) == 20) &&
                  (ai.GetAILevel(fnaf::ANIM_CHICA)  == 20) &&
                  (ai.GetAILevel(fnaf::ANIM_FOXY)   == 20);
    }
    g_ach.OnNightComplete(night, perfect);
}
void OnGameOver(){
    printf("--- GAME OVER ---\n");
    g_audio.StopAll();
    g_audio.Play(&g_pak, Snd::STATIC2, true, 0.6f);
}
void OnCameraChange(CameraId cam, int reason){
    // v2.22: 1:1 camera audio — the flip-up whir (CAMERA_VIDEO_LOA), the
    // camera-switch blip (blip3), and the flip-down "put down" are DISTINCT
    // sounds. The old code replayed the whir on EVERY cond → the "extra
    // sound" the user heard when switching cameras.
    if(reason==CAM_REASON_DOWN){
        printf("[Camera DOWN]\n");
        g_audio.Stop(Snd::STATIC_LOOP);
        g_audio.Play(&g_pak, Snd::PUT_DOWN, false, 0.9f);   // group 322 flip-down
    }
    else if(reason==CAM_REASON_UP){
        printf("[Camera UP: %s]\n",CameraSystem::GetCameraName(cam));
        // group 130: monitor flip-up whir + static while up
        g_audio.Play(&g_pak, Snd::CAMERA_SWITCH, false, 0.8f);
        g_audio.Play(&g_pak, Snd::STATIC_LOOP, true, 0.5f);
        // group 144: camcorder tape-eject (ch6) when the feed commits
        g_audio.Play(&g_pak, Snd::TAPE_EJECT, false, 0.8f);
    }
    else { // CAM_REASON_SWITCH
        printf("[Camera: %s]\n",CameraSystem::GetCameraName(cam));
        // group 16: camera-switch blip3 (static is already looping while up —
        // don't restart it).
        g_audio.Play(&g_pak, Snd::BLIP, false, 0.8f);
    }
}
void OnDoorChange(DoorSide s,bool c){
    printf("[Door %s: %s]\n",s==DOOR_LEFT?"Left":"Right",c?"CLOSED":"OPEN");
    // door slam (SFXBible_12478) on close
    g_audio.Play(&g_pak, Snd::DOOR_SLAM, false, 1.0f);
}
void OnLightChange(DoorSide s,bool o){
    printf("[Light %s: %s]\n",s==DOOR_LEFT?"Left":"Right",o?"ON":"OFF");
}
void OnAnimatronicMove(AnimatronicId a,RoomId r){
    const char* n[]={"Freddy","Bonnie","Chica","Foxy"};
    RoomInfo i=RoomSystem::GetRoomInfo(r);
    printf("[AI] %s -> %s\n",n[a],i.name);
    // v2.17: deep steps for Bonnie/Chica; Freddy's laugh is the _1d/_2d/_8d
    // giggle family (#56/57/58), NOT Laugh_Giggle_Girl_1 (#38 = Golden Freddy).
    if(a==ANIM_BONNIE||a==ANIM_CHICA) {
        // v2.19: footsteps volume by distance (groups 198-244), expressed as
        // Clickteam values: far 10, mid 30, near 40 (muted when overlapping).
        float v = CFVolumeToDb(10);
        if (r==ROOM_WEST_HALL || r==ROOM_SUPPLY_CLOSET || r==ROOM_EAST_HALL) v = CFVolumeToDb(30);
        else if (r==ROOM_WEST_HALL_CORNER || r==ROOM_EAST_HALL_CORNER ||
                 r==ROOM_LEFT_DOOR || r==ROOM_RIGHT_DOOR) v = CFVolumeToDb(40);
        g_audio.SetChannelVolume(CH_DEEPSTEPS, v);
        g_audio.PlayOnChannel(&g_pak, Snd::DEEP_STEPS, false, CH_DEEPSTEPS);
    }
    else if(a==ANIM_FREDDY) {
        // v2.19: "got in" laugh (groups 390-405): a RANDOM giggle variant
        // (#56/#57/#58 = random(1,3)) plus "running fast3" as Freddy closes
        // in. Volume ramps with proximity (Clickteam values): bathrooms
        // laugh 20/run 35, kitchen 30/40, E-hall 40/60, corner 60/75,
        // right door 80/100.
        float laughV = CFVolumeToDb(20), runV = CFVolumeToDb(35);
        if      (r==ROOM_KITCHEN)          { laughV = CFVolumeToDb(30); runV = CFVolumeToDb(40); }
        else if (r==ROOM_EAST_HALL)        { laughV = CFVolumeToDb(40); runV = CFVolumeToDb(60); }
        else if (r==ROOM_EAST_HALL_CORNER) { laughV = CFVolumeToDb(60); runV = CFVolumeToDb(75); }
        else if (r==ROOM_RIGHT_DOOR)       { laughV = CFVolumeToDb(80); runV = CFVolumeToDb(100); }
        g_audio.SetChannelVolume(CH_FREDDY_LAUGH, laughV);
        g_audio.SetChannelVolume(CH_RUNFAST, runV);
        g_audio.PlayOnChannel(&g_pak, Snd::FREDDY_LAUGH[rand() % 3], false, CH_FREDDY_LAUGH);
        g_audio.PlayOnChannel(&g_pak, Snd::RUNNING_FAST, false, CH_RUNFAST);
        // music box while Freddy is in the kitchen (groups 399/400, ch22)
        if (r==ROOM_KITCHEN) g_audio.PlayOnChannel(&g_pak, Snd::MUSIC_BOX, false, CH_MUSICBOX);
    }
    // v2.14: "No Laughing" — Freddy steps into the East Hall on Night 5
    if(a==ANIM_FREDDY && r==ROOM_EAST_HALL) g_ach.OnFreddyEast();
}
void OnFoxyStageChange(FoxyStage s){
    const char* t[]={"Curtain Closed","Peeking","Gone","Lurking","RUNNING!","AT DOOR!"};
    printf("[AI] Foxy: %s\n",t[s]);
    if(s==FOXY_STAGE_3){
        // group 39: Foxy run down the hall
        g_audio.Play(&g_pak, Snd::RUN, true, 1.0f);
        g_audio.Play(&g_pak, Snd::RUNNING_FAST, true, 1.0f);
    }
    // v2.14: "No Running" — Foxy leaves the cove (stage 4 sprint)
    if(s==FOXY_STAGE_4) g_ach.OnFoxyRan();
}
void OnFoxyDoorBang(f32 p){
    printf("[AI] Foxy bangs! -%.1f%%\n",p);
    // group 270/323: pounding + knock
    g_audio.Stop(Snd::RUN); g_audio.Stop(Snd::RUNNING_FAST);
    g_audio.Play(&g_pak, Snd::DOOR_POUNDING, false, 1.0f);
    g_audio.Play(&g_pak, Snd::KNOCK, false, 0.9f);
}

// Start the Phone Guy call for the current night (frame 3 groups 361-365)
static void StartPhoneCall(i32 night) {
    if (night < 1 || night > 5) return;           // nights 6/7: no call
    if (s_phoneMuted) return;
    g_audio.SetChannelVolume(CH_PHONE, CFVolumeToDb(100));   // office, monitor down (100 = unity)
    g_audio.PlayOnChannel(&g_pak, Snd::VOICEOVER[night-1], false, CH_PHONE);
    s_phonePlaying = true;
}

// Phone Guy call (frame 3 groups 361-365): the original plays AT OFFICE START
// (night number == N, "play voice N" == 0) — no 2.5 s delay. Nights 6/7 silent.
static bool s_phoneStarted = false;
static void TickPhoneCall(Game& game, f32 dt) {
    (void)dt;
    if (s_phoneStarted) return;
    s_phoneStarted = true;
    StartPhoneCall(game.GetCurrentNight());
}

// ============================================================
//  Debug sprite browser (LB+RB hold ~0.75 s on menu/disclaimer)
//  Screenshot its pages in Xenia to identify every UI text sprite.
// ============================================================
static bool g_browserMode = false;
static i32  g_browserPage = 0;
static int  g_browserHold = 0;

// v2.7.11: PERSPECTIVE TUNER -- L3+R3 toggles it in any state; while a
// night is running it owns the pad (DPad select/adjust, A fast, Y reset)
// and draws its HUD over the bent scene. On exit the current knob values
// are printed to the log + debug console as "PERSP FINAL ..." so they can
// be baked into GameRender.cpp.
static bool g_tunerMode = false;
static i32  g_tunerSel  = 0;
static bool g_devMode  = false;   // v2.17: DEV/debug menu (Start + B)
static i32  g_devSel   = 0;
static i32  g_devNight = 1;
static i32  g_devAnim  = 0;       // 0 Freddy / 1 Bonnie / 2 Chica / 3 Foxy / 4 Golden Freddy
static i32  g_devSound = 0;       // sound-test index
static bool g_devGod   = false;   // god mode
static bool g_devAnalogDoor = false;   // v2.21: analog-door test feature (DEV toggle)
static bool g_devHoldLights = false;   // v2.21: lights as hold-button (DEV toggle)

static const char* const DEV_ANIM_NAMES[8] = { "Freddy", "Bonnie", "Chica", "Foxy", "Golden Freddy", "Bonnie (window)", "Chica (window)", "IT'S ME" };
static const AnimatronicId DEV_ANIMS[4]    = { ANIM_FREDDY, ANIM_BONNIE, ANIM_CHICA, ANIM_FOXY };
// Scare-flash image handles (pak): 571 Golden Freddy sitting, 225/227 door-window stares.
static const int SCARE_IMG_GOLDEN   = 571;
static const int SCARE_IMG_BONNIE_W = 225;
static const int SCARE_IMG_CHICA_W  = 227;

struct DevSoundEntry { const char* label; const char* snd; };
static const DevSoundEntry DEV_SOUNDS[] = {
    { "blip3",       Snd::BLIP },
    { "door slam",   Snd::DOOR_SLAM },
    { "door error",  Snd::DOOR_ERROR },
    { "camera sw",   Snd::CAMERA_SWITCH },
    { "tape eject",  Snd::TAPE_EJECT },
    { "static",      Snd::STATIC_LOOP },
    { "deep steps",  Snd::DEEP_STEPS },
    { "run",         Snd::RUN },
    { "run fast",    Snd::RUNNING_FAST },
    { "fred laugh",  Snd::FREDDY_LAUGH[1] },
    { "golden freddy", Snd::FREDDY_LAUGH_LONG },
    { "pirate song", Snd::PIRATE_SONG },
    { "whispering",  Snd::WHISPERING },
    { "windowscare", Snd::WINDOW_SCARE },
    { "powerdown",   Snd::POWERDOWN },
    { "circus",      Snd::CIRCUS },
    { "music box",   Snd::MUSIC_BOX },
    { "XSCREAM",     Snd::XSCREAM },
    { "XSCREAM2",    Snd::XSCREAM2 },
    { "chimes",      Snd::CHIMES },
    { "crowd kids",  Snd::CROWD_KIDS },
    { "knock",       Snd::KNOCK },
    { "pounding",    Snd::DOOR_POUNDING },
};
static const int DEV_SOUND_COUNT = (int)(sizeof(DEV_SOUNDS)/sizeof(DEV_SOUNDS[0]));

static void TickBrowserEntry(const GameInput& gi) {
    if (gi.leftShoulderHeld && gi.rightShoulderHeld) {
        if (++g_browserHold >= 45) {
            g_browserMode = true;
            g_browserHold = 0;
        }
    } else {
        g_browserHold = 0;
    }
}

// ============================================================
//  main
// ============================================================
int main(int argc, char* argv[]){
    // v2.7.4: FIRST line of the log -- proves which sources are actually in
    // the running XEX (settles "for VS it's as if the files didn't change":
    // check this line or run APPLY_PATCH.bat from the minipatch)
    printf("=== FNAF1-Recomp v2.23 built %s %s ===\n", __DATE__, __TIME__);

    Game game;
    g_gameRef = &game;
    GameCallbacks cb; cb.onTimeUpdate=OnTimeUpdate; cb.onPowerUpdate=OnPowerUpdate;
    cb.onJumpscare=OnJumpscare; cb.onPowerOut=OnPowerOut; cb.onMusicBoxStart=OnMusicBoxStart;
    cb.onMusicBoxStop=OnMusicBoxStop; cb.onNightComplete=OnNightComplete;
    cb.onGameOver=OnGameOver; cb.onCameraChange=OnCameraChange; cb.onDoorChange=OnDoorChange;
    cb.onLightChange=OnLightChange; cb.onAnimatronicMove=OnAnimatronicMove;
    cb.onFoxyStageChange=OnFoxyStageChange; cb.onFoxyDoorBang=OnFoxyDoorBang;
    game.SetCallbacks(cb);
    i32 cmdNight=0; if(argc>1){ cmdNight=atoi(argv[1]); if(cmdNight<1)cmdNight=1; if(cmdNight>7)cmdNight=7; }
    if(!InitD3D()){ printf("FATAL: InitD3D failed\n"); return 1; }
    // v2.7.4: same version banner on the on-screen debug console (bottom of
    // the screen) -- visible without a debugger attached
    g_debugConsole.Print("FNAF1-Recomp v2.23 (%s %s)", __DATE__, __TIME__);

    // Try load pak from Xbox 360 canonical locations (game:\ is XEX directory;
    // e:\/hdd:\ are common on JTAG/RGH dashboards like FSD or Aurora)
    const char* pakPaths[] = {
        "game:\\fnaf1.pak",
        // "D:\\fnaf1.pak",
        // "e:\\fnaf1.pak",
        // "hdd:\\fnaf1.pak",
        // "fnaf1.pak",
        // "./fnaf1.pak"
    };
    const int pakPathCount = (int)(sizeof(pakPaths)/sizeof(pakPaths[0]));
    for(int i=0;i<pakPathCount;++i){
        const char* p = pakPaths[i];
        if(g_pak.Load(p,g_pd3dDevice)){ g_pakLoaded=true; break; }
    }
    if(g_pakLoaded){
        printf("Pak loaded: %d tex %d snd\n",g_pak.GetTextureCount(),g_pak.GetSoundCount());
        g_debugConsole.Print("Pak: %d tex %d snd",g_pak.GetTextureCount(),g_pak.GetSoundCount());
        // v2.6: sound normalization report (RIFF headers peeled / BE payloads
        // swapped / 8-bit expanded) - one glance tells if the audio pass ran
        printf("Pak sounds: riff=%d swapped=%d 8bit=%d\n",
               g_pak.GetSndRiffCount(),g_pak.GetSndSwappedCount(),g_pak.GetSnd8bitCount());

        // v2.7.4: byte-order probe -- first 8 payload bytes of the two first
        // bank sounds. Reference table in docs/EXE_VERSION.md:
        //   correct 360 build : FF 86 00 8D FF 76 00 AA  (ColdPresc B)
        //                       FE 86 00 8F 01 EC 01 64  (BallastHumMedium2)
        //   pre-v2.7 build    : 86 FF 8D 00 76 FF AA 00 / 86 FE 8F 00 EC 01 64 01
        // If the log matches the second row, the running XEX has the OLD
        // PakLoader (that is the remaining noise on the console).
        for(int si=0; si<2 && si<g_pak.GetSoundCount(); ++si){
            PakLoadedSound* s = g_pak.GetSound(si);
            if(s && s->data && s->dataSize>=8){
                printf("Snd[%d] '%s' %uHz ch%u %ubit fmt%u | first8: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                       si, s->name, s->sampleRate, s->channels, s->bits, s->format,
                       s->data[0],s->data[1],s->data[2],s->data[3],
                       s->data[4],s->data[5],s->data[6],s->data[7]);
                g_debugConsole.Print("Snd[%d] %s: %02X %02X %02X %02X %02X %02X %02X %02X",
                                     si, s->name,
                                     s->data[0],s->data[1],s->data[2],s->data[3],
                                     s->data[4],s->data[5],s->data[6],s->data[7]);
            }
        }
    } else {
        printf("Pak not found - will show error screen\n");
        bool cont = ShowPakErrorScreen();
        if(!cont){ ShutdownD3D(); return 0; }
    }

    g_audio.Init();
    g_render.Init(&g_batch,&g_text,&g_pak);

    MenuSystem menu;
    // v2.23: if a loose "freddy" save is found next to the game, ask before
    // importing it into the XContent container.
    if (Progress::HasLooseSave() && ShowImportSavePrompt()) {
        if (Progress::ImportSave()) {
            ShowImportDonePrompt();
            exit(0);   // v2.23: close so the player restarts fresh with the imported save
        }
    }
    RefreshMenuFromProgress(menu);   // v2.7.13: boot from fnaf_save.bin
    g_ach.Init();                     // v2.14: load achievements (device already chosen)

    // Boot: disclaimer first (original title-frame String obj 0 flow)
    GameState state = GAME_STATE_DISCLAIMER;
    // v2.15: boot fade-in for the disclaimer (1010 ms, FRAME_TRANSITIONS.md)
    g_fade.phase = 2; g_fade.alpha = 1.0f; g_fade.timer = 0.0f; g_fade.dur = 1.010f;
    if(cmdNight!=0){ game.Init(cmdNight); g_ach.BeginNight(cmdNight); state=GAME_STATE_NIGHT_START; g_fade.phase = 0; g_fade.alpha = 0.0f; }

    i32 tickCount=0;
    static int menuFrameCounter = 0;
    static int adCounter = 0;         // v2.15: separate timer for the newspaper screen
    float accumulator=0.0f;
    const float tickDelta = 1.0f/30.0f; // logic 30Hz, render 60Hz
    s_phoneStarted = false;
    f32 scareElapsed = 0.0f;
    i32 endFrames = 0;

    static const CameraId cameraSequence[]={CAM_1A,CAM_1B,CAM_1C,CAM_2A,CAM_2B,CAM_3,CAM_4A,CAM_4B,CAM_5,CAM_6,CAM_7};
    static const i32 cameraSequenceLen=11;

    while(true){
        GameInput gi; UpdateInput(gi);
        bool devToggled = false;   // v2.17: Start+B fired this frame (swallow it)

        // ---------------- PERSPECTIVE TUNER (v2.7.11) ----------------
        // L3+R3 together toggles the tuner (works in every state -- the
        // stick buttons are unused by the game itself). Exiting dumps the
        // final knob values for baking.
        if (gi.tunerToggle) {
            g_tunerMode = !g_tunerMode;
            if (g_tunerMode) {
                g_debugConsole.Print("PERSP tuner ON: DPad sel/adj, A fast, Y reset, L3+R3 exit");
            } else {
                printf("PERSP FINAL: ZOOM=%.1f CENTER_Y=%.1f CURVE=%.2f\n",
                       g_render.PerspTunerValue(0), g_render.PerspTunerValue(1),
                       g_render.PerspTunerValue(2));
                g_debugConsole.Print("PERSP FINAL: ZOOM=%.1f CENTER_Y=%.1f CURVE=%.2f",
                       g_render.PerspTunerValue(0), g_render.PerspTunerValue(1),
                       g_render.PerspTunerValue(2));
            }
        }

        // ---------------- DEV MENU (v2.17) ----------------
        // Start + B toggles it (works in every state). The toggle frame is
        // swallowed in the DEV block below so Start never leaks into the
        // pause handler (which was kicking the game back to the title).
        if (gi.pause && gi.back) {
            g_devMode  = !g_devMode;
            devToggled = true;
            if (g_devMode) g_debugConsole.Print("DEV menu ON");
        }

        g_render.SetLookDir(gi.lookDir);   // office pan window (v2.5)
        g_render.Tick(1.0f/60.0f);
        game.TickDoors(1.0f/60.0f);   // v2.22: door slide (visual, 60 Hz)
        g_audio.Tick();
        g_ach.Tick(1.0f/60.0f);     // v2.14: achievement toast timer
        TickFade(state, 1.0f/60.0f);  // v2.15: advance any running fade (may change `state`)
        if (g_goldenScareT >= 0.0f) {          // v2.17: scare flash timer
            g_goldenScareT += 1.0f/60.0f;
            if (g_goldenScareT > 1.3f) g_goldenScareT = -1.0f;
        }
        if (g_itsmeT >= 0.0f) {          // v2.17: IT'S ME hallucination timer
            g_itsmeT += 1.0f/60.0f;
            if (g_itsmeT > 1.5f) g_itsmeT = -1.0f;
        }

        // v2.16: office ambience starts once on landing in PLAYING, on mixer channels
        // (1:1 with office frame group 15), including the proximity loops that
        // begin MUTED and are ramped by TickAudioMixer.
        if (state == GAME_STATE_PLAYING && !g_officeAmb && g_fade.phase == 0) {
            g_audio.SetChannelVolume(CH_FAN,       CFVolumeToDb(25));
            g_audio.SetChannelVolume(CH_COLDPRESC, CFVolumeToDb(50));
            g_audio.SetChannelVolume(CH_BALLAST,   CFVolumeToDb(50));
            g_audio.PlayOnChannel(&g_pak, Snd::BUZZ_FAN,    true, CH_FAN);
            g_audio.PlayOnChannel(&g_pak, Snd::COLD_PRESC,  true, CH_COLDPRESC);
            g_audio.PlayOnChannel(&g_pak, Snd::BALLAST_HUM, true, CH_BALLAST);
            g_audio.SetChannelVolume(CH_ROBOTVOICE, -100.0f);
            g_audio.PlayOnChannel(&g_pak, Snd::ROBOT_VOICE,    true, CH_ROBOTVOICE);
            g_audio.SetChannelVolume(CH_EERIE, -100.0f);
            g_audio.PlayOnChannel(&g_pak, Snd::EERIE_AMBIENCE, true, CH_EERIE);
            // v2.18: one-shot ambience channel bases (group 284: ch14=50,
            // ch15=5; group 15: ch22=25). One-shots ride their channel volume.
            g_audio.SetChannelVolume(CH_BREATHS,  CFVolumeToDb(50));
            g_audio.SetChannelVolume(CH_CIRCUS,   CFVolumeToDb(5));
            g_audio.SetChannelVolume(CH_MUSICBOX, CFVolumeToDb(25));
            g_audio.SetChannelVolume(CH_PIRATE,   CFVolumeToDb(5));
            g_audio.SetChannelVolume(CH_OVEN,     CFVolumeToDb(30));   // v2.22 kitchen oven
            g_officeAmb = true;
        }
        if (state != GAME_STATE_PLAYING) { g_officeAmb = false; s_phoneStarted = false; }

        // v2.15: title ambience starts exactly when we LAND in MENU (i.e. after
        // the fade-out of the disclaimer / "next day"), not during that fade.
        if (state == GAME_STATE_MENU && !g_menuAmb) {
            g_audio.Stop(Snd::STATIC2); g_audio.Stop(Snd::DARKNESS_MUSIC);
            g_audio.Play(&g_pak, Snd::STATIC2, true, 0.5f);
            g_audio.Play(&g_pak, Snd::DARKNESS_MUSIC, true, 0.6f);
            g_menuAmb = true;
        }
        if (state != GAME_STATE_MENU) g_menuAmb = false;

        // v2.15: 1:1 with the original — the "what day" night-number card plays
        // blip3 exactly when it appears (frame "what day", group 2: start of
        // frame -> Speaker blip3, no fade-in on this card).
        if (state == GAME_STATE_NIGHT_START && !g_nightBlip && g_fade.phase == 0) {
            g_audio.Play(&g_pak, Snd::BLIP, false, 0.8f);
            g_nightBlip = true;
        }
        if (state != GAME_STATE_NIGHT_START) g_nightBlip = false;

        if(state==GAME_STATE_PLAYING){
            TickPhoneCall(game, 1.0f/60.0f);   // phone call belongs to the office, not the ad/card
            TickAudioMixer(game);              // v2.16: dynamic channel volumes each frame
            TickRandomEvents(game);            // v2.18: periodic pirate/breaths/circus one-shots
            TickGoldenFreddy(game, g_render);  // v2.22: Golden Freddy summon/appear/kill
            // v2.17: rare "IT'S ME" Bonnie hallucination (obj "Active 21"):
            // 1/1000 chance every ~20 s (group 419), whisper + full-screen flicker.
            g_itsmeRollTimer += 1.0f/60.0f;
            if (g_itsmeRollTimer >= 20.0f) {
                g_itsmeRollTimer = 0.0f;
                if ((rand() % 1000) == 0) {
                    g_audio.Play(&g_pak, Snd::WHISPERING, false, 0.9f);
                    g_itsmeT = 0.0f;
                }
            }
        }
        if(state==GAME_STATE_POWER_OUT) TickPowerOutFaceSound(game);   // v2.22 garble/digital

        // ---------------- DEBUG SPRITE BROWSER ----------------
        // LB+RB hold enters from menu/disclaimer; DPAD pages; A/B exits.
        if(!g_browserMode && (state==GAME_STATE_MENU || state==GAME_STATE_DISCLAIMER)){
            TickBrowserEntry(gi);
        } else {
            g_browserHold = 0;
        }
        if(g_browserMode){
            if(gi.cameraLeft)  { g_browserPage--; if(g_browserPage < 0) g_browserPage = g_render.SpriteBrowserPageCount()-1; }
            if(gi.cameraRight) { g_browserPage = (g_browserPage+1) % g_render.SpriteBrowserPageCount(); }
            if(gi.cameraToggle || gi.back || gi.pause){
                g_browserMode = false;
                menuFrameCounter = 0;
            } else if(FrameBegin(D3DCOLOR_XRGB(24,24,28))){
                g_render.RenderSpriteBrowser(g_browserPage);
                FrameEnd();
            }
            Sleep(16); continue;
        }

        // ---------------- DEV MENU (v2.17) ----------------
        // Swallow the toggle frame (Start+B) so Start never leaks into the
        // pause handler and kicks the game back to the title.
        if (devToggled) {
            if (g_devMode) {   // just opened
                if (FrameBegin(D3DCOLOR_XRGB(12,12,18))) {
                    g_render.RenderDevMenu(g_devSel, g_devNight,
                                           DEV_ANIM_NAMES[g_devAnim],
                                           DEV_SOUNDS[g_devSound].label,
                                           g_devGod, g_showConsole, g_devAnalogDoor, g_devHoldLights);
                    FrameEnd();
                }
            }
            Sleep(16); continue;
        }
        if(g_devMode){
            if(gi.cameraUp)   { g_devSel = (g_devSel + 10) % 11; }
            if(gi.cameraDown) { g_devSel = (g_devSel + 1) % 11; }
            if(g_devSel == 1){
                if(gi.cameraLeft)  { g_devNight--; if(g_devNight < 1) g_devNight = 1; }
                if(gi.cameraRight) { g_devNight++; if(g_devNight > 7) g_devNight = 7; }
            } else if(g_devSel == 4){
                if(gi.cameraLeft)  g_devAnim = (g_devAnim + 7) % 8;
                if(gi.cameraRight) g_devAnim = (g_devAnim + 1) % 8;
            } else if(g_devSel == 5){
                if(gi.cameraLeft)  g_devSound = (g_devSound + DEV_SOUND_COUNT - 1) % DEV_SOUND_COUNT;
                if(gi.cameraRight) g_devSound = (g_devSound + 1) % DEV_SOUND_COUNT;
            }
            if(gi.cameraToggle){   // A = run the selected action
                switch (g_devSel) {
                    case 0:
                        g_devGod = !g_devGod;
                        game.SetDebugGodMode(g_devGod);
                        g_debugConsole.Print(g_devGod ? "GOD MODE ON" : "GOD MODE OFF");
                        break;
                    case 1:
                        g_audio.StopAll();
                        game.Init(g_devNight);
                        g_ach.BeginNight(g_devNight);
                        s_phoneMuted=false; s_phonePlaying=false; s_phoneStarted=false;
                        s_goldState=0; s_goldInOffice=false; s_goldTimer=0.0f; g_render.SetGoldenFreddyInOffice(false);
                        tickCount=0; accumulator=0;
                        StartTransition(state, GAME_STATE_NIGHT_START);
                        g_devMode = false;
                        break;
                    case 2:
                        game.DebugForceNightComplete();
                        g_devMode = false;
                        break;
                    case 3:
                        game.DebugTriggerPowerOut();
                        g_devMode = false;
                        break;
                    case 4:
                        if (g_devAnim == 4) {
                            // Golden Freddy ("yellow bear"): distinct giggle + face flash
                            g_audio.Play(&g_pak, Snd::FREDDY_LAUGH_LONG, false, 1.0f);
                            g_scareFlashImg = SCARE_IMG_GOLDEN;
                            g_goldenScareT = 0.0f;
                        } else if (g_devAnim == 5) {
                            // Bonnie door-light stare + windowscare sting
                            g_audio.Play(&g_pak, Snd::WINDOW_SCARE, false, 1.0f);
                            g_scareFlashImg = SCARE_IMG_BONNIE_W;
                            g_goldenScareT = 0.0f;
                        } else if (g_devAnim == 6) {
                            // Chica door-light stare + windowscare sting
                            g_audio.Play(&g_pak, Snd::WINDOW_SCARE, false, 1.0f);
                            g_scareFlashImg = SCARE_IMG_CHICA_W;
                            g_goldenScareT = 0.0f;
                        } else if (g_devAnim == 7) {
                            // "IT'S ME" Bonnie hallucination (obj "Active 21")
                            g_audio.Play(&g_pak, Snd::WHISPERING, false, 0.9f);
                            g_itsmeT = 0.0f;
                        } else {
                            game.DebugTriggerJumpscare(DEV_ANIMS[g_devAnim]);
                            StartTransition(state, GAME_STATE_JUMPSCARE);   // actually render the scare
                        }
                        g_devMode = false;
                        break;
                    case 5:
                        g_audio.Play(&g_pak, DEV_SOUNDS[g_devSound].snd, false, 0.8f);
                        g_debugConsole.Print("play: %s", DEV_SOUNDS[g_devSound].label);
                        break;
                    case 6:
                        g_ach.UnlockAll();
                        g_debugConsole.Print("Achievements unlocked");
                        break;
                    case 7:
                        g_ach.ClearAll();
                        g_debugConsole.Print("Achievements cleared");
                        break;
                    case 8:
                        g_showConsole = !g_showConsole;
                        g_debugConsole.Print(g_showConsole ? "Console ON" : "Console OFF");
                        break;
                    case 9:
                        g_devAnalogDoor = !g_devAnalogDoor;
                        g_debugConsole.Print(g_devAnalogDoor ? "ANALOG DOOR ON" : "ANALOG DOOR OFF");
                        break;
                    case 10:
                        g_devHoldLights = !g_devHoldLights;
                        g_debugConsole.Print(g_devHoldLights ? "HOLD LIGHTS ON" : "HOLD LIGHTS OFF");
                        break;
                }
            }

            // v2.17: B or Y closes the DEV menu. (This is safe on the open frame
            // too -- the devToggled swallow above skips this block when Start+B
            // just opened it, so the combo's B doesn't instantly close it.)
            if(gi.back || gi.yToggle){ g_devMode = false; }

            if(FrameBegin(D3DCOLOR_XRGB(12,12,18))){
                g_render.RenderDevMenu(g_devSel, g_devNight,
                                       DEV_ANIM_NAMES[g_devAnim],
                                       DEV_SOUNDS[g_devSound].label,
                                       g_devGod, g_showConsole, g_devAnalogDoor, g_devHoldLights);
                FrameEnd();
            }
            Sleep(16); continue;
        }

        // ---------------- DISCLAIMER ----------------
        // v2.15: original auto-advances on a ~40 s timer (no Start) and fades
        // out 1010 ms into the title; any button skips once the short lock ends.
        if(state==GAME_STATE_DISCLAIMER){
            menuFrameCounter++;
            const bool bootLock = menuFrameCounter < 30;   // short boot bounce lock
            if(FrameBegin(D3DCOLOR_XRGB(0,0,0))){
                g_render.RenderDisclaimer(false);          // no "PRESS START" hint
                DrawFadeOverlay();
                FrameEnd();
            }
            if(g_fade.phase == 0 && ((!bootLock && (gi.pause||gi.cameraToggle||gi.back)) || menuFrameCounter >= 2400)){
                StartTransition(state, GAME_STATE_MENU);   // ambience starts after the fade-out
                menuFrameCounter=0;
            }
            Sleep(16); tickCount++; continue;
        }


        // ---------------- TITLE MENU ----------------
        if(state==GAME_STATE_MENU){
            if(menuFrameCounter < 30){ menuFrameCounter++; }
            PollTitleKeyboardReset(menu);   // hidden Delete-key save wipe (original title events)

            MenuAction act = MENU_ACTION_NONE;
            // v2.14/v2.20: achievements — Y opens the SYSTEM list in the system build
            // (XShowAchievementsUI); the in-game screen is the Live Safe fallback
            // (ShowSystemUI() returns false there).
            if(g_achScreen){
                if(gi.back || gi.yToggle) g_achScreen = false;
            } else {
                if(gi.yToggle && !g_ach.ShowSystemUI()) g_achScreen = true;
                MenuInput mi; mi.up=gi.cameraUp; mi.down=gi.cameraDown; mi.left=gi.cameraLeft; mi.right=gi.cameraRight;
                mi.confirm=gi.cameraToggle; mi.back=gi.back;
                if(gi.lookDir < -0.5f) mi.left=true;
                if(gi.lookDir > 0.5f) mi.right=true;
                act=menu.Update(mi);
                if(act!=MENU_ACTION_NONE){ g_audio.Play(&g_pak,Snd::BLIP,false,0.8f); }
            }

            if(FrameBegin(D3DCOLOR_XRGB(0,0,0))){
                if(g_achScreen){
                    g_render.RenderAchievements(g_ach);
                } else {
                    g_render.RenderTitle(menu, menu.HasSave(), Progress::StarCount(g_prog));   // v2.7.13
                    if(menu.GetScreen()!=MENU_MAIN) menu.Render(&g_text, SCREEN_W, SCREEN_H);
                }
                DrawFadeOverlay();
                FrameEnd();
            }

            if(!g_achScreen){
                if(act==MENU_ACTION_START_NIGHT){
                    i32 night=menu.GetSelectedNight();
                    g_audio.Stop(Snd::STATIC2); g_audio.Stop(Snd::DARKNESS_MUSIC);
                    game.Init(night);
                    g_ach.BeginNight(night);   // v2.14: reset per-night achievement flags
                    s_phoneMuted=false; s_phonePlaying=false; s_phoneStarted=false;
                        s_goldState=0; s_goldInOffice=false; s_goldTimer=0.0f; g_render.SetGoldenFreddyInOffice(false);
                    tickCount=0; accumulator=0; menuFrameCounter=0; adCounter=0;
                    // v2.7.13: New Game shows the "HELP WANTED" newspaper first
                    StartTransition(state, menu.LastStartWasNewGame() ? GAME_STATE_INTRO_AD
                                                                      : GAME_STATE_NIGHT_START);
                }
                else if(act==MENU_ACTION_OPEN_DEV){ g_devMode = true; }   // v2.17: hidden title entry
                else if(act==MENU_ACTION_EXIT) break;
            }
            Sleep(16); tickCount++; continue;
        } else {
            menuFrameCounter = 0;
        }

        // ---------------- INTRO AD ("HELP WANTED", v2.7.13) ----------------
        // New Game only: the newspaper (frame "ad", img_574) holds ~8 s,
        // any button skips, then the night-1 card. Uses its own counter:
        // menuFrameCounter is reset every frame by the menu block above.
        if(state==GAME_STATE_INTRO_AD){
            ++adCounter;
            const bool adLock = adCounter < 30;   // skip boot bounce
            if(FrameBegin(D3DCOLOR_XRGB(0,0,0))){
                g_render.RenderIntroAd(!adLock && (((adCounter/30)%2)==0));
                DrawFadeOverlay();
                FrameEnd();
            }
            const bool adTimeout = adCounter > 480;   // ~8 s
            const bool adSkip = !adLock &&
                (gi.cameraToggle||gi.pause||gi.back||gi.cameraUp||gi.cameraDown);
            if(g_fade.phase == 0 && (adTimeout || adSkip)){
                StartTransition(state, GAME_STATE_NIGHT_START);
                tickCount=0; accumulator=0;
            }
            Sleep(16); tickCount++; continue;
        }

        // ---------------- NIGHT START (title card + phone) ----------------
        if(state==GAME_STATE_NIGHT_START){
            if(FrameBegin(D3DCOLOR_XRGB(4,4,10))){
                g_render.RenderNightStart(game.GetCurrentNight());
                DrawFadeOverlay();
                FrameEnd();
            }
            if(gi.back && s_phonePlaying){           // MUTE CALL
                for(int v=0;v<5;++v) g_audio.Stop(Snd::VOICEOVER[v]);
                s_phonePlaying=false; s_phoneMuted=true;
            }
            // advance the card timer through the game's own state machine;
            // freeze while a fade-out is running so game.Tick() isn't re-run.
            if(g_fade.phase != 1){
                accumulator += 1.0f/60.0f;
                while(accumulator >= tickDelta){
                    GameState ns = game.Tick();
                    accumulator -= tickDelta;
                    if(ns != GAME_STATE_NIGHT_START){
                        StartTransition(state, ns);   // fade-out "what day" -> office
                        break;
                    }
                }
            }
            Sleep(16); tickCount++; continue;
        }

        // ---------------- GAMEPLAY INPUT ----------------
        if(state==GAME_STATE_PLAYING && g_tunerMode){
            // v2.7.11: the tuner owns the pad while it is open. DPad edges
            // (already debounced in UpdateInput) select/adjust the knobs;
            // A = fast step; Y resets the selected knob to the serialized
            // default. Gameplay input is swallowed until L3+R3 again.
            if(gi.cameraLeft)  g_tunerSel = (g_tunerSel + fnaf::GameRender::PERSP_TUNER_KNOBS - 1)
                                           % fnaf::GameRender::PERSP_TUNER_KNOBS;
            if(gi.cameraRight) g_tunerSel = (g_tunerSel + 1)
                                           % fnaf::GameRender::PERSP_TUNER_KNOBS;
            if(gi.cameraUp)    g_render.PerspTunerAdjust(g_tunerSel, +1, gi.cameraToggle);
            if(gi.cameraDown)  g_render.PerspTunerAdjust(g_tunerSel, -1, gi.cameraToggle);
            if(gi.yToggle)     g_render.PerspTunerReset(g_tunerSel);
        }
        else if(state==GAME_STATE_PLAYING){
            // v2.21 analog-door test (DEV toggle). When ON, the door position
            // follows the trigger level (0 open .. 1 closed); the logical
            // "closed" (AI block) = amount >= 0.5, so gameplay rules hold.
            // When OFF, the original LT/RT toggle behaviour is used.
            if (g_devAnalogDoor) {
                game.SetDoorAmount(DOOR_LEFT,  gi.leftDoorAxis);
                game.SetDoorAmount(DOOR_RIGHT, gi.rightDoorAxis);
            } else {
                if(gi.leftDoorToggle) game.ToggleDoor(DOOR_LEFT);
                if(gi.rightDoorToggle) game.ToggleDoor(DOOR_RIGHT);
            }
            // v2.21 hold-lights test (DEV toggle). When ON, the light stays on only
            // while its bumper (LB/RB) is held; when OFF, the original toggle.
            if (g_devHoldLights) {
                game.SetLight(DOOR_LEFT,  gi.leftShoulderHeld);
                game.SetLight(DOOR_RIGHT, gi.rightShoulderHeld);
            } else {
                if(gi.leftLightToggle) game.ToggleLight(DOOR_LEFT);
                if(gi.rightLightToggle) game.ToggleLight(DOOR_RIGHT);
            }
            if(gi.cameraToggle || gi.back){
                if(game.GetCameras().IsMonitorUp() && gi.back) game.SetCameraUp(false);
                else game.ToggleCamera();
            }
            if(game.GetCameras().IsMonitorUp() && (gi.cameraUp||gi.cameraDown||gi.cameraLeft||gi.cameraRight)){
                CameraId cur=game.GetCameras().GetCurrentCamera();
                int idx=-1; for(int i=0;i<cameraSequenceLen;++i) if(cameraSequence[i]==cur) idx=i;
                if(idx>=0){
                    if(gi.cameraUp||gi.cameraLeft) idx=(idx-1+cameraSequenceLen)%cameraSequenceLen;
                    if(gi.cameraDown||gi.cameraRight) idx=(idx+1)%cameraSequenceLen;
                    game.SwitchCamera(cameraSequence[idx]);
                }
            }
            // v2.22: Freddy nose honk easter egg (group 349, click "Active 26").
            // The official console maps it to Y; gated to the office (monitor down)
            // since the poster/nose is an office object.
            if(gi.yToggle && !game.GetCameras().IsMonitorUp())
                g_audio.Play(&g_pak, Snd::PARTY_FAVOR, false, 0.9f);
            if(gi.pause){
                if(s_phonePlaying){ for(int v=0;v<5;++v) g_audio.Stop(Snd::VOICEOVER[v]); s_phonePlaying=false; }
                // stop the office ambience when leaving to the menu, else it keeps
                // playing and doubles on resume (the mixer re-starts it for the office)
                g_audio.Stop(Snd::COLD_PRESC); g_audio.Stop(Snd::BUZZ_FAN);
                g_audio.Stop(Snd::BALLAST_HUM); g_audio.Stop(Snd::ROBOT_VOICE);
                g_audio.Stop(Snd::EERIE_AMBIENCE);
                StartTransition(state, GAME_STATE_MENU); menu.Reset();
                continue;
            }
            if(gi.back && s_phonePlaying && !game.GetCameras().IsMonitorUp()){
                // MUTE CALL during gameplay
                for(int v=0;v<5;++v) g_audio.Stop(Snd::VOICEOVER[v]);
                s_phonePlaying=false; s_phoneMuted=true;
            }
        }

        // ---------------- LOGIC TICK (30 Hz) ----------------
        if(g_fade.phase != 1){
            accumulator += 1.0f/60.0f;
            while(accumulator >= tickDelta){
                GameState ns = game.Tick();
                accumulator -= tickDelta;
                tickCount++;
                if(ns != GAME_STATE_PLAYING){
                    if(ns != state) StartTransition(state, ns);   // fade-in next-day/game-over
                    break;
                }
            }
        }

        // ---------------- RENDER 60 Hz ----------------
        if(state!=GAME_STATE_JUMPSCARE) scareElapsed = 0.0f;
        if(FrameBegin(ColorForState(state, game.GetPower().GetPower()))){
            if(state==GAME_STATE_JUMPSCARE){
                g_render.RenderJumpscare(game.GetJumpscareAnimatronic(), scareElapsed);
                scareElapsed += 1.0f/60.0f;   // v2.18: advance AFTER the first frame renders (start on frame 0)
            } else if(state==GAME_STATE_POWER_OUT){
                g_render.RenderPowerOut(game);
            } else if(state==GAME_STATE_NIGHT_COMPLETE){
                // v2.7.13: elapsed drives the data "6" roll (nights 1-4)
                g_render.RenderNightComplete(game.GetCurrentNight(), endFrames/60.0f);
            } else if(state==GAME_STATE_GAME_OVER){
                if(endFrames<96){
                    // v2.7.13: "died" static burst (1.6 s) before the backroom
                    g_render.DrawStaticOverlay(1.0f);
                } else {
                    g_render.RenderGameOver();
                }
            } else if(game.GetCameras().IsMonitorUp()){
                g_render.RenderCamera(game, s_phonePlaying);
            } else {
                g_render.RenderOffice(game, s_phonePlaying);
            }
            // v2.7.11: tuner HUD on top of the bent scene (office/monitor)
            if(g_tunerMode && state==GAME_STATE_PLAYING){
                g_render.RenderPerspTuner(g_tunerSel);
            }
            // v2.14: achievement-unlocked toast, drawn over any screen
            if(g_ach.HasToast()){
                g_render.DrawAchievementToast(g_ach.ToastName(), g_ach.ToastGamerscore());
            }
            DrawFadeOverlay();
            FrameEnd();
        }

        if(tickCount % 60==0 && state==GAME_STATE_PLAYING){
            g_debugConsole.Print("[%s] Power: %5.1f%% Usage:%d Tick:%d",
                game.GetTimer().GetHourString(), game.GetPower().GetPower(),
                game.GetPower().GetUsageLevel(), tickCount);
        }

        if(g_fade.phase != 1 && (state==GAME_STATE_NIGHT_COMPLETE || state==GAME_STATE_GAME_OVER)){
            // v2.7.13 night-flow routing (frames "next day" / "the end")
            ++endFrames;
            // kids cheer when the "6" lands on the clock (~1 s, nights 1-4)
            if(state==GAME_STATE_NIGHT_COMPLETE && endFrames==60 && game.GetCurrentNight()<5){
                g_audio.Play(&g_pak,Snd::CROWD_KIDS,false,0.8f);
            }
            const f32  holdSec = (f32)endFrames / 60.0f;
            const bool skipEnd = endFrames>45 &&
                (gi.cameraToggle||gi.pause||gi.back||gi.cameraUp||gi.cameraDown);
            bool done=false;
            if(state==GAME_STATE_NIGHT_COMPLETE){
                const i32 c = game.GetCurrentNight();
                if(c<5) done = (holdSec >= (f32)TimeConstants::NIGHT_COMPLETE_DISPLAY_SEC) || skipEnd;
                else    done = (holdSec >= 12.0f) || skipEnd;  // paycheck/overtime/pink slip
            } else {
                done = (holdSec >= 7.6f) || skipEnd;           // 1.6 static + 6.0 backroom
            }
            if(done){
                endFrames=0;
                if(state==GAME_STATE_NIGHT_COMPLETE && game.GetCurrentNight()<5){
                    // nights 1-4: straight into the next night card
                    game.Init(game.GetCurrentNight()+1);
                    g_ach.BeginNight(game.GetCurrentNight()+1);   // v2.14
                    s_phoneMuted=false; s_phonePlaying=false; s_phoneStarted=false;
                        s_goldState=0; s_goldInOffice=false; s_goldTimer=0.0f; g_render.SetGoldenFreddyInOffice(false);
                    StartTransition(state, GAME_STATE_NIGHT_START); tickCount=0; accumulator=0;
                    continue;
                }
                // v2.23: progress flags ONLY on a 6 AM screen (NIGHT_COMPLETE).
                // A game-over previously fell through here and wrongly awarded
                // the night-5/6/7 star on death.
                if(state==GAME_STATE_NIGHT_COMPLETE)
                {
                    i32 night = game.GetCurrentNight();
                    Progress::Load(g_prog);
                    if (night == 5) {
                        g_prog.beat5 = true;
                    } else if (night == 6) {
                        g_prog.beat6 = true;
                    } else if (night == 7) {
                        bool perfect = false;
                        if (g_gameRef) {
                            const fnaf::AnimatronicAI& ai = g_gameRef->GetAI();
                            perfect = (ai.GetAILevel(fnaf::ANIM_FREDDY) == 20) &&
                                      (ai.GetAILevel(fnaf::ANIM_BONNIE) == 20) &&
                                      (ai.GetAILevel(fnaf::ANIM_CHICA)  == 20) &&
                                      (ai.GetAILevel(fnaf::ANIM_FOXY)   == 20);
                        }
                        if (perfect) g_prog.beat7 = true;
                    }
                    Progress::Save(g_prog);
                }
                g_audio.StopAll();
                RefreshMenuFromProgress(menu);   // unlocks + stars from the save
                StartTransition(state, GAME_STATE_MENU); menu.Reset(); tickCount=0; accumulator=0;
                continue;
            }
        }
        Sleep(16);
        if(tickCount>36000) break;
    }
    ShutdownD3D();
    return 0;
}
