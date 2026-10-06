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
#include "AppRegistry.h"  // v2.28: AppModule contract + module table
#include "AppModules.h"   // v2.28: full AppModule type (banner prints Name/PakName)

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
static f32               g_itsmeRollTimer = 0.0f; // v2.53: 1 s accumulator for the IT'S ME roll (dump g419)
static f32               g_creepyT = -1.0f;      // v2.36: post-game-over "creepy start" (f14) timer (-1 = off)
static int               s_clickCooldown = 0;    // v2.53: 10-tick door/light anti-mash (dump g95)
static int               s_camHighlight = -1;    // v2.62: pre-selected cam (CameraId) while the monitor is up (-1 = none)

// v2.37: pad RUMBLE — two motors (left = low-freq thump, right = high-freq
// buzz). XInputSetState with a linear-decay envelope (pattern: the scare-
// flash timers). Kicks are one-shot; the last kick wins; an empty envelope
// writes zeros every frame (safe in menus/pause).
static DWORD s_rumL = 0, s_rumR = 0;
static DWORD s_rumStartL = 0, s_rumStartR = 0;
static f32   s_rumT = 0.0f, s_rumDur = 0.0f;

static void RumbleKick(DWORD left, DWORD right, f32 seconds) {
    if (seconds <= 0.0f) return;
    if (seconds > 5.0f) seconds = 5.0f;         // safety cap
    s_rumStartL = left; s_rumStartR = right;
    s_rumL = left;     s_rumR = right;
    s_rumDur = seconds; s_rumT = seconds;
}

static void TickRumble(f32 dt) {
    XINPUT_VIBRATION vib;
    vib.wLeftMotorSpeed  = (WORD)s_rumL;   // DWORD -> WORD (safe: <= 65535 cap)
    vib.wRightMotorSpeed = (WORD)s_rumR;
    if (s_rumT > 0.0f) {
        s_rumT -= dt;
        if (s_rumT <= 0.0f) {
            s_rumL = s_rumR = 0;                // dead silence after the kick
            vib.wLeftMotorSpeed = 0; vib.wRightMotorSpeed = 0;
        } else {
            const f32 f = s_rumT / s_rumDur;    // linear decay
            s_rumL = (DWORD)((f32)s_rumStartL * f);
            s_rumR = (DWORD)((f32)s_rumStartR * f);
            vib.wLeftMotorSpeed  = (WORD)s_rumL;
            vib.wRightMotorSpeed = (WORD)s_rumR;
        }
    }
    XInputSetState(0, &vib);
}

// Game reference for callbacks needing state
static Game* g_gameRef = nullptr;

static const i32 SCREEN_W = 1280;
static const i32 SCREEN_H = 720;

// v2.34/2.35: message-box helper — SysPrompt wraps XShowMessageBoxUI with
// return-code logging and a plain blocking wait. Real-HW evidence: the
// system box always appears (the FIRST call is slow because XAM cold-
// starts its XUI), so a software fallback was tried (v2.34) and removed
// again (v2.35) — the box itself is the 1:1 UI.
static bool SysPrompt(const wchar_t* title, const wchar_t* text,
                      const wchar_t** buttons, DWORD nButtons,
                      DWORD focus, DWORD flags, DWORD* pressedOut);

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
    DWORD pressed = 0;
    SysPrompt(wTitle, wText, awszButtons, 1, 0, XMB_ERRORICON, &pressed);
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
        case GAME_STATE_CUSTOMIZE: return D3DCOLOR_XRGB(0, 0, 0);   // v2.46
        default: return D3DCOLOR_XRGB(0,0,0);
    }
}
// v2.17: DEV/debug menu flag (Start + B) — defined HERE because FrameEnd's
// console gate reads it (the definition used to sit below its first use).
static bool g_devMode  = false;
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
    // v2.60: the on-screen console draws on FNAF1 (its log sink) or when the
    // DEV menu is open — the FNAF2+ titles render clean (user call: the boot
    // log was drawn over the FNAF2 menu).
    {
        AppModule* act = AppRegistry_Active();
        const bool fnaf1Flow = (act == 0) || (strcmp(AppRegistry_Active()->Name(), "FNAF1") == 0);
        if (g_showConsole && (fnaf1Flow || g_devMode))
            g_debugConsole.Render(SCREEN_W, SCREEN_H);
    }
    if (g_goldenScareT >= 0.0f) g_render.RenderScareFlash(g_scareFlashImg, g_goldenScareT);   // scare flash
    // v2.27: IT'S ME hallucination — visible only on ~1-in-10 frames while
    // the window is open (dump groups 413/416-418: Random(10)==1 per frame
    // gates the SHOW; anything else HIDEs it). That per-frame chatter is
    // what reads as "rapid flicker" in the original.
    if (g_itsmeT >= 0.0f && (rand() % 10) == 0) g_render.RenderItsmeFlash(g_itsmeT);
    // Flush ALL queued quads (sprites AND text) before ending the scene --
    // without this the last same-texture batch renders one frame late
    // (or not at all for static screens).
    g_batch.End();
    g_pd3dDevice->EndScene();
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

// v2.34/2.41: one XShowMessageBoxUI call with logging; returns
// true only if the box actually completed (pressedOut valid).
// v2.41: NO timeout cap, and the FRAME STREAM KEEPS RUNNING while the
// box is pending. The box is asynchronous and XAM composites it over
// the title's frame chain — if the title stops presenting (the bare
// Sleep wait of v2.35+), the box has nothing to composite over and
// only the sound plays ("sound only, no window"). The v2.34-era
// software fallback unintentionally kept frames flowing, which is why
// the box worked back then. A busy UI (ERROR_ACCESS_DENIED, e.g.
// another system screen right after boot) is retried, not fatal.
static bool SysPrompt(const wchar_t* title, const wchar_t* text,
                      const wchar_t** buttons, DWORD nButtons,
                      DWORD focus, DWORD flags, DWORD* pressedOut) {
    MESSAGEBOX_RESULT result;
    XOVERLAPPED overlapped;

    DWORD dwRet = ERROR_ACCESS_DENIED;
    for (int attempt = 0; attempt < 40; ++attempt) {
        ZeroMemory(&overlapped, sizeof(XOVERLAPPED));
        dwRet = XShowMessageBoxUI(XUSER_INDEX_ANY, title, text, nButtons,
                                  buttons, focus, flags, &result, &overlapped);
        if (dwRet != ERROR_ACCESS_DENIED) break;
        printf("XMB: UI busy (ACCESS_DENIED), retry %d/40\n", attempt + 1);
        g_debugConsole.Print("XMB: UI busy, retry %d/40", attempt + 1);
        Sleep(250);
    }
    printf("XMB: ret=0x%08X\n", dwRet);
    g_debugConsole.Print("XMB: ret=0x%08X", dwRet);

    if (dwRet == ERROR_IO_PENDING) {
        // Blocking wait, but frames keep flowing: pump a black frame every
        // iteration so XAM can composite the dialog over it. The heartbeat
        // every 5 s measures the real cold-start time on this console.
        DWORD waitedMs = 0;
        DWORD lastBeat = 0;
        while (!XHasOverlappedIoCompleted(&overlapped)) {
            if (FrameBegin(D3DCOLOR_XRGB(0, 0, 0))) FrameEnd();
            Sleep(16);
            waitedMs += 16;
            if (waitedMs - lastBeat >= 5000) {
                lastBeat = waitedMs;
                printf("XMB: waiting %u s for the box...\n", (unsigned)(waitedMs / 1000));
                g_debugConsole.Print("XMB: waiting %u s...", (unsigned)(waitedMs / 1000));
            }
        }
        DWORD dwRes = XGetOverlappedResult(&overlapped, NULL, TRUE);
        printf("XMB: res=0x%08X btn=%lu\n", dwRes, result.dwButtonPressed);
        g_debugConsole.Print("XMB: res=0x%08X btn=%lu", dwRes, result.dwButtonPressed);
        if (dwRes != ERROR_SUCCESS) return false;
    } else if (dwRet != ERROR_SUCCESS) {
        return false;
    }
    if (pressedOut) *pressedOut = result.dwButtonPressed;
    return true;
}

// v2.35: a clean exit to the Xbox dashboard. exit(0) on XDK tears the
// process down abruptly (the kernel logs threads dying with code 0 — the
// "core falls" the user saw); XLaunchNewImage(NULL) relaunches the system
// dashboard instead. Exception: Golden Freddy's force-close keeps exit(0)
// on purpose (it mirrors the original's abrupt close).
static void ExitToDashboard() {
    XLaunchNewImage(NULL, NULL);
}

static bool ShowPakErrorScreen(){
    static bool shown = false;
    if (shown) { ExitToDashboard(); return false; }
    shown = true;
    LPCWSTR wszTitle = L"fnaf1.pak NOT FOUND";
    LPCWSTR wszText  = L"The game requires fnaf1.pak\nPlace it next to the .xex";
    LPCWSTR awszButtons[] = { L"OK" };
    DWORD pressed = 0;
    SysPrompt(wszTitle, wszText, awszButtons, 1, 0, XMB_ERRORICON, &pressed);
    ExitToDashboard();
    return false;
}

// v2.23: ask whether to import a loose "freddy" save found next to the game
// into the save storage. Same async pattern as ShowPakErrorScreen.
static bool ShowImportSavePrompt(){
    LPCWSTR wszTitle = L"Import save";
    LPCWSTR wszText  = L"Do you want to import the save found in the game folder?";
    LPCWSTR awszButtons[] = { L"No", L"Yes" };

    // v2.34: system box first (1:1 feel on real HW); if it failed to show,
    // the software prompt still lets the player answer (works on Xenia too)
    DWORD pressed = 0;
    SysPrompt(wszTitle, wszText, awszButtons, 2, 1, XMB_WARNINGICON, &pressed);
    return pressed == 1;                   // 1 == "Yes"
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
    // v2.53 (dump groups 114-129/122/326): the ballast hum is SILENT in a
    // dark office and FULL while a door light is lit, with a ~1/10 dark-frame
    // strobe (the old port code had this law inverted). Entering the cams
    // kills the lights, which zeroes it through the same branch.
    {
        const bool litAny = lightL || lightR;
        const bool strobeOut = (rand() % 10) == 0;
        g_audio.SetChannelVolume(CH_BALLAST,
            (litAny && !strobeOut) ? CFVolumeToDb(100) : -100.0f);
    }
    // phone: 100 office / 50 viewing / 0 mute (groups 360/361/379)
    g_audio.SetChannelVolume(CH_PHONE, s_phoneMuted ? -100.0f : CFVolumeToDb(monUp ? 50 : 100));
    // pirate song2: 15 watching the cove (CAM 1C), 5 otherwise (groups 274/275)
    g_audio.SetChannelVolume(CH_PIRATE,
        CFVolumeToDb((game.GetCameras().GetCurrentCamera() == CAM_1C) ? 15 : 5));

    const AnimatronicAI& ai = game.GetAI();
    const RoomId bonnie = ai.GetAnimatronic(ANIM_BONNIE).currentRoom;
    const RoomId chica  = ai.GetAnimatronic(ANIM_CHICA).currentRoom;
    const RoomId freddy = ai.GetAnimatronic(ANIM_FREDDY).currentRoom;

    // v2.53 EERIE dread ladder (groups 351-359): presence units =
    // Bonnie@west-corner zone + Chica@east-corner zone + Foxy progress>=2;
    // none -> OFF; one -> 30; two -> 50; three -> 75; Freddy inside -> 100.
    // v2.62 EQUIVALENCE PROOF (closes the old "heuristic" caveat): the dump
    // tests bonnie/chica SPRITES overlapping the invisible "vol zone"
    // (objInfo 136, parked at the doorways) and foxy progress <2/>=2, per
    // groups 351-358 -> volumes 0/30/50/75 exactly when 0/1/2/3 units are
    // present — the additive model reproduces the table row-for-row, and a
    // sprite sits in the doorway zone exactly when its AI room is the
    // corner/door room, so the room test IS the zone test. Freddy-in -> 100
    // (group 359). No behavioral gap remains.
    {
        i32 dread = 0;
        if (bonnie == ROOM_WEST_HALL_CORNER || bonnie == ROOM_LEFT_DOOR)  dread++;
        if (chica  == ROOM_EAST_HALL_CORNER || chica  == ROOM_RIGHT_DOOR) dread++;
        if (ai.GetAnimatronic(ANIM_FOXY).foxyStage >= FOXY_STAGE_2)       dread++;
        float db;
        if (game.IsFreddyInOffice())  db = CFVolumeToDb(100);
        else if (dread == 0)          db = -100.0f;
        else if (dread == 1)          db = CFVolumeToDb(30);
        else if (dread == 2)          db = CFVolumeToDb(50);
        else                          db = CFVolumeToDb(75);
        g_audio.SetChannelVolume(CH_EERIE, db);
    }

    // v2.53 robotvoice corner-mumble (groups 381-385/416): silent before
    // night 4; lives only while Bonnie@2B-zone / Chica@4B-zone; office reads
    // 1+5*Random(5), watching their cam 1+20*Random(5) — both re-rolled every
    // 100 ms; the ITSME flash pins 100.
    {
        static f32 s_robT = 0.0f;
        const f32 rdt = 1.0f / 60.0f;
        s_robT += rdt;
        if (g_itsmeT >= 0.0f) {
            g_audio.SetChannelVolume(CH_ROBOTVOICE, CFVolumeToDb(100));
        } else if (s_robT >= 0.100f) {
            s_robT -= 0.100f;
            const bool zoneB = (bonnie == ROOM_WEST_HALL_CORNER || bonnie == ROOM_LEFT_DOOR);
            const bool zoneC = (chica  == ROOM_EAST_HALL_CORNER || chica  == ROOM_RIGHT_DOOR);
            if (game.GetCurrentNight() >= 4 && (zoneB || zoneC)) {
                const RoomId zroom = zoneB ? ROOM_WEST_HALL_CORNER : ROOM_EAST_HALL_CORNER;
                const bool watch = game.IsWatchingRoom(zroom) ||
                                   game.IsWatchingRoom(zoneB ? ROOM_LEFT_DOOR : ROOM_RIGHT_DOOR);
                const i32 v = watch ? 1 + 20 * (rand() % 5) : 1 + 5 * (rand() % 5);
                g_audio.SetChannelVolume(CH_ROBOTVOICE, CFVolumeToDb(v));
            } else {
                g_audio.SetChannelVolume(CH_ROBOTVOICE, -100.0f);
            }
        }
    }

    // v2.53 kitchen ladder (groups 251-258) + the 300 s camp replay (400):
    //   clatter ch10: 0 absent / 10 office / 20 some other cam / 75 on CAM 6
    //   tune    ch22: 0 absent / 5 office / 5 other cam / 50 on CAM 6
    {
        static f32 s_kitchenCampT = 0.0f;
        const bool chicaKitchen = (chica == ROOM_KITCHEN);
        const bool freddKitchen = (freddy == ROOM_KITCHEN);
        const bool watchKitchen = game.IsWatchingRoom(ROOM_KITCHEN);
        i32 ovenDb, tuneDb;
        if (!chicaKitchen)     ovenDb = 0;
        else if (watchKitchen) ovenDb = 75;
        else if (monUp)        ovenDb = 20;
        else                   ovenDb = 10;
        if (!freddKitchen)     tuneDb = 0;
        else if (watchKitchen) tuneDb = 50;
        else                   tuneDb = 5;
        g_audio.SetChannelVolume(CH_OVEN,     CFVolumeToDb(ovenDb));
        g_audio.SetChannelVolume(CH_MUSICBOX, CFVolumeToDb(tuneDb));
        // music box replays every 300 s while Freddy camps the kitchen (g400)
        if (freddKitchen) {
            s_kitchenCampT += 1.0f / 60.0f;
            if (s_kitchenCampT >= 300.0f) {
                s_kitchenCampT = 0.0f;
                g_audio.PlayOnChannel(&g_pak, Snd::MUSIC_BOX, false, CH_MUSICBOX);
            }
        } else s_kitchenCampT = 0.0f;
    }
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

    // v2.53 (dump groups 269/270/276/245): these fire far more often than the
    // port used to think — the dump's "(~Ns)" annotations are the 50 Hz
    // artifact; the raw numbers (4000/5000 ms) are the truth.
    s_pirateT += dt;
    if (s_pirateT >= 4.0f) {                    // g269: every 4 s, 1/30
        s_pirateT = 0.0f;
        if (foxy.foxyStage == FOXY_STAGE_0 && (rand() % 30) == 0)
            g_audio.PlayOnChannel(&g_pak, Snd::PIRATE_SONG, false, CH_PIRATE);
    }

    s_circusT += dt;
    if (s_circusT >= 5.0f) {                    // g270: every 5 s, 1/30
        s_circusT = 0.0f;
        if ((rand() % 30) == 0)
            g_audio.PlayOnChannel(&g_pak, Snd::CIRCUS, false, CH_CIRCUS);
    }

    s_breathT += dt;
    if (s_breathT >= 5.0f) {                    // g276/278: every 5 s, 1/3
        s_breathT = 0.0f;
        if (monUp) {
            if (bonnie.currentRoom == ROOM_LEFT_DOOR  && (rand() % 3) == 0)
                g_audio.PlayOnChannel(&g_pak, Snd::BREATHS[rand() % 4], false, CH_BREATHS);
            if (chica.currentRoom  == ROOM_RIGHT_DOOR && (rand() % 3) == 0)
                g_audio.PlayOnChannel(&g_pak, Snd::BREATHS[rand() % 4], false, CH_BREATHS);
        }
    }

    // kitchen oven (groups 245-250): Chica in the kitchen re-rolls the clatter
    // pick every 4 s (a raw 4000 ms timer, not 80).
    static f32 s_ovenT = 0.0f;
    s_ovenT += dt;
    if (s_ovenT >= 4.0f) {
        s_ovenT = 0.0f;
        if (chica.currentRoom == ROOM_KITCHEN && (rand() % 10) < 5)
            g_audio.PlayOnChannel(&g_pak, Snd::OVEN_DRAW[rand() % 4], false, CH_OVEN);
    }

    // v2.53 (group 271): the building's rare pounding — every 10 s, 1/50,
    // volume 10 + Random(40). It used to be stolen for Foxy's bang.
    static f32 s_poundT = 0.0f;
    s_poundT += dt;
    if (s_poundT >= 10.0f) {
        s_poundT = 0.0f;
        if ((rand() % 50) == 0) {
            g_audio.SetChannelVolume(CH_POUNDING, CFVolumeToDb(10 + rand() % 41));
            g_audio.PlayOnChannel(&g_pak, Snd::DOOR_POUNDING, false, CH_POUNDING);
        }
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
static f32   s_goldItsmeT = 0.0f;   // v2.35: IT'S ME burst clock during his visit

// v2.46: Customize screen session state — the original keeps the four AI
// counters (objInfo 141-144) as Fusion GLOBALS, so they persist across the
// frame change and a death retry within one app run (cleared on restart).
static i32  g_custLevels[4] = { 0, 0, 0, 0 };
static i32  g_custSel = 0;
static bool g_creepyToMenu = false;   // the 1987 easter egg returns to the title

// v2.35: DEV — spawn the REAL Golden Freddy pipeline in the office
// (bypasses the 1/100 poster roll): giggle + office appearance (img 573) +
// IT'S ME flashes + full-screen face + the intentional close.
static void DebugSpawnGoldenFreddy() {
    if (s_goldInOffice) return;
    s_goldState = 2;
    s_goldInOffice = true;
    s_goldTimer = 0.0f;
    s_goldItsmeT = 0.0f;
    g_audio.Play(&g_pak, Snd::FREDDY_LAUGH_LONG, false, 1.0f);
    g_debugConsole.Print("GOLDEN SPAWNED (debug): appears in the office");
}

// v2.53 (dump group 425): the Golden Freddy ARM is a silent global roll —
// 1/100000 once per SECOND, unconditional from the very first night, with no
// monitor/camera gating at all. The old build armed him 1/100 per monitor
// drop (fused with the 2B poster art roll) — thousands of times too common.
static f32 s_goldArmT = 0.0f;

static void TickGoldenFreddy(Game& game, GameRender& render) {
    const bool monUp = game.GetCameras().IsMonitorUp();
    const CameraId cam = game.GetCameras().GetCurrentCamera();

    // 1. the rare arm (dump g425) — global seconds roll
    if (s_goldState == 0) {
        s_goldArmT += 1.0f / 60.0f;
        if (s_goldArmT >= 1.0f) {
            s_goldArmT -= 1.0f;
            if (rand() % 100000 == 0) s_goldState = 1;
        }
    }
    // The armed state also forces the 2B poster to show Golden (dump g43).
    render.SetGoldenPosterArmed(s_goldState == 1);

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

    // 4. hold: raising the monitor despawns him; ~5 s -> the kill.
    // v2.35: his visit is accompanied by IT'S ME flashes (~0.2 s every
    // ~1.1 s) — the wiki's hallucination phase; then the full-screen face
    // (f14) and the intentional close.
    if (s_goldInOffice) {
        s_goldTimer += 1.0f / 60.0f;
        if (monUp) {
            s_goldInOffice = false; s_goldState = 0; s_goldTimer = 0.0f;
            s_goldItsmeT = 0.0f;
            g_itsmeT = -1.0f;
        } else {
            s_goldItsmeT += 1.0f / 60.0f;
            if (s_goldItsmeT >= 1.1f) s_goldItsmeT -= 1.1f;
            g_itsmeT = (s_goldItsmeT < 0.2f) ? 1.0f : -1.0f;
            if (s_goldTimer >= 5.0f) {
                s_goldInOffice = false; s_goldState = 0; s_goldTimer = 0.0f;
                s_goldItsmeT = 0.0f;
                g_itsmeT = -1.0f;
                game.DebugTriggerGoldenFreddy();
            }
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

// v2.54: X-button save wipe on the title — hold 5 s, then a system
// confirmation box (the same SysPrompt pipeline as the import box); "Yes"
// wipes progress. v2.54 removed the older cheats this replaces (USB-keyboard
// Delete, LT+RT pad hold) — the X version is the one wipe path.
static f32 g_titleXWipeHoldT = 0.0f;
static bool g_titleXWipePending = false;

static void PollTitleXWipe(MenuSystem& menu, const GameInput& gi, f32 dt) {
    if (g_titleXWipePending) return;   // the box is up; wait for its answer
    if (!gi.xHeld) {
        g_titleXWipeHoldT = 0.0f;
        return;
    }
    g_titleXWipeHoldT += dt;
    if (g_titleXWipeHoldT < 5.0f) return;

    g_titleXWipeHoldT = 0.0f;
    g_titleXWipePending = true;
    static const wchar_t* btns[] = { L"No", L"Yes" };
    DWORD pressed = 0;
    // SysPrompt returns "the box completed", not the choice — read the index
    // ("Yes" is index 1; focus sits on "No"=0 so a stray A can't wipe).
    SysPrompt(L"Save wipe",
        L"Delete the whole save? All nights and stars will be reset.",
        btns, 2, 0, XMB_WARNINGICON, &pressed);
    if (pressed == 1) {
        Progress::Reset(g_prog);
        if (Progress::Save(g_prog)) {
            RefreshMenuFromProgress(menu);
            g_debugConsole.Print("SAVE WIPED (X hold)");
        } else {
            g_debugConsole.Print("SAVE WIPE FAILED");
        }
    }
    g_titleXWipePending = false;
}

void OnTimeUpdate(i32 hour){
    if(hour!=s_lastHour){
        s_lastHour=hour;
        // v2.17: no hourly laugh in the original — Freddy's giggle is tied to
        // his "got in" entry, not the clock (the old FREDDY_LAUGH[hour%3] was wrong).
    }
}
void OnPowerUpdate(f32 power){ s_lastPower=power; }

// v2.48/2.49: XSCREAM is NOT always instant in the original — the office
// kills reach the scream a beat after the scare anim starts: Freddy at frame
// 7 of the 31-frame lunge (group 409: anim 65 frame 7 → 0.233 s), Bonnie/Chica
// when their alt2 counter runs 10→1 (groups 228/229/231 — 9 ticks, 0.150 s).
// Foxy's kill and both special ones (Golden, power-out dark face) scream
// immediately. The dump has NO fade anywhere — only instant channel-volume
// sets plus hard "stop all" on frame transitions.
static bool s_screamPending = false;
static f32  s_screamDelay   = 0.0f;

void OnJumpscare(AnimatronicId anim){
    const char* n[]={"Freddy","Bonnie","Chica","Foxy"};
    const char* nm = (anim == ANIM_FREDDY_DARK) ? "Freddy (power-out)"
                   : (anim >= ANIM_FREDDY && anim < ANIM_COUNT) ? n[anim] : "Golden Freddy";
    printf("*** JUMP SCARE by %s! ***\n", nm);
    // group 228/322/408: XSCREAM (voiceover/garble stop too)
    g_audio.Stop(Snd::VOICEOVER[0]); g_audio.Stop(Snd::VOICEOVER[1]);
    g_audio.Stop(Snd::VOICEOVER[2]); g_audio.Stop(Snd::VOICEOVER[3]);
    g_audio.Stop(Snd::VOICEOVER[4]);
    g_audio.Stop(Snd::AMBIENCE2); g_audio.Stop(Snd::CIRCUS);
    s_phonePlaying=false;
    // v2.27: Golden Freddy (ANIM_COUNT) gets XSCREAM2 — a DELIBERATE
    // DEVIATION from the dump (see CHANGELOG v2.27): the dump itself only
    // plays XSCREAM2 on f15 "creepy end" (unreachable demo ending) and its
    // f14 "creepy start" is silent. The robots share XSCREAM (ch1).
    s_screamPending = false;
    if (anim == ANIM_FREDDY) {                      // group 409: anim-65 frame 7 (0.233 s)
        s_screamPending = true; s_screamDelay = 7.0f / 30.0f;
    } else if (anim == ANIM_BONNIE || anim == ANIM_CHICA) {   // groups 228/229/231: alt2 10->1 = 9 ticks (0.150 s)
        s_screamPending = true; s_screamDelay = 9.0f / 60.0f;
    } else {                                        // Foxy / Golden / dark power-out: at once
        g_audio.Play(&g_pak, (anim == ANIM_COUNT) ? Snd::XSCREAM2 : Snd::XSCREAM, false, 1.0f);
    }
    g_audio.Stop(Snd::WHISPERING);   // v2.46: the office whisper dies with the kill
    // v2.37: the pad shakes at FULL force for the whole scare (both motors)
    RumbleKick(65535, 65535, 1.4f);
    g_debugConsole.Print("SCARE %s begin", nm);
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
    g_audio.Stop(Snd::WHISPERING);   // v2.46: Freddy's office whisper (ch25)
    s_phonePlaying=false;
    g_audio.Play(&g_pak, Snd::POWERDOWN, false, 1.0f);
    // v2.37: dry fading crackle on the high-freq motor at the power cut
    RumbleKick(0, 26000, 0.6f);
    // group 271/286: dark ambient drone (ambience2) alongside the music box
    g_audio.Play(&g_pak, Snd::AMBIENCE2, true, 0.5f);
    s_faceStatePrev = 0;   // v2.22: reset the "Active 2" face-sound cycle
}
void OnMusicBoxStart(){
    printf("(Music box starts...)\n");
    // v2.48 (groups 272/273): the real "music box" jingle, one-shot. The old
    // code looped CIRCUS here by mistake.
    g_audio.Play(&g_pak, Snd::MUSIC_BOX, false, 1.0f);
}
void OnMusicBoxStop(){
    printf("(Music box stops...)\n");
    g_audio.Stop(Snd::MUSIC_BOX);
}
// v2.49 (wiki detail, user-picked): Freddy's footsteps approach in the dark
// between the jingle and the kill — the "prepare the player" cue.
void OnPowerOutSteps(){
    printf("(Freddy's steps...)\n");
    g_audio.Play(&g_pak, Snd::DEEP_STEPS, false, 0.9f);
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
        // v2.53 (groups 130 + 144 + 16): the flip-up plays whir (ch7), the
        // camcorder tape-eject (ch6) when the feed commits, and the blip3 of
        // the content-commit frame. The port's invented monitor static bed
        // is removed — no such loop exists in the original soundscript.
        g_audio.Play(&g_pak, Snd::CAMERA_SWITCH, false, 0.8f);
        g_audio.Play(&g_pak, Snd::TAPE_EJECT, false, 0.8f);
        g_audio.Play(&g_pak, Snd::BLIP, false, 0.8f);
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
    // v2.37: dull heavy left-motor push when the door lands (close only)
    if (c) RumbleKick(18000, 0, 0.15f);
    // v2.53: door motor (SFXBible_12478, ch4) plays on BOTH close (g96/100)
    // and open (g102-105) — the port used to slam close-only.
    g_audio.Play(&g_pak, Snd::DOOR_SLAM, false, 1.0f);
}
void OnLightChange(DoorSide s,bool o){
    printf("[Light %s: %s]\n",s==DOOR_LEFT?"Left":"Right",o?"ON":"OFF");
}
void OnAnimatronicMove(AnimatronicId a,RoomId r){
    const char* n[]={"Freddy","Bonnie","Chica","Foxy"};
    RoomInfo i=RoomSystem::GetRoomInfo(r);
    printf("[AI] %s -> %s\n",n[a],i.name);
    // v2.37: Freddy's hulk creeping closer — a barely-there low-freq pulse
    // on his path steps (the touch of the approach)
    if (a == ANIM_FREDDY) RumbleKick(13000, 0, 0.2f);
    // v2.17: deep steps for Bonnie/Chica; Freddy's laugh is the _1d/_2d/_8d
    // giggle family (#56/57/58), NOT Laugh_Giggle_Girl_1 (#38 = Golden Freddy).
    if(a==ANIM_BONNIE||a==ANIM_CHICA) {
        // v2.53 (dump groups 199-244): the full loudness ladder of the step
        // sound — 10 for the far hops, 20 for dining/restrooms/kitchen, 30
        // the mid halls & closet, 40 the corners and the door zones; and the
        // steps are MUTED while you watch the room (groups 212/213).
        const bool watched = g_gameRef && g_gameRef->IsWatchingRoom(r);
        if (!watched) {
            float v = CFVolumeToDb(10);
            if (r==ROOM_DINING_AREA || r==ROOM_RESTROOMS || r==ROOM_KITCHEN) v = CFVolumeToDb(20);
            else if (r==ROOM_WEST_HALL || r==ROOM_SUPPLY_CLOSET || r==ROOM_EAST_HALL) v = CFVolumeToDb(30);
            else if (r==ROOM_WEST_HALL_CORNER || r==ROOM_EAST_HALL_CORNER ||
                     r==ROOM_LEFT_DOOR || r==ROOM_RIGHT_DOOR) v = CFVolumeToDb(40);
            g_audio.SetChannelVolume(CH_DEEPSTEPS, v);
            g_audio.PlayOnChannel(&g_pak, Snd::DEEP_STEPS, false, CH_DEEPSTEPS);
        }
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
        RumbleKick(13000, 0, 0.2f);   // v2.37: Freddy's laugh — faint LF pulse
        g_audio.PlayOnChannel(&g_pak, Snd::RUNNING_FAST, false, CH_RUNFAST);
        // music box while Freddy is in the kitchen (groups 399/400, ch22)
        if (r==ROOM_KITCHEN) g_audio.PlayOnChannel(&g_pak, Snd::MUSIC_BOX, false, CH_MUSICBOX);
        // v2.46 (group 405): "whispering2" while Freddy stands inside the
        // office ("freddy got in") — his dark-office kill ambience.
        if (r==ROOM_RIGHT_DOOR) g_audio.PlayOnChannel(&g_pak, Snd::WHISPERING, true, CH_WHISPER);
    }
    // v2.14: "No Laughing" — Freddy steps into the East Hall on Night 5
    if(a==ANIM_FREDDY && r==ROOM_EAST_HALL) g_ach.OnFreddyEast();
}
void OnFoxyStageChange(FoxyStage s){
    const char* t[]={"Curtain Closed","Peeking","Gone","Lurking","RUNNING!","AT DOOR!"};
    printf("[AI] Foxy: %s\n",t[s]);
    // v2.53 (group 40): the sprint sound is a ONE-SHOT "run" at the 3->4
    // trigger while you watch CAM 2A — the port used to loop it from stage 3.
    if(s==FOXY_STAGE_4) g_audio.Play(&g_pak, Snd::RUN, false, 1.0f);
    // v2.14: "No Running" — Foxy leaves the cove (stage 4 sprint)
    if(s==FOXY_STAGE_4) g_ach.OnFoxyRan();
}
void OnFoxyDoorBang(f32 p){
    printf("[AI] Foxy bangs! -%.1f%%\n",p);
    // v2.37: strong single thump on the LEFT motor at the bang (the power
    // penalty moment), + diagnostics: bang # (power drain 10+50*(n-1)
    // tenths -> n = (p-1)/5) and the right-door state — tells the "scare
    // plays every ~3 bangs" report apart from "door was closed".
    RumbleKick(30000, 0, 0.15f);
    int bangIdx = (int)((p - 1.0f) / 5.0f) + 1;
    if (bangIdx < 1) bangIdx = 1;
    const bool doorClosed = g_gameRef ? g_gameRef->GetDoors().IsDoorClosed(DOOR_RIGHT) : false;
    g_debugConsole.Print("FOXY bang #%d door=%s", bangIdx, doorClosed ? "closed" : "open");
    // v2.53 (group 324): the bang is just knock2 ch9 — the pounding sample
    // returned to its ambience home (the periodic door-pounding, group 271,
    // got restored in the random-event pass).
    g_audio.Stop(Snd::RUN); g_audio.Stop(Snd::RUNNING_FAST);
    g_audio.Play(&g_pak, Snd::KNOCK, false, 0.9f);
}

// Start the Phone Guy call for the current night (frame 3 groups 361-365)
static f32 s_phoneT = 0.0f;   // v2.53: call elapsed (for the mute-button window)
static void StartPhoneCall(i32 night) {
    if (night < 1 || night > 5) return;           // nights 6/7: no call
    if (s_phoneMuted) return;
    g_audio.SetChannelVolume(CH_PHONE, CFVolumeToDb(100));   // office, monitor down (100 = unity)
    g_audio.PlayOnChannel(&g_pak, Snd::VOICEOVER[night-1], false, CH_PHONE);
    s_phonePlaying = true;
    s_phoneT = 0.0f;
}

// Phone Guy call (frame 3 groups 361-365): the original plays AT OFFICE START
// (night number == N, "play voice N" == 0) — no 2.5 s delay. Nights 6/7 silent.
// The mute button exists only for the call's +20..+40 s window (groups 380/378).
static bool s_phoneStarted = false;
static void TickPhoneCall(Game& game, f32 dt) {
    if (s_phonePlaying) s_phoneT += dt;
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
// (g_devMode moved above FrameEnd — its console gate reads it)
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
// v2.32: the boot selector state (filled by the pak scan in main())
static bool g_pakFound[8] = { false };

// The boot selector: runs on the bare core (text only, pak not loaded).
// D-pad/stick select, A launch, B = default (FNAF1). Returns the chosen
// module index; AppRegistry_Active() is left pointing at it.
static i32 RunBootSelector() {
    i32 cursor = 0;
    while (!g_pakFound[cursor]) ++cursor;   // start on the first presentable
    bool selDone = false;
    while (!selDone) {
        GameInput gi; UpdateInput(gi);
        if (gi.cameraUp)   { do { cursor = (cursor + AppRegistry_Count() - 1) % AppRegistry_Count(); } while (!g_pakFound[cursor]); }
        if (gi.cameraDown) { do { cursor = (cursor + 1) % AppRegistry_Count(); } while (!g_pakFound[cursor]); }
        if (gi.cameraToggle) selDone = true;
        if (gi.back) { cursor = 0; selDone = true; }   // B = default (FNAF1)

        FrameBegin(D3DCOLOR_XRGB(0,0,0));
        {
            g_text.DrawText(490, 120, "SELECT GAME", 0xFFFFFFFF);
            for (i32 mi = 0; mi < AppRegistry_Count() && mi < 8; ++mi) {
                if (!g_pakFound[mi]) continue;   // only launchable games
                char row[80];
                Snprintf(row, sizeof(row), "%s %s  (%s)",
                         mi == cursor ? ">" : " ",
                         AppRegistry_Get(mi)->Name(),
                         AppRegistry_Get(mi)->PakName());
                g_text.DrawText(500, 220 + mi * 60, row,
                                mi == cursor ? 0xFF80FF80 : 0xFFB0B0B0);
            }
            g_text.DrawText(430, 620, "D-pad select   A launch   B default (FNAF1)", 0xFF909090);
        }
        FrameEnd();
    }
    AppRegistry_SetActive(cursor);
    g_debugConsole.Print("Selected: %s", AppRegistry_Active()->Name());
    return cursor;
}


int main(int argc, char* argv[]){
    // v2.7.4: FIRST line of the log -- proves which sources are actually in
    // the running XEX (settles "for VS it's as if the files didn't change":
    // check this line or run APPLY_PATCH.bat from the minipatch)
    printf("=== FNAF1-Recomp v2.62 built %s %s ===\n", __DATE__, __TIME__);

    Game game;
    g_gameRef = &game;
    GameCallbacks cb; cb.onTimeUpdate=OnTimeUpdate; cb.onPowerUpdate=OnPowerUpdate;
    cb.onJumpscare=OnJumpscare; cb.onPowerOut=OnPowerOut; cb.onMusicBoxStart=OnMusicBoxStart;
    cb.onMusicBoxStop=OnMusicBoxStop; cb.onPowerOutSteps=OnPowerOutSteps; cb.onNightComplete=OnNightComplete;
    cb.onGameOver=OnGameOver; cb.onCameraChange=OnCameraChange; cb.onDoorChange=OnDoorChange;
    cb.onLightChange=OnLightChange; cb.onAnimatronicMove=OnAnimatronicMove;
    cb.onFoxyStageChange=OnFoxyStageChange; cb.onFoxyDoorBang=OnFoxyDoorBang;
    game.SetCallbacks(cb);
    i32 cmdNight=0; if(argc>1){ cmdNight=atoi(argv[1]); if(cmdNight<1)cmdNight=1; if(cmdNight>7)cmdNight=7; }
    if(!InitD3D()){ printf("FATAL: InitD3D failed\n"); return 1; }
    // v2.7.4: same version banner on the on-screen debug console (bottom of
    // the screen) -- visible without a debugger attached
    g_debugConsole.Print("FNAF1-Recomp v2.62 (%s %s)", __DATE__, __TIME__);
    // v2.28: the app shell addresses the game through the AppModule contract
    // v2.29: SOFT pak scan — probe every module's bundle at the canonical
    // location, report each, and park the active module on one that exists
    // (FNAF1 has priority; boot never fails here).
    AppRegistry_ScanPaks(g_pakFound, 8);
    for (i32 mi = 0; mi < AppRegistry_Count() && mi < 8; ++mi) {
        printf("Pak scan: %s -> %s\n", AppRegistry_Get(mi)->PakName(),
               g_pakFound[mi] ? "OK" : "not found");
        g_debugConsole.Print("Pak %s: %s", AppRegistry_Get(mi)->PakName(),
                             g_pakFound[mi] ? "OK" : "not found");
    }

    // v2.32: BOOT SELECTOR — more than one bundle present? show the list.
    {
        i32 foundCount = 0;
        for (i32 mi = 0; mi < AppRegistry_Count() && mi < 8; ++mi)
            if (g_pakFound[mi]) ++foundCount;
        if (foundCount > 1) RunBootSelector();
    }

    g_debugConsole.Print("Module: %s (%s)", AppRegistry_Active()->Name(),
                         AppRegistry_Active()->PakName());

    // v2.30: dev override for the per-game XEX work — a command-line arg
    // naming a module ("fnaf2"/"fnaf3"/"fnaf4"/"sl") activates it after
    // the scan (Xenia/debugger args; the console launcher will pick the
    // XEX itself once the per-game builds exist).
    for (int ai = 1; ai < argc; ++ai) {
        for (i32 m = 0; m < AppRegistry_Count(); ++m) {
            AppModule* mm = AppRegistry_Get(m);
            if (!mm) continue;
            // case-insensitive compare of the lowercase arg vs module name
            const char* a = argv[ai]; const char* b = mm->Name();
            bool match = true;
            for (int k = 0; a[k] || b[k]; ++k) {
                char ca = a[k], cb = b[k];
                if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                if (ca != cb) { match = false; break; }
                if (!ca) break;
            }
            if (match && a[0]) {
                AppRegistry_SetActive(m);
                g_debugConsole.Print("Arg override -> %s", mm->Name());
            }
        }
    }

    // Try load the ACTIVE module's pak from Xbox 360 canonical locations
    // (game:\ is XEX directory; e:\/hdd:\ are common on JTAG/RGH dashboards
    // like FSD or Aurora — re-enable those variants when needed)
    char pakPath[128];
    Snprintf(pakPath, sizeof(pakPath), "game:\\%s", AppRegistry_Active()->PakName());
    // v2.32: the module picks its loader — eager fits FNAF1-3 in the
    // 512 MB UMA pool, FNAF4/SL must stream (real-HW OOM otherwise).
    const bool pakOk = AppRegistry_Active()->PrefersStreaming()
        ? g_pak.LoadStreaming(pakPath, g_pd3dDevice)
        : g_pak.Load(pakPath, g_pd3dDevice);
    if(pakOk){ g_pakLoaded=true; }
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
    // v2.34: present one frame first — at boot the XAM message box can
    // "sound without showing" if nothing has been presented yet
    FrameBegin(D3DCOLOR_XRGB(0,0,0));
    FrameEnd();
    // v2.50: pick the storage device up front — before ANY message box. On a
    // single-device console (Slim + HDD, no MU slots) the selector answers
    // silently with the HDD and never shows UI; later saves/mounts (incl. the
    // import below) then never invoke XAM's UI pipeline mid-flow — that was
    // the ACCESS_DENIED storm source.
    Progress::PrimeStorage();
    // v2.23: if a loose "freddy" save is found next to the game, ask before
    // importing it into the save storage.
    // v2.43: NO second box and NO dashboard exit — the import runs BEFORE
    // the disclaimer, and RefreshMenuFromProgress right below reads the
    // fresh progress in the SAME boot. Real HW: XAM keeps refusing new UI
    // for a long while after a box closes, so any follow-up system screen
    // is denied anyway.
    if (Progress::HasLooseSave()) {
        printf("IMPORT: loose freddy found, asking user\n");
        g_debugConsole.Print("IMPORT: loose freddy found");
        if (ShowImportSavePrompt()) {
            printf("IMPORT: user chose YES, importing...\n");
            g_debugConsole.Print("IMPORT: YES -> importing");
            if (Progress::ImportSave()) {
                printf("IMPORT: done, progress re-read this boot (no restart)\n");
                g_debugConsole.Print("IMPORT: done -> same-boot read");
            } else {
                printf("IMPORT: FAILED (no source file or XContent down)\n");
                g_debugConsole.Print("IMPORT: FAILED");
            }
        } else {
            printf("IMPORT: user chose NO (or box timed out)\n");
        }
    }
    RefreshMenuFromProgress(menu);   // v2.7.13: boot from fnaf_save.bin
    g_ach.Init();                     // v2.14: load achievements (device already chosen)
    g_ach.SetDebugConsole(&g_debugConsole);   // v2.35: ACH results on the debug console

    // Boot: disclaimer first (original title-frame String obj 0 flow)
    GameState state = GAME_STATE_DISCLAIMER;
    // v2.15: boot fade-in for the disclaimer (1010 ms, FRAME_TRANSITIONS.md)
    g_fade.phase = 2; g_fade.alpha = 1.0f; g_fade.timer = 0.0f; g_fade.dur = 1.010f;
    if(cmdNight!=0){ game.Init(cmdNight); g_ach.BeginNight(cmdNight); state=GAME_STATE_NIGHT_START; g_fade.phase = 0; g_fade.alpha = 0.0f; }

    i32 tickCount=0;
    static int menuFrameCounter = 0;
    static int adCounter = 0;         // v2.15: separate timer for the newspaper screen
    float accumulator=0.0f;
    // v2.46: was 1/30 ("logic 30Hz") while the accumulator gains 1/60 per
    // frame and EVERY system inside (clock 5400 ticks/hour = 90 s, power
    // drain per 60 ticks, AI opportunity/delay/foxy timers) is denominated
    // in 60 Hz ticks -- the whole game ran at HALF the original's speed
    // (hours took 180 s). Original: application frameRate = 60.
    const float tickDelta = 1.0f/60.0f; // logic 60Hz, matches TICK_RATE
    s_phoneStarted = false;
    f32 scareElapsed = 0.0f;
    i32 endFrames = 0;

    static const CameraId cameraSequence[]={CAM_1A,CAM_1B,CAM_1C,CAM_2A,CAM_2B,CAM_3,CAM_4A,CAM_4B,CAM_5,CAM_6,CAM_7};
    static const i32 cameraSequenceLen=11;

    while(true){
        GameInput gi; UpdateInput(gi);
        bool devToggled = false;   // v2.17: Start+B fired this frame (swallow it)

        // ---------------- MODULE MODE (v2.30) ----------------
        // A non-FNAF1 active module owns the frame: Tick + Render per loop,
        // no FNAF1 state machine, no fades. B exits the app (the per-game
        // XEX builds will own their own flow; this shared-core branch is
        // the testbed for their screens until then).
        if (strcmp(AppRegistry_Active()->Name(), "FNAF1") != 0) {
            AppModule* m = AppRegistry_Active();
            g_audio.Tick();
            // fill the shared services once and hand them over; the pad
            // snapshot is refreshed every frame before Tick
            static AppServices s_svc;
            static GameInput   s_in;
            static AppModule*  s_loadedModule = 0;
            if (s_loadedModule != m) {
                if (!s_svc.pak) {
                    s_svc.audio = &g_audio;
                    s_svc.pak   = &g_pak;
                    s_svc.batch = &g_batch;
                    s_svc.text  = &g_text;
                    s_svc.ach   = &g_ach;   // v2.62: the FNAF2 add-on ids
                }
                m->Load(s_svc);
                s_loadedModule = m;
                g_debugConsole.Print("Module %s: services handed over", m->Name());
            }
            s_in = gi;
            s_svc.input = &s_in;
            m->Tick(1.0f / 60.0f);
            if (gi.back) {
                // v2.32: B = back to the boot selector (NOT a console kill):
                // drop the current pak, pick another game, load its bundle.
                g_pak.Unload();
                g_pakLoaded = false;
                s_loadedModule = 0;
                RunBootSelector();
                char pakPath2[128];
                Snprintf(pakPath2, sizeof(pakPath2), "game:\\%s",
                         AppRegistry_Active()->PakName());
                const bool pakOk2 = AppRegistry_Active()->PrefersStreaming()
                    ? g_pak.LoadStreaming(pakPath2, g_pd3dDevice)
                    : g_pak.Load(pakPath2, g_pd3dDevice);
                g_pakLoaded = pakOk2;
                if (strcmp(AppRegistry_Active()->Name(), "FNAF1") == 0) {
                    state = GAME_STATE_DISCLAIMER;   // fresh FNAF1 boot flow
                } else {
                    AppRegistry_Active()->Load(s_svc);
                    s_loadedModule = AppRegistry_Active();
                }
            }
            if (strcmp(AppRegistry_Active()->Name(), "FNAF1") != 0) {
                FrameBegin(D3DCOLOR_XRGB(0,0,0));
                m->Render();
                FrameEnd();
                continue;
            }
            // FNAF1 was chosen in the selector: fall through to its flow
        }

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
        // Start+B DEV combo. The pad almost never reports BOTH edges on the
        // SAME 16 ms frame — B normally lands first, Start a frame or two
        // later — and a lone Start edge falls into the pause handler below,
        // kicking the game to the title. So: remember a B edge for ~330 ms
        // and accept a Start edge inside that window (same-frame still works
        // because the B edge is recorded before the check).
        static int  s_devBWindow = 0;           // frames left of the B window
        if (gi.back) s_devBWindow = 20;
        if (s_devBWindow > 0) --s_devBWindow;
        if (gi.pause && (gi.back || s_devBWindow > 0)) {
            g_devMode  = !g_devMode;
            devToggled = true;
            s_devBWindow = 0;
            if (g_devMode) g_debugConsole.Print("DEV menu ON");
        }

        g_render.SetLookDir(gi.lookDir);   // office pan window (v2.5)
        g_render.Tick(1.0f/60.0f);
        game.TickDoors(1.0f/60.0f);   // v2.22: door slide (visual, 60 Hz)
        g_audio.Tick();
        g_ach.Tick(1.0f/60.0f);     // v2.14: achievement toast timer
        TickRumble(1.0f/60.0f);     // v2.37: pad rumble envelope (decay + XInputSetState)
        TickFade(state, 1.0f/60.0f);  // v2.15: advance any running fade (may change `state`)
        if (g_goldenScareT >= 0.0f) {          // v2.17: scare flash timer
            g_goldenScareT += 1.0f/60.0f;
            if (g_goldenScareT > 1.3f) g_goldenScareT = -1.0f;
        }
        if (g_itsmeT >= 0.0f) {          // v2.27: hallucination window = 100 ticks (group 415)
            g_itsmeT += 1.0f/60.0f;
            if (g_itsmeT > 100.0f/60.0f) g_itsmeT = -1.0f;
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
            // v2.53 (dump groups 333/334 + 386/387): the doorway-pose reveal
            // stinger — "windowscare" plays once per arrival when the pose
            // first shows under the light; the latch resets when they leave.
            {
                static bool s_poseLATL = false;
                static bool s_poseRATR = false;
                const AnimatronicAI& lai = game.GetAI();
                const DoorSystem&  ds  = game.GetDoors();
                const bool office = !game.GetCameras().IsMonitorUp();
                const bool poseL = office && ds.IsLightOn(DOOR_LEFT)  && lai.IsAnimatronicAtDoor(ANIM_BONNIE, DOOR_LEFT);
                const bool poseR = office && ds.IsLightOn(DOOR_RIGHT) && lai.IsAnimatronicAtDoor(ANIM_CHICA, DOOR_RIGHT);
                if (poseL && !s_poseLATL) { g_audio.Play(&g_pak, Snd::WINDOW_SCARE, false, 0.9f); s_poseLATL = true; }
                if (poseR && !s_poseRATR) { g_audio.Play(&g_pak, Snd::WINDOW_SCARE, false, 0.9f); s_poseRATR = true; }
                if (!lai.IsAnimatronicAtDoor(ANIM_BONNIE, DOOR_LEFT))  s_poseLATL = false;
                if (!lai.IsAnimatronicAtDoor(ANIM_CHICA, DOOR_RIGHT)) s_poseRATR = false;
            }
            // v2.46 (groups 219-222): a garble/digital sample on the rising
            // edge of the camera static-out window (Random(4)+1 -> 1..4).
            {
                static i32 prevFeedStatic = 0;
                const i32 fs = game.GetFeedStaticTicks();
                if (fs > 0 && prevFeedStatic == 0) {
                    const int g = rand() % 4;
                    if (g == 0) g_audio.Play(&g_pak, Snd::COMPUTER_DIG, false, 0.8f);
                    else        g_audio.Play(&g_pak, Snd::GARBLE[g - 1], false, 0.8f);
                }
                prevFeedStatic = fs;
            }
            // v2.53: the "IT'S ME" arm roll is once per SECOND (dump group
            // 419: timer 1000, 1/1000) — the port used to roll once per 20 s
            // (the "(~20.00s)" annotation misread). The 100-tick show window
            // and the decade-view flicker below match (g415/416).
            g_itsmeRollTimer += 1.0f/60.0f;
            if (g_itsmeRollTimer >= 1.0f) {
                g_itsmeRollTimer = 0.0f;
                if ((rand() % 1000) == 0) g_itsmeT = 0.0f;
            }
            if (g_itsmeT >= 0.0f) {
                // group 416: ch21 (robotvoice) volume 100 while it flashes;
                // TickAudioMixer resumes its proximity heuristic afterwards
                g_audio.SetChannelVolume(CH_ROBOTVOICE, CFVolumeToDb(100));
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
                            // Golden Freddy: the REAL pipeline — appears in
                            // the office (not a camera frame), IT'S ME
                            // flashes, ~5 s later the face fills the screen
                            // and the title closes (safe, intentional).
                            DebugSpawnGoldenFreddy();
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
            // the save wipe lives ONLY on the X hold (v2.54)
            PollTitleXWipe(menu, gi, 1.0f / 60.0f);

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
                    // v2.46: Night 7 opens the CUSTOMIZE screen (frame 12):
                    // the original copies the four global AI counters into
                    // the activity counters only at night start (group 311);
                    // the sliders are the 1987-easter-egg input too.
                    if(night == 7){
                        g_audio.Stop(Snd::STATIC2); g_audio.Stop(Snd::DARKNESS_MUSIC);
                        state = GAME_STATE_CUSTOMIZE;
                        tickCount=0; accumulator=0; menuFrameCounter=0;
                        Sleep(16); continue;
                    }
                    g_audio.Stop(Snd::STATIC2); g_audio.Stop(Snd::DARKNESS_MUSIC);
                    // v2.33: 1:1 with dump group 24 — clicking New Game writes
                    // `level=1` to the ini (the beat flags are KEPT, exactly
                    // like the original's set-"level" action)
                    if (menu.LastStartWasNewGame()) {
                        Progress::Load(g_prog);
                        g_prog.nextNight = 1;
                        // v2.53 (audit): the original wipes beatgame/beat6/beat7
                        // ONLY on the hold-Delete cheat (title group 49) — New
                        // Game touches just level=1. Stars survive a New Game.
                        Progress::Save(g_prog);
                    }
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

        // ---------------- CUSTOMIZE (Night 7 setup, frame 12) ----------------
        // v2.46: the original is mouse-driven (11 button groups); the console
        // maps it to up/down = row, left/right = -1/+1 (groups 5-12 clamp
        // 0..20), A = START (group 2), B = back. The 1987 combo (groups
        // 13-17: 1/9/8/7) jumps to the creepy screen instead of the night.
        if(state==GAME_STATE_CUSTOMIZE){
            if(gi.cameraUp)         { g_custSel = (g_custSel + 3) % 4; g_audio.Play(&g_pak,Snd::BLIP,false,0.8f); }
            else if(gi.cameraDown)  { g_custSel = (g_custSel + 1) % 4; g_audio.Play(&g_pak,Snd::BLIP,false,0.8f); }
            else if(gi.cameraLeft || gi.lookDir < -0.5f){
                if(g_custLevels[g_custSel] > 0){ g_custLevels[g_custSel]--; g_audio.Play(&g_pak,Snd::BLIP,false,0.8f); }
            }
            else if(gi.cameraRight || gi.lookDir > 0.5f){
                if(g_custLevels[g_custSel] < 20){ g_custLevels[g_custSel]++; g_audio.Play(&g_pak,Snd::BLIP,false,0.8f); }
            }

            if(gi.cameraToggle){
                const bool e1987 = (g_custLevels[0]==1 && g_custLevels[1]==9 &&
                                    g_custLevels[2]==8 && g_custLevels[3]==7);
                g_audio.Stop(Snd::STATIC2); g_audio.Stop(Snd::DARKNESS_MUSIC);
                if(e1987){
                    // group 3: START with 1987 -> frame 14 "creepy start"
                    g_creepyToMenu = true;
                    game.DebugTriggerGoldenFreddy();
                    state = GAME_STATE_JUMPSCARE; scareElapsed = 0.0f;
                } else {
                    game.InitCustomNight(g_custLevels);
                    g_ach.BeginNight(7);
                    s_phoneMuted=false; s_phonePlaying=false; s_phoneStarted=false;
                    s_goldState=0; s_goldInOffice=false; s_goldTimer=0.0f; g_render.SetGoldenFreddyInOffice(false);
                    tickCount=0; accumulator=0; menuFrameCounter=0; adCounter=0;
                    StartTransition(state, GAME_STATE_NIGHT_START);
                }
            } else if(gi.back){
                StartTransition(state, GAME_STATE_MENU); menu.Reset();
            }

            if(FrameBegin(D3DCOLOR_XRGB(0,0,0))){
                g_render.RenderCustomize(g_custLevels, g_custSel);
                DrawFadeOverlay();
                FrameEnd();
            }
            Sleep(16); tickCount++; continue;
        }

        // ---------------- INTRO AD ("HELP WANTED", v2.7.13) ----------------
        // New Game only: the newspaper (frame "ad", img_574) holds ~8 s,
        // any button skips, then the night-1 card. Uses its own counter:
        // menuFrameCounter is reset every frame by the menu block above.
        if(state==GAME_STATE_INTRO_AD){
            ++adCounter;
            const bool adLock = adCounter < 30;   // skip boot bounce
            if(FrameBegin(D3DCOLOR_XRGB(0,0,0))){
                g_render.RenderIntroAd();
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
            // v2.53 (dump g95-109): door/light buttons listen only while the
            // monitor is DOWN; every press pays the 10-tick click cooldown;
            // buttons "jam" (error click, no action) only while an intruder
            // stands at that door — Bonnie on the left, Chica on the right
            // (Freddy/Foxy never jam). LIGHTS stay pressable like the dump
            // (the light-up IS how you verify the doorway pose); only closing
            // a door onto an occupant errors (a reopen plays the motor as usual).
            if (s_clickCooldown > 0) s_clickCooldown--;
            const bool inputOk = !game.GetCameras().IsMonitorUp() && s_clickCooldown == 0;
            const bool jamL = game.GetAI().IsAnimatronicAtDoor(ANIM_BONNIE, DOOR_LEFT);
            const bool jamR = game.GetAI().IsAnimatronicAtDoor(ANIM_CHICA, DOOR_RIGHT);
            if (g_devAnalogDoor) {
                if (!jamL) game.SetDoorAmount(DOOR_LEFT,  gi.leftDoorAxis);
                if (!jamR) game.SetDoorAmount(DOOR_RIGHT, gi.rightDoorAxis);
            } else {
                if (gi.leftDoorToggle && inputOk) {
                    // the error answers only a CLOSE onto an occupied doorway
                    // (dump g97/101); reopening plays the motor like always
                    if (jamL && !game.GetDoors().IsDoorClosed(DOOR_LEFT))
                        g_audio.Play(&g_pak, Snd::DOOR_ERROR, false, 0.9f);
                    else
                        game.ToggleDoor(DOOR_LEFT);
                    s_clickCooldown = 10;
                }
                if (gi.rightDoorToggle && inputOk) {
                    if (jamR && !game.GetDoors().IsDoorClosed(DOOR_RIGHT))
                        g_audio.Play(&g_pak, Snd::DOOR_ERROR, false, 0.9f);
                    else
                        game.ToggleDoor(DOOR_RIGHT);
                    s_clickCooldown = 10;
                }
            }
            // v2.21 hold-lights test (DEV toggle). When ON, the light stays on only
            // while its bumper (LB/RB) is held; when OFF, the original toggle.
            if (g_devHoldLights) {
                if (!jamL) game.SetLight(DOOR_LEFT,  gi.leftShoulderHeld);
                if (!jamR) game.SetLight(DOOR_RIGHT, gi.rightShoulderHeld);
            } else {
                if (gi.leftLightToggle && inputOk) {
                    game.ToggleLight(DOOR_LEFT);
                    s_clickCooldown = 10;
                }
                if (gi.rightLightToggle && inputOk) {
                    game.ToggleLight(DOOR_RIGHT);
                    s_clickCooldown = 10;
                }
            }
            // v2.62: the official FNAF1 console scheme for the monitor —
            // A raises it; in camera mode the D-pad moves a PRE-SELECTION
            // over the cam strip and A confirms the switch; B exits camera
            // mode. (Was: the D-pad switched instantly and A lowered.)
            if(game.GetCameras().IsMonitorUp()){
                if(gi.back){
                    game.SetCameraUp(false);
                    s_camHighlight = -1;
                } else {
                    if(s_camHighlight < 0){
                        const CameraId cur = game.GetCameras().GetCurrentCamera();
                        s_camHighlight = (int)cur;
                        if(s_camHighlight < (int)CAM_1A || s_camHighlight > (int)CAM_7)
                            s_camHighlight = (int)cameraSequence[0];
                    }
                    int idx = -1;
                    for(int i = 0; i < cameraSequenceLen; ++i)
                        if(cameraSequence[i] == (CameraId)s_camHighlight) idx = i;
                    if(idx < 0) idx = 0;
                    if(gi.cameraUp || gi.cameraLeft)
                        idx = (idx - 1 + cameraSequenceLen) % cameraSequenceLen;
                    if(gi.cameraDown || gi.cameraRight)
                        idx = (idx + 1) % cameraSequenceLen;
                    s_camHighlight = (int)cameraSequence[idx];
                    if(gi.cameraToggle && s_camHighlight != (int)game.GetCameras().GetCurrentCamera())
                        game.SwitchCamera((CameraId)s_camHighlight);
                }
            } else if(gi.cameraToggle){
                game.ToggleCamera();       // A raises the monitor
                s_camHighlight = -1;       // re-seeded to the current cam on open
            }
            // v2.22: Freddy nose honk easter egg (group 349, click "Active 26").
            // The official console maps it to Y; gated to the office (monitor down)
            // since the poster/nose is an office object.
            if(gi.yToggle && !game.GetCameras().IsMonitorUp())
                g_audio.Play(&g_pak, Snd::PARTY_FAVOR, false, 0.9f);
                RumbleKick(0, 12000, 0.05f);   // v2.37: nose honk — micro right-motor click
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

        // ---------------- LOGIC TICK (60 Hz) ----------------
        if(g_fade.phase != 1){
            accumulator += 1.0f/60.0f;
            while(accumulator >= tickDelta){
                GameState ns = game.Tick();
                accumulator -= tickDelta;
                tickCount++;
                // v2.37: hold the non-Golden scare until the RENDER clock
                // (scareElapsed, 60 Hz) ALSO passed the duration — the logic
                // timer alone could cut the scare short on frame drift
                // (the Foxy scare looked like "1 in 3"). v2.46: the 1987
                // creepy scare holds the same way (it routes to the title,
                // not to the force-close).
                if (ns == GAME_STATE_GAME_OVER && state == GAME_STATE_JUMPSCARE &&
                    (game.GetJumpscareAnimatronic() != ANIM_COUNT || g_creepyToMenu) &&
                    scareElapsed < (f32)game.GetJumpscareDurationSec())
                    break;
                if(ns != GAME_STATE_PLAYING){
                    if(ns != state) {
                        if (state == GAME_STATE_JUMPSCARE)
                            g_debugConsole.Print("SCARE end: rend=%.2f/%.2fs",
                                scareElapsed, (f32)game.GetJumpscareDurationSec());
                        // v2.46: the 1987 creepy scare returns to the title
                        // (customize START -> frame 14 -> scare -> title).
                        if (ns == GAME_STATE_GAME_OVER && g_creepyToMenu) {
                            g_creepyToMenu = false;
                            scareElapsed = 0.0f;
                            StartTransition(state, GAME_STATE_MENU); menu.Reset();
                        } else {
                            // v2.62 dump frame 4 "died" group 1: StopAll, then
                            // the STATIC loop rides the died screen (ch1, vol
                            // 100). The blip flash is render-only (see below).
                            if (ns == GAME_STATE_GAME_OVER) {
                                g_audio.StopAll();
                                g_audio.SetChannelVolume(1, CFVolumeToDb(100));
                                g_audio.PlayOnChannel(&g_pak, Snd::STATIC_LOOP, true, 1);
                            }
                            StartTransition(state, ns);   // fade-in next-day/game-over
                        }
                    }
                    break;
                }
            }
        }

        // ---------------- RENDER 60 Hz ----------------
        if(state!=GAME_STATE_JUMPSCARE) scareElapsed = 0.0f;
        if(FrameBegin(ColorForState(state, game.GetPower().GetPower()))){
            if(state==GAME_STATE_JUMPSCARE){
                // v2.48: fire the delayed XSCREAM once the scare anim reaches
                // its beat (Freddy frame 7; Bonnie/Chica after the kill-pan)
                if (s_screamPending && scareElapsed >= s_screamDelay) {
                    s_screamPending = false;
                    g_audio.Play(&g_pak, Snd::XSCREAM, false, 1.0f);
                }
                g_render.RenderJumpscare(game.GetJumpscareAnimatronic(), scareElapsed);
                // v2.26: Golden Freddy — per the original, instead of the normal
                // Game Over screen the game ABRUPTLY closes (only avoidable
                // by raising the Monitor in time). v2.35 decision: keep
                // exit(0) on purpose — it is SAFE (a plain process end; on
                // RGH dashboards the system may present it as a "crash" and
                // reboot to the dashboard, but the console is never harmed
                // and always recovers; README documents this as a feature).
                if (game.GetJumpscareAnimatronic()==ANIM_COUNT &&
                    scareElapsed >= (f32)game.GetJumpscareDurationSec()) {
                    // v2.46: the 1987 easter egg path returns to the title
                    // (customize START -> frame 14 creepy -> scare -> title);
                    // Golden Freddy's own kill keeps the force-close.
                    if (g_creepyToMenu) {
                        g_creepyToMenu = false;
                        scareElapsed = 0.0f;
                        StartTransition(state, GAME_STATE_MENU); menu.Reset();
                    } else {
                        exit(0);
                    }
                }
                scareElapsed += 1.0f/60.0f;   // v2.18: advance AFTER the first frame renders (start on frame 0)
            } else if(state==GAME_STATE_POWER_OUT){
                g_render.RenderPowerOut(game);
            } else if(state==GAME_STATE_NIGHT_COMPLETE){
                // v2.7.13: elapsed drives the data "6" roll (nights 1-4)
                g_render.RenderNightComplete(game.GetCurrentNight(), endFrames/60.0f);
            } else if(state==GAME_STATE_GAME_OVER){
                if (g_creepyT >= 0.0f) {
                    // v2.36: the 1/10000 "creepy start" screen (f14: full
                    // screen face, silent — the same render as Golden Freddy's
                    // kill, but WITHOUT the force-close; it ends on the title)
                    g_render.RenderJumpscare(ANIM_COUNT, g_creepyT);
                } else if(endFrames<600){
                    // v2.62 dump frame 4 "died" (was 1.6 s): fullscreen static
                    // (the static object's own 8-frame cycle) + ONE blip-flash
                    // pass ([23,23,23,4,25,6,8,9,10,21,22] @ 45 fps, then the
                    // object is destroyed). The phase lasts 10 s (group 4's
                    // timer 10000 ms) before the gameover frame. The burst is
                    // shifted past the transition fade (frame 4 has no fade
                    // of its own — the port's StartTransition fade is the
                    // only reason not to fire at endFrames 0).
                    g_render.DrawStaticOverlay(1.0f);
                    g_render.RenderDiedBurst(((f32)endFrames - 45.0f) / 60.0f);
                } else {
                    g_render.RenderGameOver();
                }
            } else if(game.GetCameras().IsMonitorUp()){
                g_render.SetCamHighlight(s_camHighlight);   // v2.62: pre-selection
                g_render.RenderCamera(game, s_phonePlaying);
            } else {
                g_render.SetCamHighlight(-1);
                // v2.53 (groups 380/378): the mute button exists only in the
                // +20..+40 s window of the call
                g_render.RenderOffice(game, s_phonePlaying && s_phoneT >= 20.0f && s_phoneT < 40.0f);
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
            if (g_creepyT >= 0.0f) {
                // v2.36: on the 1/10000 creepy screen — hold ~2.5 s (dump
                // f14 is silent and simply ends on the title, NO exit)
                g_creepyT += 1.0f/60.0f;
                done = g_creepyT >= 2.5f;
            } else if(state==GAME_STATE_NIGHT_COMPLETE){
                const i32 c = game.GetCurrentNight();
                if(c<5) done = (holdSec >= (f32)TimeConstants::NIGHT_COMPLETE_DISPLAY_SEC) || skipEnd;
                else    done = (holdSec >= 12.0f) || skipEnd;  // paycheck/overtime/pink slip
            } else {
                // v2.62 dump chain: "died" (static + blip flash + the static
                // loop) holds 10 s (frame 4 group 4 timer 10000 ms), then the
                // "gameover" backroom holds 10 s (frame 8 group 4's own
                // timer 10000). Frame 8 group 1 StopAlls on entry (the static
                // loop ends), group 5 re-rolls random := Random(10000)+1 EVERY
                // second, and a roll of 1 routes to the creepy start (f14).
                if (endFrames == 600) g_audio.StopAll();
                if (endFrames >= 600 && (endFrames % 60) == 0 &&
                    g_creepyT < 0.0f && (rand() % 10000) == 0) {
                    g_creepyT = 0.0f;   // the face replaces the backroom at once
                }
                done = (holdSec >= 20.0f) || skipEnd;
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
                g_creepyT = -1.0f;   // v2.36: leave the creepy screen (or never enter)
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
