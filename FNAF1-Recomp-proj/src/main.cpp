/**
 * Five Nights at Freddy's 1 -- Recompilation
 * main.cpp: Xbox 360 only - D3D9 + Main Menu + InputSystem (§7/§8)
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "fnaf.h"
#include "DebugConsole.h"
#include "MenuSystem.h"
#include "SpriteBatch.h"
#include "PakLoader.h"
#include "InputSystem.h"
#include "asset_mapping.hpp"

#include <xtl.h>
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
static bool              g_pakLoaded = false;

static const i32 SCREEN_W = 1280;
static const i32 SCREEN_H = 720;
static float g_lookDir = 0.0f;

static bool InitD3D() {
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_pD3D) return false;
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth   = SCREEN_W;
    d3dpp.BackBufferHeight  = SCREEN_H;
    d3dpp.BackBufferFormat  = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount   = 1;
    d3dpp.MultiSampleType   = D3DMULTISAMPLE_NONE;
    d3dpp.SwapEffect        = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.Windowed = FALSE;
    d3dpp.hDeviceWindow = NULL;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    d3dpp.FullScreen_RefreshRateInHz = 60;
    HRESULT hr = g_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);
    if (FAILED(hr)) return false;
    g_text.Init(g_pd3dDevice, 26, "Arial");
    g_debugConsole.Init(&g_text, 128, 18);
    g_batch.Init(g_pd3dDevice);
    return true;
}
static void ShutdownD3D() {
    g_pak.Unload();
    g_batch.Shutdown();
    g_text.Shutdown();
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}
static DWORD ColorForState(GameState state, f32 power) {
    switch (state) {
        case GAME_STATE_MENU: return D3DCOLOR_XRGB(10, 10, 10);
        case GAME_STATE_NIGHT_START: return D3DCOLOR_XRGB(5, 5, 20);
        case GAME_STATE_PLAYING: { BYTE g = (BYTE)((power/100.0f)*30.0f); return D3DCOLOR_XRGB(5, 5+g, 20); }
        case GAME_STATE_POWER_OUT: return D3DCOLOR_XRGB(30, 2, 2);
        case GAME_STATE_JUMPSCARE: return D3DCOLOR_XRGB(200, 200, 200);
        case GAME_STATE_NIGHT_COMPLETE: return D3DCOLOR_XRGB(5, 40, 5);
        case GAME_STATE_GAME_OVER: return D3DCOLOR_XRGB(40, 0, 0);
        default: return D3DCOLOR_XRGB(0,0,0);
    }
}
static void DrawButtonPrompt(const char* label, float x, float y, u32 color) {
    char buf[32]; sprintf(buf, "[%s]", label);
    g_text.DrawText((i32)x, (i32)y, buf, color);
}
static void RenderMenu(MenuSystem& menu) {
    if (!g_pd3dDevice) return;
    // dark blue-ish clear for menu background
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0x10,0x10,0x20), 1.0f, 0);
    HRESULT hr = g_pd3dDevice->BeginScene();
    if (FAILED(hr)) { g_pd3dDevice->Present(NULL,NULL,NULL,NULL); return; }

    // If pak loaded, draw background texture -- pick menu background from PAK
    if (g_pakLoaded && g_pak.GetTextureCount() > 0) {
        PakLoadedTexture* bg = NULL;
        // Known menu background from dump: img_417_1280x720
        bg = g_pak.FindTexture("img_417");
        if (!bg) {
            // Fallback: first 1280x720 img_*
            for (int i = 0; i < g_pak.GetTextureCount(); ++i) {
                PakLoadedTexture* t = g_pak.GetTexture(i);
                if (!t || !t->texture) continue;
                if (t->origWidth == 1280 && t->origHeight == 720 && strstr(t->name, "img_") != NULL) { bg = t; break; }
            }
        }
        // Last resort: largest texture
        if (!bg) {
            u32 bestArea = 0;
            for (int i = 0; i < g_pak.GetTextureCount(); ++i) {
                PakLoadedTexture* t = g_pak.GetTexture(i);
                if (!t || !t->texture) continue;
                u32 area = t->origWidth * t->origHeight;
                if (area > bestArea) { bestArea = area; bg = t; }
            }
        }
        if (bg && bg->texture) {
            g_batch.Begin();
            g_batch.Draw(bg->texture, 0, 0, (float)SCREEN_W, (float)SCREEN_H, 0xFFFFFFFF);
            g_batch.End();
        } else {
            g_debugConsole.Print("No valid texture to draw (count=%d)", g_pak.GetTextureCount());
        }
    } else {
        // No pak – nothing to draw
    }

    // Render menu text on top
    menu.Render(&g_text, SCREEN_W, SCREEN_H);
    g_debugConsole.Render(SCREEN_W, SCREEN_H);
    g_pd3dDevice->EndScene();
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}
static void RenderHUD(Game& game, GameState state, float lookDir) {
    if (!g_pd3dDevice) return;
    g_pd3dDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, ColorForState(state, game.GetPower().GetPower()),1.0f,0);
    HRESULT hr=g_pd3dDevice->BeginScene(); if(FAILED(hr)){g_pd3dDevice->Present(NULL,NULL,NULL,NULL);return;}
    if (g_pakLoaded && state==GAME_STATE_PLAYING) {
        PakLoadedTexture* office = g_pak.FindTexture("img_1_1280x720");
        if (!office) {
            // fallback: find any texture with 1280x720 size
            for (int i=0; i<g_pak.GetTextureCount(); ++i) {
                PakLoadedTexture* t = g_pak.GetTexture(i);
                if (t && t->origWidth == 1280 && t->origHeight == 720) {
                    office = t;
                    break;
                }
            }
        }
        if (!office) office = g_pak.GetTexture(0);
        if (office && office->texture) {
            float parallaxX = lookDir * 20.0f;
            g_batch.Begin();
            g_batch.Draw(office->texture, parallaxX, 0, (float)SCREEN_W, (float)SCREEN_H, 0xFFFFFFFF);
            g_batch.End();
        }
    }
    // No textual HUD – render only Pak assets
    // switch(state){
    //     case GAME_STATE_NIGHT_START: // ...
    //     case GAME_STATE_PLAYING: // ...
    //     case GAME_STATE_POWER_OUT: // ...
    //     case GAME_STATE_JUMPSCARE: // ...
    //     case GAME_STATE_NIGHT_COMPLETE: // ...
    //     case GAME_STATE_GAME_OVER: // ...
    //     default: break;
    // }
    g_debugConsole.Render(SCREEN_W,SCREEN_H);
    g_pd3dDevice->EndScene(); g_pd3dDevice->Present(NULL,NULL,NULL,NULL);
}

static bool ShowPakErrorScreen(){
    static bool shown = false;
    if (shown) { exit(0); return false; }
    shown = true;
    // Show error and exit to Dashboard on OK
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
            // Cancelled e.g. DVD eject - also exit to Dashboard
            exit(0);
        }
    } else {
        exit(0);
    }
    return false;
}
static i32 s_lastHour=-1; static f32 s_lastPower=-1.0f;
void OnTimeUpdate(i32 hour){ s_lastHour=hour; }
void OnPowerUpdate(f32 power){ s_lastPower=power; }
void OnJumpscare(AnimatronicId anim){ const char* n[]={"Freddy","Bonnie","Chica","Foxy"}; printf("*** JUMP SCARE by %s! ***\n",n[anim]); }
void OnPowerOut(){ printf("*** POWER OUT! ***\n"); }
void OnMusicBoxStart(){ printf("(Music box starts...)\n"); }
void OnMusicBoxStop(){ printf("(Music box stops...)\n"); }
void OnNightComplete(i32 night){ printf("\n6 AM -- Night %d Complete!\n",night); }
void OnGameOver(){ printf("--- GAME OVER ---\n"); }
void OnCameraChange(CameraId cam){ if(cam==CAM_OFF)printf("[Camera DOWN]\n"); else printf("[Camera: %s]\n",CameraSystem::GetCameraName(cam)); }
void OnDoorChange(DoorSide s,bool c){ printf("[Door %s: %s]\n",s==DOOR_LEFT?"Left":"Right",c?"CLOSED":"OPEN"); }
void OnLightChange(DoorSide s,bool o){ printf("[Light %s: %s]\n",s==DOOR_LEFT?"Left":"Right",o?"ON":"OFF"); }
void OnAnimatronicMove(AnimatronicId a,RoomId r){ const char* n[]={"Freddy","Bonnie","Chica","Foxy"}; RoomInfo i=RoomSystem::GetRoomInfo(r); printf("[AI] %s -> %s\n",n[a],i.name); }
void OnFoxyStageChange(FoxyStage s){ const char* t[]={"Curtain Closed","Peeking","Gone","RUNNING!"}; printf("[AI] Foxy: %s\n",t[s]); }
void OnFoxyDoorBang(f32 p){ printf("[AI] Foxy bangs! -%.1f%%\n",p); }

int main(int argc, char* argv[]){
    Game game;
    GameCallbacks cb; cb.onTimeUpdate=OnTimeUpdate; cb.onPowerUpdate=OnPowerUpdate; cb.onJumpscare=OnJumpscare; cb.onPowerOut=OnPowerOut; cb.onMusicBoxStart=OnMusicBoxStart; cb.onMusicBoxStop=OnMusicBoxStop; cb.onNightComplete=OnNightComplete; cb.onGameOver=OnGameOver; cb.onCameraChange=OnCameraChange; cb.onDoorChange=OnDoorChange; cb.onLightChange=OnLightChange; cb.onAnimatronicMove=OnAnimatronicMove; cb.onFoxyStageChange=OnFoxyStageChange; cb.onFoxyDoorBang=OnFoxyDoorBang; game.SetCallbacks(cb);
    i32 cmdNight=0; if(argc>1){ cmdNight=atoi(argv[1]); if(cmdNight<1)cmdNight=1; if(cmdNight>7)cmdNight=7; }
    if(!InitD3D()){ printf("FATAL: InitD3D failed\n"); return 1; }
    // Try load pak from Xbox 360 canonical locations (game:\ is XEX directory)
    const char* pakPaths[] = {
        "game:\\fnaf1.pak",
        "D:\\fnaf1.pak",
        "fnaf1.pak",
        "./fnaf1.pak",
        "fnaf1.pak"
    };
    for(int i=0;i<5;++i){
        const char* p = pakPaths[i];
        if(g_pak.Load(p,g_pd3dDevice)){ g_pakLoaded=true; break; }
    }
    if(g_pakLoaded){ printf("Pak loaded: %d tex %d snd\n",g_pak.GetTextureCount(),g_pak.GetSoundCount()); g_debugConsole.Print("Pak: %d tex %d snd",g_pak.GetTextureCount(),g_pak.GetSoundCount());
        // Dump texture names for reverse engineering
        for(int i=0;i<g_pak.GetTextureCount();++i){ PakLoadedTexture* t = g_pak.GetTexture(i); if(t) printf("  %03d: %s %dx%d\n", i, t->name, t->origWidth, t->origHeight); }
    } else {
        printf("Pak not found - will show error screen\n");
        // Show FreeMyXe-style error screen (blocks until user input)
        bool cont = ShowPakErrorScreen();
        if(!cont){ ShutdownD3D(); return 0; }
        // else continue with text fallback
    }
    MenuSystem menu; menu.Init(3,0);
    if(cmdNight!=0) game.Init(cmdNight);
    GameState state=(cmdNight!=0)?GAME_STATE_NIGHT_START:GAME_STATE_MENU;
    i32 tickCount=0, lastCameraTick=0; (void)lastCameraTick;
    static const CameraId cameraSequence[]={CAM_1A,CAM_1B,CAM_1C,CAM_2A,CAM_2B,CAM_3,CAM_4A,CAM_4B,CAM_5,CAM_6,CAM_7};
    static const i32 cameraSequenceLen=11;
    float accumulator=0.0f;
    const float tickDelta = 1.0f/30.0f; // logic 30Hz, render 60Hz (§8)
    g_lookDir=0.0f;
    static int menuFrameCounter = 0;
    while(true){
        GameInput gi; UpdateInput(gi);
        g_lookDir = gi.lookDir;
        if(state==GAME_STATE_MENU){
            // Debounce input for first ~120 frames to avoid boot button bounce / DVD eject
            if(menuFrameCounter < 120){
                menuFrameCounter++;
                RenderMenu(menu);
                Sleep(16); tickCount++; continue;
            }
            MenuInput mi; mi.up=gi.cameraUp; mi.down=gi.cameraDown; mi.left=gi.cameraLeft; mi.right=gi.cameraRight; mi.confirm=gi.cameraToggle; mi.back=gi.back;
            // Also map LB/RB not needed in menu, but allow Up/Down via left stick already in gi
            if(gi.lookDir < -0.5f) mi.left=true; if(gi.lookDir > 0.5f) mi.right=true;
            MenuAction act=menu.Update(mi);
            RenderMenu(menu);
            if(act==MENU_ACTION_START_NIGHT){ i32 night=menu.GetSelectedNight(); game.Init(night); state=GAME_STATE_NIGHT_START; tickCount=0; accumulator=0; menuFrameCounter=0; }
            else if(act==MENU_ACTION_EXIT) break;
            Sleep(16); tickCount++; continue;
        } else {
            menuFrameCounter = 0;
        }
        // Gameplay input (§7/§8)
        if(state==GAME_STATE_PLAYING){
            if(gi.leftDoorToggle) game.ToggleDoor(DOOR_LEFT);
            if(gi.rightDoorToggle) game.ToggleDoor(DOOR_RIGHT);
            if(gi.leftLightToggle) game.ToggleLight(DOOR_LEFT);
            if(gi.rightLightToggle) game.ToggleLight(DOOR_RIGHT);
            if(gi.cameraToggle || gi.back){
                // Back also closes camera if up
                if(game.GetCameras().IsMonitorUp() && gi.back) game.SetCameraUp(false);
                else game.ToggleCamera();
            }
            if(game.GetCameras().IsMonitorUp() && (gi.cameraUp||gi.cameraDown||gi.cameraLeft||gi.cameraRight)){
                // Simple spatial nav via sequence
                CameraId cur=game.GetCameras().GetCurrentCamera();
                int idx=-1; for(int i=0;i<cameraSequenceLen;++i) if(cameraSequence[i]==cur) idx=i;
                if(idx>=0){
                    if(gi.cameraUp||gi.cameraLeft) idx=(idx-1+cameraSequenceLen)%cameraSequenceLen;
                    if(gi.cameraDown||gi.cameraRight) idx=(idx+1)%cameraSequenceLen;
                    game.SwitchCamera(cameraSequence[idx]);
                }
            }
            if(gi.pause){
                state=GAME_STATE_MENU; menu.Reset(); continue;
            }
        }
        // Frame-rate independent logic (§8): 30 tick/sec, render 60Hz
        accumulator += 1.0f/60.0f;
        while(accumulator >= tickDelta){
            state = game.Tick();
            accumulator -= tickDelta;
            tickCount++;
            if(state!=GAME_STATE_PLAYING) break;
        }
        // Render at 60Hz
        if(state != GAME_STATE_MENU){
            RenderHUD(game, state, g_lookDir);
            if(tickCount % 60==0 && state==GAME_STATE_PLAYING){
                g_debugConsole.Print("[%s] Power: %5.1f%% Usage:%d Tick:%d Look:%.2f", game.GetTimer().GetHourString(), game.GetPower().GetPower(), game.GetPower().GetUsageLevel(), tickCount, g_lookDir);
            }
        }
        if(state==GAME_STATE_GAME_OVER || state==GAME_STATE_NIGHT_COMPLETE){
            // Allow B/Start to return to menu
            if(gi.back || gi.cameraToggle || gi.pause){
                if(state==GAME_STATE_NIGHT_COMPLETE){
                    i32 completed=game.GetCurrentNight();
                    if(completed>=1 && completed<7){ menu.SetUnlockedNight(completed+1); menu.SetHasSave(true); }
                }
                state=GAME_STATE_MENU; menu.Reset(); tickCount=0; accumulator=0; continue;
            }
        }
        if(state==GAME_STATE_NIGHT_COMPLETE || state==GAME_STATE_GAME_OVER){
            for(int w=0;w<180;++w){ RenderHUD(game,state,g_lookDir); Sleep(16); GameInput early; UpdateInput(early); if(early.back||early.cameraToggle||early.pause) break; }
            if(state==GAME_STATE_NIGHT_COMPLETE){
                i32 c=game.GetCurrentNight(); if(c>=1&&c<7){ menu.SetUnlockedNight(c+1); menu.SetHasSave(true); }
            }
            state=GAME_STATE_MENU; menu.Reset(); tickCount=0; accumulator=0; continue;
        }
        Sleep(16);
        if(tickCount>36000) break;
    }
    ShutdownD3D();
    return 0;
}
