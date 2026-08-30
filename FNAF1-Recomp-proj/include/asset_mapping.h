#pragma once
#include <string>

// Semantic asset names, corrected against the real game data
// (FiveNightsatFreddys.exe, parsed with ctfak-cpp "Recomp Pack").
// Every value is a fnaf1.pak texture name; the old placeholder handles
// (94/1/12/106-109/118/144) pointed at unrelated images.
//
// Identity provenance: handles were resolved from the office/title frame
// object tables (object name -> starting image) and verified visually:
//   OFFICE_BG  - object "Active 3", frame "Frame 1", placed at (0,0)
//   MENU_BG    - object "Active 2", frame "title", placed at (0,0)
//   CAM_MONITOR_BG - object "frame", layer 2 of the office frame
//   DOOR_BUTTON - objects "door open left/right", "light on left/right"
//   CAM_MAP    - object "Active 9" (floor plan with the YOU marker)
namespace fnaf_asset {
    inline const char* MENU_BG         = "img_431"; // title screen, 1280x720
    inline const char* OFFICE_BG       = "img_39";  // office parallax bg, 1600x720
    inline const char* CAM_MONITOR_BG  = "img_11";  // monitor frame, 1280x720
    inline const char* CAM_STATIC      = "img_18";  // video static overlay, 1280x720
    inline const char* CAM_MAP         = "img_164"; // camera floor plan, 400x400
    inline const char* OFFICE_LIGHTS   = "img_608"; // light cone overlay, 1600x253
    inline const char* LEFT_DOOR       = "img_103"; // left door, 223x720
    inline const char* RIGHT_DOOR      = "img_119"; // right door, 248x720
    inline const char* DOOR_BUTTON     = "img_129"; // door/light button, 62x120
    inline const char* MAP_BUTTON      = "img_167"; // cam map button, 60x40
    inline const char* MAP_ICON        = "img_170"; // cam map location icon, 31x25
    inline const char* MUTE_CALL       = "img_481"; // mute call button, 121x31
    inline const char* FREDDY_PORTRAIT = "img_573"; // "yellow bear" poster, 541x521
    inline const char* FLIP_UP_BAR     = "img_156"; // monitor flip-up strip, 792x82
    inline const char* FLIP_DOWN_BAR   = "img_162"; // monitor flip-down strip, 1070x82
}
