/**
 * Five Nights at Freddy's 1 -- Recompilation
 * TextRenderer.h: D3DX-based text rendering for Xbox 360 XDK
 *
 * Uses ID3DXFont to draw text onto the D3D9 backbuffer.
 * All GDI constants (FW_BOLD, DT_*, etc.) are defined here
 * since Xbox 360 XDK has no wingdi.h.
 */

#ifndef FNAF_TEXTRENDERER_H
#define FNAF_TEXTRENDERER_H

#include "Types.h"

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#include <d3dx9.h>
typedef D3DDevice D3DDeviceX;
#else
#error "Only Xbox 360 target supported"
#endif

namespace fnaf {

// ============================================================
//  GDI-compatible constants for Xbox 360 XDK
//  (normally from wingdi.h, which doesn't exist on XDK)
// ============================================================
namespace FontConst {
    // Font weights
    static const i32 FW_DONTCARE  = 0;
    static const i32 FW_THIN      = 100;
    static const i32 FW_NORMAL    = 400;
    static const i32 FW_BOLD      = 700;
    static const i32 FW_BLACK     = 900;

    // Charset
    static const u8  DEFAULT_CHARSET   = 1;
    static const u8  ANSI_CHARSET      = 0;

    // Output precision
    static const u8  OUT_DEFAULT_PRECIS = 0;
    static const u8  OUT_STRING_PRECIS  = 1;

    // Quality
    static const u8  DEFAULT_QUALITY     = 0;
    static const u8  ANTIALIASED_QUALITY = 4;
    static const u8  CLEARTYPE_QUALITY   = 5;

    // Pitch and family
    static const u8  DEFAULT_PITCH   = 0;
    static const u8  FIXED_PITCH     = 1;
    static const u8  VARIABLE_PITCH  = 2;
    static const u8  FF_DONTCARE     = 0;
    static const u8  FF_ROMAN        = 16;
    static const u8  FF_SWISS        = 32;
    static const u8  FF_MODERN       = 48;
    static const u8  FF_SCRIPT       = 64;
    static const u8  FF_DECORATIVE   = 80;

    // DrawText format flags
    static const u32 DT_LEFT         = 0x00000000;
    static const u32 DT_TOP          = 0x00000000;
    static const u32 DT_CENTER       = 0x00000001;
    static const u32 DT_RIGHT        = 0x00000002;
    static const u32 DT_VCENTER      = 0x00000004;
    static const u32 DT_NOCLIP       = 0x00000100;
    static const u32 DT_SINGLELINE   = 0x00000020;
}

// ============================================================
//  TextRenderer
// ============================================================

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Initialize font. Call once after D3D device is ready. Use void* to avoid Xbox/PC type mismatch (VS2010)
    bool Init(void* device, i32 height = 24, const char* faceName = "Arial");
    void Shutdown();

    // Draw text at (x, y). color: D3DCOLOR (DWORD ARGB).
    // Call only between BeginScene and EndScene.
    void DrawText(i32 x, i32 y, const char* text, u32 color);
    void DrawTextf(i32 x, i32 y, u32 color, const char* fmt, ...);
    void DrawTextCentered(i32 y, const char* text, u32 color, i32 screenWidth = 1280);
    void DrawTextCenteredXY(i32 cx, i32 cy, const char* text, u32 color);

    bool IsInitialized() const;

private:
    void* m_device;
    void* m_font;
    bool               m_initialized;

    char m_fmtBuf[512];
};

// Pre-defined colors (D3DCOLOR = 0xAARRGGBB)
namespace TextColor {
    static const u32 WHITE       = 0xFFFFFFFF;
    static const u32 GRAY        = 0xFF888888;
    static const u32 DARK_GRAY   = 0xFF444444;
    static const u32 RED         = 0xFFFF0000;
    static const u32 GREEN       = 0xFF00FF00;
    static const u32 YELLOW      = 0xFFFFFF00;
    static const u32 ORANGE      = 0xFFFF8800;
    static const u32 DIM_WHITE   = 0xFFAAAAAA;
    static const u32 POWER_GREEN = 0xFF44FF44;
    static const u32 POWER_RED   = 0xFFFF4444;
    static const u32 CAMERA_NAME = 0xFFFFCC00;
}

} // namespace fnaf

#endif // FNAF_TEXTRENDERER_H
