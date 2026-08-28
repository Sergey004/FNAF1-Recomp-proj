/**
 * Five Nights at Freddy's 1 — Recompilation
 * CameraSystem.cpp: Camera monitor implementation
 */

#include "CameraSystem.h"

namespace fnaf {

static const char* s_cameraNames[12] = {
    "CAM 1A",  // Show Stage
    "CAM 1B",  // Dining Area
    "CAM 1C",  // Pirate Cove
    "CAM 2A",  // West Hall
    "CAM 2B",  // W. Hall Corner
    "CAM 3",   // Supply Closet
    "CAM 4A",  // East Hall
    "CAM 4B",  // E. Hall Corner
    "CAM 5",   // Backstage
    "CAM 6",   // Kitchen
    "CAM 7",   // Restrooms
    "OFF"      // Monitor down
};

CameraSystem::CameraSystem()
    : m_monitorUp(false)
    , m_currentCamera(CAM_OFF)
{
}

void CameraSystem::Reset() {
    m_monitorUp = false;
    m_currentCamera = CAM_OFF;
}

bool CameraSystem::ToggleMonitor() {
    m_monitorUp = !m_monitorUp;
    if (!m_monitorUp) {
        m_currentCamera = CAM_OFF;
    } else if (m_currentCamera == CAM_OFF) {
        m_currentCamera = CAM_1A; // Default camera when opening monitor
    }
    return m_monitorUp;
}

void CameraSystem::SetMonitorUp(bool up) {
    m_monitorUp = up;
    if (!up) {
        m_currentCamera = CAM_OFF;
    }
}

bool CameraSystem::IsMonitorUp() const {
    return m_monitorUp;
}

void CameraSystem::SwitchCamera(CameraId cam) {
    if (cam >= CAM_1A && cam <= CAM_7) {
        m_currentCamera = cam;
        // Switching camera also raises the monitor if it was down
        if (!m_monitorUp) {
            m_monitorUp = true;
        }
    }
}

CameraId CameraSystem::GetCurrentCamera() const {
    return m_currentCamera;
}

bool CameraSystem::IsViewingCamera(CameraId cam) const {
    return m_monitorUp && m_currentCamera == cam;
}

bool CameraSystem::IsViewingPirateCove() const {
    return IsViewingCamera(CAM_1C);
}

const char* CameraSystem::GetCameraName(CameraId cam) {
    i32 index = static_cast<i32>(cam);
    if (index >= 0 && index < 12) {
        return s_cameraNames[index];
    }
    return "UNKNOWN";
}

} // namespace fnaf