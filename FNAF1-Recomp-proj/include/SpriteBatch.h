/**
 * Five Nights at Freddy's 1 — Recompilation
 * SpriteBatch.h: 2D sprite batch for Xbox 360 D3D9 (Xenos, VS2010 XDK)
 *
 * Xenos has NO fixed-function pipeline: every draw needs a real
 * vertex+pixel shader. Shaders are compiled at RUNTIME from an embedded
 * HLSL string via D3DXCompileShader (verified present in the XDK
 * d3dx9shader.h, profiles promoted to vs_3_0/ps_3_0). No offline .vsh/.psh
 * files are needed anymore -- this removes the manual fxc build step that
 * caused the "black screen" when Shaders/*.vsh were missing next to the xex.
 */

#ifndef FNAF_SPRITE_BATCH_H
#define FNAF_SPRITE_BATCH_H

#include "Types.h"

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#include <d3d9.h>
#include <d3dx9.h>
typedef D3DDevice D3DDeviceX;
typedef D3DTexture D3DTextureX;
#else
#error "Only Xbox 360 target supported"
#endif

namespace fnaf {

#define MAX_SPRITES_PER_BATCH 2048

// Screen-space quad vertex: ortho matrix applied in the vertex shader.
struct SpriteVertex {
    float x, y, z, w;   // screen-space position (z=0, w=1)
    u32   color;        // D3DCOLOR ARGB
    float u, v;
};

class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    bool Init(void* device);
    void Shutdown();

    // Diagnostics: human-readable reason of the last Init() failure
    // (empty when ready). main.cpp shows it on screen via XShowMessageBoxUI.
    const char* GetInitError() const { return m_initError; }
    bool IsReady() const { return m_ready; }

    void Begin();
    void Draw(void* tex, float x, float y, float w, float h,
              float u0, float v0, float u1, float v1,
              u32 color);
    // Draw with default UVs 0,0,1,1
    void Draw(void* tex, float x, float y, float w, float h, u32 color);
    void End();

    // v2.8.0: fullscreen CRT post-effect, drawn immediately (own draw call).
    // Procedural pixel shader: scanlines + corner vignette + fine grain.
    // Pure ALU -- no texture sampling, composes with the existing alpha
    // blend as a darken-only overlay. Strengths: 0..1 (0 disables a term).
    void DrawCRT(float timeSec, float scanStrength, float vignetteStrength,
                 float grainStrength);

private:
    void Flush();
    void SetupRenderState();   // (declaration was lost in workspace rollback)

    void* m_device;
    void* m_vertices;                 // SpriteVertex[MAX_SPRITES_PER_BATCH*4]
    int   m_vertexCount;
    void* m_currentTexture;
    void* m_vertexShader;             // IDirect3DVertexShader9*
    void* m_pixelShader;              // IDirect3DPixelShader9*
    void* m_crtPixelShader;           // IDirect3DPixelShader9* (v2.8.0 CRT)
    void* m_vertexDecl;               // IDirect3DVertexDeclaration9*
    bool  m_ready;
    char  m_initError[192];
};

} // namespace fnaf

#endif // FNAF_SPRITE_BATCH_H
