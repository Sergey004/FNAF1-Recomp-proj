/**
 * Five Nights at Freddy's 1 — Recompilation
 * CameraSystem.h: Camera monitor system
 */

#ifndef FNAF_CAMERA_SYSTEM_H
#define FNAF_CAMERA_SYSTEM_H

#include "Types.h"

namespace fnaf {

class CameraSystem {
public:
    CameraSystem();

    // Reset for a new night
    void Reset();

    // Toggle camera monitor up/down
    // Returns true if monitor is now UP.
    bool ToggleMonitor();

    // Force monitor state
    void SetMonitorUp(bool up);

    // Is the monitor currently up?
    bool IsMonitorUp() const;

    // Switch to a specific camera
    void SwitchCamera(CameraId cam);

    // Get the currently viewed camera
    CameraId GetCurrentCamera() const;

    // Check if a specific camera is being viewed
    bool IsViewingCamera(CameraId cam) const;

    // Check if Pirate Cove (CAM 1C) is being viewed
    // This is important for Foxy's AI.
    bool IsViewingPirateCove() const;

    // Map display name for a camera
    static const char* GetCameraName(CameraId cam);

    // Total number of cameras
    static const i32 CAMERA_COUNT = 11;

private:
    bool     m_monitorUp;
    CameraId m_currentCamera;
};

} // namespace fnaf

#endif // FNAF_CAMERA_SYSTEM_H
