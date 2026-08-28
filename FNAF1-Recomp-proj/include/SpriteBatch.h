/**
 * Five Nights at Freddy's 1 — Recompilation
 * SpriteBatch.h: 2D sprite batch for Xbox 360 D3D9 (VS2010 compatible)
 * Uses simple vertex/pixel shaders (vs_2_0 / ps_2_0)
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
// FVF constants may not be defined in Xbox 360 XDK d3d9.h
#ifndef D3DFVF_XYZRHW
#define D3DFVF_XYZRHW 0x002
#endif
#ifndef D3DFVF_DIFFUSE
#define D3DFVF_DIFFUSE 0x040
#endif
#ifndef D3DFVF_TEX1
#define D3DFVF_TEX1 0x00000100
#endif
#define SPRITE_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct SpriteVertex {
    float x, y, z, rhw;
    u32 color; // D3DCOLOR
    float u, v;
};

class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    bool Init(void* device);
    void Shutdown();

    void Begin();
    void Draw(void* tex, float x, float y, float w, float h,
              float u0, float v0, float u1, float v1,
              u32 color);
    // Draw with default UVs 0,0,1,1
    void Draw(void* tex, float x, float y, float w, float h, u32 color);
    void End();

private:
    void Flush();
    void SetupRenderState();

    void* m_device;
    SpriteVertex m_vertices[MAX_SPRITES_PER_BATCH * 4];
    int m_vertexCount;
    void* m_currentTexture;
    void* m_vertexShader;
    void* m_pixelShader;
    void* m_vsCodeBuffer;
    void* m_psCodeBuffer;
    void* m_vertexDecl;
};

} // namespace fnaf

#endif // FNAF_SPRITE_BATCH_H