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
    float leftDoorAxis;     // v2.21: LT analog (0..1) — "analog door" test feature
    float rightDoorAxis;    // v2.21: RT analog (0..1)
    bool cameraToggle;      // A toggle
    bool cameraUp;
    bool cameraDown;
    bool cameraLeft;
    bool cameraRight;
    bool pause;             // Start
    bool back;              // B
    bool leftShoulderHeld;  // LB level (debug combos / sprite browser)
    bool rightShoulderHeld; // RB level
    bool tunerToggle;       // v2.7.11: L3+R3 edge (PERSPECTIVE tuner enter/exit)
    bool yToggle;           // v2.7.11: Y button edge (tuner knob reset)
    bool xHeld;             // v2.54: X button level (title hold-to-wipe save)

    GameInput() : lookDir(0), leftLightToggle(false), rightLightToggle(false),
                  leftDoorToggle(false), rightDoorToggle(false),
                  leftDoorAxis(0), rightDoorAxis(0),
                  cameraToggle(false), cameraUp(false), cameraDown(false),
                  cameraLeft(false), cameraRight(false), pause(false), back(false),
                  leftShoulderHeld(false), rightShoulderHeld(false),
                  tunerToggle(false), yToggle(false), xHeld(false) {}
};

// Poll XInput and fill GameInput with toggle detection (edge)
void UpdateInput(GameInput& out);

// Also expose low-level MenuInput for menu (reuse GameInput)
MenuInput PollMenuInputFromGameInput(const GameInput& gi);

} // namespace fnaf

#endif // FNAF_INPUT_SYSTEM_H
