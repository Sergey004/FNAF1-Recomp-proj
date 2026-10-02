/**
 * InputSystem.cpp: Xbox 360 XInput mapping (§7)
 * VS2010 compatible, no lambdas
 */

#include "InputSystem.h"
#include <xtl.h>
#include <cmath>

namespace fnaf {

void UpdateInput(GameInput& out)
{
    // Clear toggles
    out.leftLightToggle = false;
    out.rightLightToggle = false;
    out.leftDoorToggle = false;
    out.rightDoorToggle = false;
    out.leftDoorAxis = 0.0f;
    out.rightDoorAxis = 0.0f;
    out.cameraToggle = false;
    out.cameraUp = false;
    out.cameraDown = false;
    out.cameraLeft = false;
    out.cameraRight = false;
    out.pause = false;
    out.back = false;
    out.tunerToggle = false;
    out.yToggle = false;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));
    if (XInputGetState(0, &state) != ERROR_SUCCESS) {
        out.lookDir = 0.0f;
        out.lookDirY = 0.0f;
        return;
    }

    // Left Stick X -> lookDir with deadzone
    float lx = (float)state.Gamepad.sThumbLX / 32767.0f;
    if (fabs(lx) < INPUT_DEADZONE) lx = 0.0f;
    if (lx > 1.0f) lx = 1.0f;
    if (lx < -1.0f) lx = -1.0f;
    out.lookDir = lx;

    // v2.60: Left Stick Y for the FNAF2 cursor (stick up = +1)
    float ly = (float)state.Gamepad.sThumbLY / 32767.0f;
    if (fabs(ly) < INPUT_DEADZONE) ly = 0.0f;
    out.lookDirY = ly;

    // Persistent prev states for toggle detection
    static bool lbPrev = false, rbPrev = false;
    static bool ltPrev = false, rtPrev = false;
    static bool aPrev = false, bPrev = false, startPrev = false;
    static bool dpadUpPrev = false, dpadDownPrev = false, dpadLeftPrev = false, dpadRightPrev = false;

    bool lbNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    bool rbNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
    out.leftShoulderHeld = lbNow;
    out.rightShoulderHeld = rbNow;
    out.leftLightToggle = lbNow && !lbPrev;
    out.rightLightToggle = rbNow && !rbPrev;
    lbPrev = lbNow; rbPrev = rbNow;

    bool ltNow = state.Gamepad.bLeftTrigger > 128;
    bool rtNow = state.Gamepad.bRightTrigger > 128;
    out.leftDoorToggle = ltNow && !ltPrev;
    out.rightDoorToggle = rtNow && !rtPrev;
    out.leftDoorAxis  = (float)state.Gamepad.bLeftTrigger  / 255.0f;   // v2.21 analog
    out.rightDoorAxis = (float)state.Gamepad.bRightTrigger / 255.0f;
    ltPrev = ltNow; rtPrev = rtNow;

    bool aNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    out.aHeld = aNow;   // v2.60: A level (the FNAF2 cursor click/hold)
    out.cameraToggle = aNow && !aPrev;
    aPrev = aNow;

    // D-Pad edge for camera nav (hold repeat not needed, edge only)
    bool upNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
    bool downNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
    bool leftNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
    bool rightNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
    out.cameraUp = upNow && !dpadUpPrev;
    out.cameraDown = downNow && !dpadDownPrev;
    out.cameraLeft = leftNow && !dpadLeftPrev;
    out.cameraRight = rightNow && !dpadRightPrev;
    dpadUpPrev = upNow; dpadDownPrev = downNow; dpadLeftPrev = leftNow; dpadRightPrev = rightNow;

    bool startNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
    out.pause = startNow && !startPrev;
    startPrev = startNow;

    bool bNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
    out.back = bNow && !bPrev;
    bPrev = bNow;

    // v2.7.11: Y button edge (PERSP tuner knob reset)
    static bool yPrev = false;
    bool yNow = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
    out.yToggle = yNow && !yPrev;
    yPrev = yNow;

    // v2.54: X button level — the title reads it as hold-to-wipe (5 s)
#ifndef XINPUT_GAMEPAD_X
#define XINPUT_GAMEPAD_X 0x4000
#endif
    out.xHeld = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;

    // v2.7.11: L3+R3 TOGETHER = PERSPECTIVE tuner enter/exit. The stick
    // buttons are never used anywhere else in the game, so the combo cannot
    // collide with lights/doors/camera. Fallback defines keep the build
    // green even on an XDK whose xinputdefs predates the thumb bits.
#ifndef XINPUT_GAMEPAD_LEFT_THUMB
#define XINPUT_GAMEPAD_LEFT_THUMB  0x0040
#endif
#ifndef XINPUT_GAMEPAD_RIGHT_THUMB
#define XINPUT_GAMEPAD_RIGHT_THUMB 0x0080
#endif
    static bool thumbsPrev = false;
    const bool thumbsNow =
        (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)  != 0 &&
        (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
    out.tunerToggle = thumbsNow && !thumbsPrev;
    thumbsPrev = thumbsNow;
}

MenuInput PollMenuInputFromGameInput(const GameInput& gi)
{
    MenuInput m;
    // Map GameInput to MenuInput for menu navigation reuse
    // Left Stick / D-Pad Up/Down for menu up/down
    if (gi.cameraUp || gi.lookDir < -0.5f) m.up = true;
    if (gi.cameraDown || gi.lookDir > 0.5f) m.down = true;
    if (gi.cameraLeft) m.left = true;
    if (gi.cameraRight) m.right = true;
    if (gi.cameraToggle) m.confirm = true;
    if (gi.back) m.back = true;
    if (gi.pause) m.confirm = true; // Start also confirm in menu
    return m;
}

} // namespace fnaf
