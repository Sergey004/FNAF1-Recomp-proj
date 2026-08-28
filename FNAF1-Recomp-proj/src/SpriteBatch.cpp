/**
 * Five Nights at Freddy's 1 — Recompilation
 * SpriteBatch.cpp: Xbox 360 D3D9 sprite batch using a minimal
 * pass-through vertex/pixel shader pair.
 *
 * IMPORTANT: Xbox 360's GPU (Xenos) has NO fixed-function T&L path,
 * unlike desktop D3D9. SetVertexShader(NULL) does not fall back to
 * FFP the way it does on PC -- it just leaves the device with no
 * shader bound at all, which is why the previous version of this
 * file produced "Vertex fetch constant ... completely invalid" /
 * "A vertex shader must be set" errors in Xenia and would behave the
 * same on real hardware. Every draw call on this platform needs an
 * actual vertex + pixel shader, even for the simplest 2D quad.
 *
 * We use the D3DDECLUSAGE_POSITIONT ("pre-transformed position")
 * vertex declaration + a vertex shader that just passes the position
 * straight through, which is the standard way to do 2D/screen-space
 * sprite rendering on a fully-programmable pipeline without doing a
 * real projection transform.
 */

#include "SpriteBatch.h"
#include <cstdio>
#include <cstddef>
#include <vector>

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#include <d3d9.h>
#include <d3dx9.h>
#ifndef D3DDECLUSAGE_POSITIONT
#define D3DDECLUSAGE_POSITIONT 9
#endif
#else
#error "Only Xbox 360 target supported"
#endif

namespace fnaf {

// ------------------------------------------------------------------
//  Shader bytecode is compiled OFFLINE at build time from the .hlsl
//  sources in Shaders/ (see Shaders/README.md) -- NOT compiled at
//  runtime. Runtime HLSL compilation (D3DXCompileShader) works, but it
//  drags the whole shader compiler onto the console's PPC cores at
//  startup and isn't guaranteed present outside a devkit. Precompiled
//  .vsh/.psh bytecode just gets handed straight to CreateVertexShader/
//  CreatePixelShader -- no compiler needed at runtime at all.
// ------------------------------------------------------------------

static bool LoadShaderBytecode(const char* path, std::vector<u8>& outBytes)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("[SpriteBatch] Could not open shader file: %s\n", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fclose(f); return false; }
    outBytes.resize((size_t)size);
    size_t read = fread(outBytes.data(), 1, (size_t)size, f);
    fclose(f);
    return read == (size_t)size;
}

static const D3DVERTEXELEMENT9 kSpriteVertexDecl[] = {
    { 0, offsetof(SpriteVertex, x),     D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
    { 0, offsetof(SpriteVertex, color), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
    { 0, offsetof(SpriteVertex, u),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
    D3DDECL_END()
};

SpriteBatch::SpriteBatch()
    : m_device(0)
    , m_vertexCount(0)
    , m_currentTexture(0)
    , m_vertexShader(0)
    , m_pixelShader(0)
    , m_vsCodeBuffer(0)
    , m_psCodeBuffer(0)
    , m_vertexDecl(0)
{
}

SpriteBatch::~SpriteBatch()
{
    Shutdown();
}

bool SpriteBatch::Init(void* device)
{
    if (!device) return false;
    m_device = device;
    D3DDeviceX* dev = (D3DDeviceX*)device;

    // --- Load precompiled vertex shader bytecode ---
    std::vector<u8> vsBytes;
    if (!LoadShaderBytecode("game:\\Shaders\\sprite_vs.vsh", vsBytes)) {
        printf("[SpriteBatch] Failed to load sprite_vs.vsh -- did you run the offline\n"
               "shader build step? See Shaders/README.md.\n");
        return false;
    }
    IDirect3DVertexShader9* vs = NULL;
    HRESULT hr = dev->CreateVertexShader((const DWORD*)vsBytes.data(), &vs);
    if (FAILED(hr)) {
        printf("[SpriteBatch] CreateVertexShader failed (hr=0x%08X)\n", (unsigned)hr);
        return false;
    }
    m_vertexShader = vs;

    // --- Load precompiled pixel shader bytecode ---
    std::vector<u8> psBytes;
    if (!LoadShaderBytecode("game:\\Shaders\\sprite_ps.psh", psBytes)) {
        printf("[SpriteBatch] Failed to load sprite_ps.psh -- did you run the offline\n"
               "shader build step? See Shaders/README.md.\n");
        return false;
    }
    IDirect3DPixelShader9* ps = NULL;
    hr = dev->CreatePixelShader((const DWORD*)psBytes.data(), &ps);
    if (FAILED(hr)) {
        printf("[SpriteBatch] CreatePixelShader failed (hr=0x%08X)\n", (unsigned)hr);
        return false;
    }
    m_pixelShader = ps;

    // --- Vertex declaration ---
    IDirect3DVertexDeclaration9* decl = NULL;
    hr = dev->CreateVertexDeclaration(kSpriteVertexDecl, &decl);
    if (FAILED(hr)) {
        printf("[SpriteBatch] CreateVertexDeclaration failed (hr=0x%08X)\n", (unsigned)hr);
        return false;
    }
    m_vertexDecl = decl;

    m_vertexCount = 0;
    m_currentTexture = 0;
    return true;
}

void SpriteBatch::Shutdown()
{
    if (m_vertexShader) { ((IDirect3DVertexShader9*)m_vertexShader)->Release(); m_vertexShader = 0; }
    if (m_pixelShader)  { ((IDirect3DPixelShader9*)m_pixelShader)->Release();   m_pixelShader = 0; }
    if (m_vertexDecl)   { ((IDirect3DVertexDeclaration9*)m_vertexDecl)->Release(); m_vertexDecl = 0; }
    // m_vsCodeBuffer/m_psCodeBuffer are unused now -- shader bytecode is loaded
    // from disk into a local std::vector<u8> in Init() and doesn't need to be
    // kept alive after CreateVertexShader/CreatePixelShader.
    m_device = 0;
    m_vertexCount = 0;
    m_currentTexture = 0;
}

void SpriteBatch::Begin()
{
    m_vertexCount = 0;
    m_currentTexture = 0;
}

void SpriteBatch::Draw(void* tex, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1, u32 color)
{
    if (m_vertexCount + 4 > MAX_SPRITES_PER_BATCH * 4) {
        Flush();
    }
    if (tex != m_currentTexture && m_vertexCount > 0) {
        Flush();
    }
    m_currentTexture = tex;

    SpriteVertex* v = &m_vertices[m_vertexCount];
    v[0].x = x;     v[0].y = y;     v[0].z = 0.0f; v[0].rhw = 1.0f; v[0].u = u0; v[0].v = v0; v[0].color = color;
    v[1].x = x+w;   v[1].y = y;     v[1].z = 0.0f; v[1].rhw = 1.0f; v[1].u = u1; v[1].v = v0; v[1].color = color;
    v[2].x = x;     v[2].y = y+h;   v[2].z = 0.0f; v[2].rhw = 1.0f; v[2].u = u0; v[2].v = v1; v[2].color = color;
    v[3].x = x+w;   v[3].y = y+h;   v[3].z = 0.0f; v[3].rhw = 1.0f; v[3].u = u1; v[3].v = v1; v[3].color = color;
    m_vertexCount += 4;

    if (m_vertexCount >= MAX_SPRITES_PER_BATCH * 4 - 4) {
        Flush();
    }
}

void SpriteBatch::Draw(void* tex, float x, float y, float w, float h, u32 color)
{
    Draw(tex, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

void SpriteBatch::End()
{
    Flush();
}

void SpriteBatch::SetupRenderState()
{
    D3DDeviceX* dev = (D3DDeviceX*)m_device;
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    dev->SetRenderState(D3DRS_ALPHAREF, 1);
    dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);

    dev->SetTexture(0, (D3DTextureX*)m_currentTexture);

    // Programmable pipeline: explicit shaders + vertex declaration.
    // (No SetFVF / SetVertexShader(NULL) -- Xenos has no FFP fallback.)
    dev->SetVertexDeclaration((IDirect3DVertexDeclaration9*)m_vertexDecl);
    dev->SetVertexShader((IDirect3DVertexShader9*)m_vertexShader);
    dev->SetPixelShader((IDirect3DPixelShader9*)m_pixelShader);
}

void SpriteBatch::Flush()
{
    if (m_vertexCount == 0) return;

    SetupRenderState();

    D3DDeviceX* dev = (D3DDeviceX*)m_device;
    dev->SetTexture(0, (D3DTextureX*)m_currentTexture);

    int spriteCount = m_vertexCount / 4;
    int base = 0;
    for (int i = 0; i < spriteCount; ++i) {
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_vertices[base], sizeof(SpriteVertex));
        base += 4;
    }

    m_vertexCount = 0;
}

} // namespace fnaf
