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

    // v2.57: POINT-sampled draw (nearest neighbour) — the title/camera static
    // keeps its coarse 1024px grain when stretched to 1280 (the original's
    // Clickteam default is point sampling; linear blurs the noise soft).
    void DrawPoint(void* tex, float x, float y, float w, float h,
                   float u0, float v0, float u1, float v1,
                   u32 color);

    // General-purpose explicit triangle list in screen space (same vertex
    // layout as the quad path); one DrawPrimitiveUP for a whole mesh.
    // Flushes any queued quads first, so paint order stays exact.
    // (No longer used by the Perspective path -- v2.8 replaced the bent mesh
    // with the render-target + parabola shader.)
    void DrawTriangles(void* tex, const SpriteVertex* verts, int vertexCount);

    // v2.8 clean-room Perspective shader (see GameRender.cpp / RPanorama.fx).
    // The original HWA "Perspective" extension grabs the ALREADY DRAWN flat
    // layer 0 off the screen and re-projects it through a ps_2_0 parabola
    // pixel shader. We mirror that with a 1280x720 render-target capture:
    //   BeginSceneCapture -> (draw layer 0) -> EndSceneCapture (resolves the
    //   EDRAM capture into a sampleable texture) -> DrawPerspective (full-
    //   screen parabola pass). Layers 2/3 are drawn flat afterwards.
    bool PerspectiveReady() const { return m_panReady; }
    void BeginSceneCapture(u32 clearColor);
    void EndSceneCapture();
    void DrawPerspective(float zoom, float centerY, float curve);

    void End();

private:
    void SetupRenderState();
    void Flush();

    void* m_device;
    void* m_vertices;                 // SpriteVertex[MAX_SPRITES_PER_BATCH*4]
    int   m_vertexCount;
    void* m_currentTexture;
    void* m_vertexShader;             // IDirect3DVertexShader9*
    void* m_pixelShader;              // IDirect3DPixelShader9*
    void* m_vertexDecl;               // IDirect3DVertexDeclaration9*
    void* m_panRT;                    // D3DSurface*     (1280x720 EDRAM capture RT)
    void* m_panTex;                   // D3DTexture*     (resolve destination, sampleable)
    void* m_panPS;                    // D3DPixelShader* (parabola panorama)
    void* m_backRT;                   // D3DSurface*     (saved back-buffer RT0)
    bool  m_panReady;
    bool  m_ready;
    char  m_initError[192];
};

} // namespace fnaf

#endif // FNAF_SPRITE_BATCH_H
