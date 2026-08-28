/**
 * InputSystem.h: Xbox 360 controller mapping - console-accurate (§7)
 * VS2010 compatible, XInput only
 */

#ifndef FNAF_INPUT_SYSTEM_H
#define FNAF_INPUT_SYSTEM_H

#include "Types.h"
#include "MenuSystem.h"

namespace fnaf {

#define INPUT_DEADZONE 0.24f

struct GameInput {
    float lookDir;          // -1.0 left, 0 center, 1.0 right (Left Stick X)
    bool leftLightToggle;   // LB toggle
    bool rightLightToggle;  // RB toggle
    bool leftDoorToggle;    // LT toggle (>128)
    bool rightDoorToggle;   // RT toggle (>128)
    bool cameraToggle;      // A toggle
    bool cameraUp;
    bool cameraDown;
    bool cameraLeft;
    bool cameraRight;
    bool pause;             // Start
    bool back;              // B

    GameInput() : lookDir(0), leftLightToggle(false), rightLightToggle(false),
                  leftDoorToggle(false), rightDoorToggle(false),
                  cameraToggle(false), cameraUp(false), cameraDown(false),
                  cameraLeft(false), cameraRight(false), pause(false), back(false) {}
};

// Poll XInput and fill GameInput with toggle detection (edge)
void UpdateInput(GameInput& out);

// Also expose low-level MenuInput for menu (reuse GameInput)
MenuInput PollMenuInputFromGameInput(const GameInput& gi);

} // namespace fnaf

#endif // FNAF_INPUT_SYSTEM_H
