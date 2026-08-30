/**
 * Five Nights at Freddy's 1 -- Recompilation
 * TextRenderer.h: bitmap-font text rendering for Xbox 360 XDK
 *
 * The XDK has no ID3DXFont and no GDI -- the previous stub version of this
 * class drew NOTHING, which contributed to the "black screen". v2.2 renders
 * real text: an embedded 160x160 RGBA atlas (ASCII 32..126, Liberation Mono)
 * is decoded with D3DXCreateTextureFromFileInMemoryEx (present in the XDK
 * d3dx9tex.h) and each character is batched as a white-glyph quad through
 * fnaf::SpriteBatch, tinted by the requested color.
 */

#ifndef FNAF_TEXTRENDERER_H
#define FNAF_TEXTRENDERER_H

#include "Types.h"

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#include <d3d9.h>
#include <d3dx9.h>
typedef D3DDevice D3DDeviceX;
#else
#error "Only Xbox 360 target supported"
#endif

namespace fnaf {

class SpriteBatch;

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Initialize with the embedded font atlas. `batch` must be an
    // initialized fnaf::SpriteBatch (quads are queued through it).
    // heightPx: desired text height in pixels; glyphs are scaled
    // from their native 16 px cell accordingly.
    bool Init(void* device, SpriteBatch* batch, i32 heightPx = 16);

    void Shutdown();

    // Draw text at (x, y). color: D3DCOLOR (0xAARRGGBB).
    // Call only between BeginScene and EndScene.
    void DrawText(i32 x, i32 y, const char* text, u32 color);
    void DrawTextf(i32 x, i32 y, u32 color, const char* fmt, ...);
    void DrawTextCentered(i32 y, const char* text, u32 color, i32 screenWidth = 1280);
    void DrawTextCenteredXY(i32 cx, i32 cy, const char* text, u32 color);

    // Approximate width in pixels (monospace advance * chars)
    i32 MeasureText(const char* text) const;

    bool IsInitialized() const { return m_initialized; }

private:
    void*      m_device;
    void*      m_fontTexture;   // IDirect3DTexture9* from the atlas PNG
    SpriteBatch* m_batch;
    float      m_scale;         // requested height / native cell height
    bool       m_initialized;
    char       m_fmtBuf[512];
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
