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
    g_debugConsole.Render(SCREEN_W, SCREEN_H);
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
        // Freddy laughs on the hour move like the original's giggle hooks
        if(hour>=1) g_audio.Play(&g_pak, Snd::FREDDY_LAUGH[hour%3], false, 0.9f);
    }
}
void OnPowerUpdate(f32 power){ s_lastPower=power; }

void OnJumpscare(AnimatronicId anim){
    const char* n[]={"Freddy","Bonnie","Chica","Foxy"};
    printf("*** JUMP SCARE by %s! ***\n",n[anim]);
    // group 228/322/408: XSCREAM (voiceover/garble stop too)
    g_audio.Stop(Snd::VOICEOVER[0]); g_audio.Stop(Snd::VOICEOVER[1]);
    g_audio.Stop(Snd::VOICEOVER[2]); g_audio.Stop(Snd::VOICEOVER[3]);
    g_audio.Stop(Snd::VOICEOVER[4]);
    s_phonePlaying=false;
    g_audio.Play(&g_pak, Snd::XSCREAM, false, 1.0f);
    // lives decrement on jumpscare (original Ini "lives")
    Progress::Load(g_prog);
    if (g_prog.lives > 0) g_prog.lives--;
    if (g_prog.lives <= 0) {
        g_prog.nextNight = 1;
        g_prog.beat5 = false;
        g_prog.beat6 = false;
        g_prog.beat7 = false;
    }
    Progress::Save(g_prog);
    g_ach.OnJumpscare();   // v2.14: "No Hiding"
}
void OnPowerOut(){
    printf("*** POWER OUT! ***\n");
    // group 285: powerdown + stop office loops
    g_audio.Stop(Snd::COLD_PRESC); g_audio.Stop(Snd::BUZZ_FAN);
    g_audio.Stop(Snd::BALLAST_HUM); g_audio.Stop(Snd::ROBOT_VOICE);
    g_audio.Stop(Snd::STATIC_LOOP); g_audio.Stop(Snd::STATIC2);
    s_phonePlaying=false;
    g_audio.Play(&g_pak, Snd::POWERDOWN, false, 1.0f);
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
void OnCameraChange(CameraId cam){
    if(cam==CAM_OFF){ printf("[Camera DOWN]\n"); g_audio.Stop(Snd::STATIC_LOOP); }
    else {
        printf("[Camera: %s]\n",CameraSystem::GetCameraName(cam));
        // group 129/143: camera switch blip + static while up
        g_audio.Play(&g_pak, Snd::CAMERA_SWITCH, false, 0.8f);
        g_audio.Play(&g_pak, Snd::STATIC_LOOP, true, 0.5f);
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
    // deep steps for Bonnie/Chica, giggle for Freddy (data: groups 198-243)
    if(a==ANIM_BONNIE||a==ANIM_CHICA) g_audio.Play(&g_pak, Snd::DEEP_STEPS, false, 0.9f);
    else if(a==ANIM_FREDDY)           g_audio.Play(&g_pak, Snd::FREDDY_LAUGH_LONG, false, 0.9f);
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
    g_audio.Play(&g_pak, Snd::VOICEOVER[night-1], false, 1.0f);
    s_phonePlaying = true;
}

// Phone Guy call scheduling (frame 3 groups 361-365): starts ~2.5 s
// into the night, one voiceover per night, nights 6/7 have no call.
static f32  s_phoneDelay = 2.5f;
static void TickPhoneCall(Game& game, f32 dt) {
    if (s_phoneDelay <= 0.0f) return;
    s_phoneDelay -= dt;
    if (s_phoneDelay <= 0.0f) {
        StartPhoneCall(game.GetCurrentNight());
    }
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
    printf("=== FNAF1-Recomp v2.7.13-nightflow built %s %s ===\n", __DATE__, __TIME__);

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
    g_debugConsole.Print("FNAF1-Recomp v2.7.13-nightflow (%s %s)", __DATE__, __TIME__);

    // Try load pak from Xbox 360 canonical locations (game:\ is XEX directory;
    // e:\/hdd:\ are common on JTAG/RGH dashboards like FSD or Aurora)
    const char* pakPaths[] = {
        "game:\\fnaf1.pak",
        "D:\\fnaf1.pak",
        "e:\\fnaf1.pak",
        "hdd:\\fnaf1.pak",
        "fnaf1.pak",
        "./fnaf1.pak"
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
    s_phoneDelay = 2.5f;
    f32 scareElapsed = 0.0f;
    i32 endFrames = 0;

    static const CameraId cameraSequence[]={CAM_1A,CAM_1B,CAM_1C,CAM_2A,CAM_2B,CAM_3,CAM_4A,CAM_4B,CAM_5,CAM_6,CAM_7};
    static const i32 cameraSequenceLen=11;

    while(true){
        GameInput gi; UpdateInput(gi);

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

        g_render.SetLookDir(gi.lookDir);   // office pan window (v2.5)
        g_render.Tick(1.0f/60.0f);
        g_audio.Tick();
        g_ach.Tick(1.0f/60.0f);     // v2.14: achievement toast timer
        TickFade(state, 1.0f/60.0f);  // v2.15: advance any running fade (may change `state`)

        // v2.15: office ambience fires exactly once when we actually land in
        // PLAYING (the fade-out of the "what day" card may delay the switch).
        if (state == GAME_STATE_PLAYING && !g_officeAmb && g_fade.phase == 0) {
            g_audio.Play(&g_pak, Snd::COLD_PRESC, true, 0.6f);
            g_audio.Play(&g_pak, Snd::BUZZ_FAN, true, 0.6f);
            g_audio.Play(&g_pak, Snd::BALLAST_HUM, true, 0.5f);
            // NOTE: robotvoice (sample #40, ch21) is ALSO a start-of-frame loop
            // in the original, but it starts MUTED (vol 0) and is raised only by
            // proximity triggers (Active 21) — a sub-system not ported yet.
            g_officeAmb = true;
        }
        if (state != GAME_STATE_PLAYING) g_officeAmb = false;

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
        }

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
            // v2.14: achievements screen — Y opens on the title, B/Y closes
            if(g_achScreen){
                if(gi.back || gi.yToggle) g_achScreen = false;
            } else {
                if(gi.yToggle) g_achScreen = true;
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
                    s_phoneMuted=false; s_phonePlaying=false; s_phoneDelay=2.5f;
                    tickCount=0; accumulator=0; menuFrameCounter=0; adCounter=0;
                    // v2.7.13: New Game shows the "HELP WANTED" newspaper first
                    StartTransition(state, menu.LastStartWasNewGame() ? GAME_STATE_INTRO_AD
                                                                      : GAME_STATE_NIGHT_START);
                }
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
            if(gi.leftDoorToggle) game.ToggleDoor(DOOR_LEFT);
            if(gi.rightDoorToggle) game.ToggleDoor(DOOR_RIGHT);
            if(gi.leftLightToggle) game.ToggleLight(DOOR_LEFT);
            if(gi.rightLightToggle) game.ToggleLight(DOOR_RIGHT);
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
            if(gi.pause){
                if(s_phonePlaying){ for(int v=0;v<5;++v) g_audio.Stop(Snd::VOICEOVER[v]); s_phonePlaying=false; }
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
        if(state==GAME_STATE_JUMPSCARE) scareElapsed += 1.0f/60.0f;
        else scareElapsed = 0.0f;
        if(FrameBegin(ColorForState(state, game.GetPower().GetPower()))){
            if(state==GAME_STATE_JUMPSCARE){
                g_render.RenderJumpscare(game.GetJumpscareAnimatronic(), scareElapsed);
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
                    s_phoneMuted=false; s_phonePlaying=false; s_phoneDelay=2.5f;
                    StartTransition(state, GAME_STATE_NIGHT_START); tickCount=0; accumulator=0;
                    continue;
                }
                // Update progress flags on night completion screens (original next day / the end timing)
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
