/**
 * Five Nights at Freddy's 1 — Recompilation
 * SpriteBatch.cpp: Xbox 360 D3D9 sprite batch, self-contained.
 *
 * Why this file exists the way it does
 * ------------------------------------
 * The Xenos GPU has no fixed-function T&L path, unlike desktop D3D9.
 * SetVertexShader(NULL) does not fall back to FFP the way it does on PC.
 * Every draw call on this platform needs an actual vertex + pixel shader,
 * even for the simplest 2D quad.
 *
 * v2.2b: the shader pair is compiled AT RUNTIME from the HLSL strings below
 * with D3DXCompileShader (present in every XDK: d3dx9shader.h; on the 360
 * the only shader profiles are vs_3_0 and ps_3_0 -- any other vs_ or ps_
 * profile name is promoted there). NOTE TO EDITORS: never write "star-slash"
 * sequences (asterisk + slash) inside this comment, e.g. a glob like
 * "vs_* / ps_*" -- it silently closes the block comment and turns the rest
 * of the header into C++ tokens (that is exactly the v2.3.1 compile break).
 *
 * The previous scheme -- offline .vsh/.psh bytecode produced by the PC fxc.exe
 * and loaded from game:\Shaders\ -- could never work: Xenos does not execute
 * PC D3D9 shader tokens (see hedge-dev/XenosRecomp: 360 shader binaries are
 * a completely separate microcode), and if the files were not deployed the
 * init failed silently -> black screen. Now there is nothing external to
 * deploy, and if compilation ever fails, GetInitError() returns a readable
 * reason that main.cpp shows as a full-screen message instead of black.
 *
 * Vertex pipeline: POSITION float4 (screen space) -> vertex shader maps to
 * clip space with two scalar constant registers (c0 = scales, c1 = offsets).
 * Deliberately NOT a float4x4: HLSL matrices have a column_major/row_major
 * packing default that silently transposes your constants if you get the
 * convention wrong (on the 360 compiler the default packing is not obvious),
 * and a transposed ortho puts the translation into .w -- every pixel clips
 * away and you get an all-black frame. Scalar registers are convention-free.
 * POSITIONT was NOT used: it is a fixed-function concept and is unreliable
 * with custom shaders on Xenos.
 */

#include "SpriteBatch.h"
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <stdio.h>      // _snprintf on the XDK CRT

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#include <d3d9.h>
#include <d3dx9.h>
#else
#error "Only Xbox 360 target supported"
#endif

namespace fnaf {

// ------------------------------------------------------------------
//  Embedded shaders (compiled at runtime, see file header)
// ------------------------------------------------------------------

static const char* kSpriteVS_HLSL =
    // c0 = (1/2W, -1/2H, 1, 0) scale, c1 = (-1, +1, 0, 0) offset.
    // Explicit scalar math: immune to matrix packing (column/row-major).
    "float4 kScale  : register(c0);\n"
    "float4 kOffset : register(c1);\n"
    "struct VS_IN  { float4 pos : POSITION; float4 color : COLOR0; float2 uv : TEXCOORD0; };\n"
    "struct VS_OUT { float4 pos : POSITION; float4 color : COLOR0; float2 uv : TEXCOORD0; };\n"
    "VS_OUT main(VS_IN i)\n"
    "{\n"
    "    VS_OUT o;\n"
    "    o.pos   = float4(i.pos.x * kScale.x + kOffset.x,\n"
    "                     i.pos.y * kScale.y + kOffset.y,\n"
    "                     i.pos.z * kScale.z + kOffset.z,\n"
    "                     1.0);\n"
    "    o.color = i.color;\n"
    "    o.uv    = i.uv;\n"
    "    return o;\n"
    "}\n";

static const char* kSpritePS_HLSL =
    "sampler2D tex0 : register(s0);\n"
    "struct PS_IN { float4 color : COLOR0; float2 uv : TEXCOORD0; };\n"
    "float4 main(PS_IN i) : COLOR0\n"
    "{\n"
    "    return tex2D(tex0, i.uv) * i.color;\n"
    "}\n";

// Screen-space -> clip space for a 1280x720 back buffer, y down:
//   clip.x = x/640 - 1,  clip.y = 1 - y/360,  clip.z = z,  clip.w = 1
// Uploaded as two scalar float4 registers -- no matrix packing ambiguity.
static const float kVSScaleConst[4]  = { 1.0f/640.0f, -1.0f/360.0f, 1.0f, 0.0f };
static const float kVSOffsetConst[4] = { -1.0f, 1.0f, 0.0f, 0.0f };

static const D3DVERTEXELEMENT9 kSpriteVertexDecl[] = {
    { 0, offsetof(SpriteVertex, x),     D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
    { 0, offsetof(SpriteVertex, color), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
    { 0, offsetof(SpriteVertex, u),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
    D3DDECL_END()
};

// ------------------------------------------------------------------
//  Runtime shader compilation helper
// ------------------------------------------------------------------

static bool CompileShaderFromMemory(D3DDeviceX* dev,
                                    const char* src,
                                    const char* profile,
                                    bool vertexStage,
                                    void** outShader,
                                    char* errOut, size_t errOutSize)
{
    *outShader = 0;
    if (errOutSize) errOut[0] = '\0';

    LPD3DXBUFFER codeBuf = 0;
    LPD3DXBUFFER errBuf  = 0;
    HRESULT hr = D3DXCompileShader(
        src, (UINT)strlen(src),
        NULL,           // defines
        NULL,           // include
        "main",
        profile,
        0,              // flags
        &codeBuf,
        &errBuf,
        NULL);          // constant table (registers are explicit in the HLSL)

    if (FAILED(hr) || !codeBuf) {
        const char* msg = errBuf ? (const char*)errBuf->GetBufferPointer() : "";
        _snprintf(errOut, errOutSize - 1,
                  "D3DXCompileShader(%s) failed hr=0x%08X: %.80s",
                  profile, (unsigned)hr, msg);
        errOut[errOutSize - 1] = '\0';
        if (errBuf) errBuf->Release();
        return false;
    }
    if (errBuf) errBuf->Release();

    HRESULT chr;
    if (vertexStage) {
        IDirect3DVertexShader9* vs = 0;
        chr = dev->CreateVertexShader((const DWORD*)codeBuf->GetBufferPointer(), &vs);
        if (SUCCEEDED(chr)) *outShader = vs;
    } else {
        IDirect3DPixelShader9* ps = 0;
        chr = dev->CreatePixelShader((const DWORD*)codeBuf->GetBufferPointer(), &ps);
        if (SUCCEEDED(chr)) *outShader = ps;
    }
    codeBuf->Release();

    if (FAILED(chr)) {
        _snprintf(errOut, errOutSize - 1,
                  "Create%sShader failed hr=0x%08X",
                  vertexStage ? "Vertex" : "Pixel", (unsigned)chr);
        errOut[errOutSize - 1] = '\0';
        return false;
    }
    return true;
}

// ------------------------------------------------------------------

SpriteBatch::SpriteBatch()
    : m_device(0)
    , m_vertices(0)
    , m_vertexCount(0)
    , m_currentTexture(0)
    , m_vertexShader(0)
    , m_pixelShader(0)
    , m_vertexDecl(0)
    , m_ready(false)
{
    m_initError[0] = '\0';
}

SpriteBatch::~SpriteBatch()
{
    Shutdown();
}

bool SpriteBatch::Init(void* device)
{
    m_ready = false;
    m_initError[0] = '\0';
    if (!device) return false;
    m_device = device;
    D3DDeviceX* dev = (D3DDeviceX*)device;

    if (!m_vertices) {
        m_vertices = new SpriteVertex[MAX_SPRITES_PER_BATCH * 4];
    }
    m_vertexCount = 0;
    m_currentTexture = 0;

    // --- Vertex + pixel shaders (runtime compile, see file header) ---
    char err[160];
    if (!CompileShaderFromMemory(dev, kSpriteVS_HLSL, "vs_3_0", true,
                                 &m_vertexShader, err, sizeof(err))) {
        _snprintf(m_initError, sizeof(m_initError) - 1, "VS: %s", err);
        m_initError[sizeof(m_initError) - 1] = '\0';
        return false;
    }
    if (!CompileShaderFromMemory(dev, kSpritePS_HLSL, "ps_3_0", false,
                                 &m_pixelShader, err, sizeof(err))) {
        _snprintf(m_initError, sizeof(m_initError) - 1, "PS: %s", err);
        m_initError[sizeof(m_initError) - 1] = '\0';
        return false;
    }

    // --- Vertex declaration ---
    IDirect3DVertexDeclaration9* decl = NULL;
    HRESULT hr = dev->CreateVertexDeclaration(kSpriteVertexDecl, &decl);
    if (FAILED(hr)) {
        _snprintf(m_initError, sizeof(m_initError) - 1,
                  "CreateVertexDeclaration failed hr=0x%08X", (unsigned)hr);
        m_initError[sizeof(m_initError) - 1] = '\0';
        return false;
    }
    m_vertexDecl = decl;

    m_ready = true;
    return true;
}

void SpriteBatch::Shutdown()
{
    if (m_vertexShader) { ((IDirect3DVertexShader9*)m_vertexShader)->Release(); m_vertexShader = 0; }
    if (m_pixelShader)  { ((IDirect3DPixelShader9*)m_pixelShader)->Release();   m_pixelShader = 0; }
    if (m_vertexDecl)   { ((IDirect3DVertexDeclaration9*)m_vertexDecl)->Release(); m_vertexDecl = 0; }
    if (m_vertices)     { delete[] (SpriteVertex*)m_vertices; m_vertices = 0; }
    m_device = 0;
    m_vertexCount = 0;
    m_currentTexture = 0;
    m_ready = false;
}

void SpriteBatch::Begin()
{
    m_vertexCount = 0;
    m_currentTexture = 0;
}

void SpriteBatch::Draw(void* tex, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1, u32 color)
{
    if (!m_ready) return;
    if (m_vertexCount + 4 > MAX_SPRITES_PER_BATCH * 4) {
        Flush();
    }
    if (tex != m_currentTexture && m_vertexCount > 0) {
        Flush();
    }
    m_currentTexture = tex;

    SpriteVertex* v = (SpriteVertex*)m_vertices + m_vertexCount;
    v[0].x = x;     v[0].y = y;     v[0].z = 0.0f; v[0].w = 1.0f; v[0].u = u0; v[0].v = v0; v[0].color = color;
    v[1].x = x+w;   v[1].y = y;     v[1].z = 0.0f; v[1].w = 1.0f; v[1].u = u1; v[1].v = v0; v[1].color = color;
    v[2].x = x;     v[2].y = y+h;   v[2].z = 0.0f; v[2].w = 1.0f; v[2].u = u0; v[2].v = v1; v[2].color = color;
    v[3].x = x+w;   v[3].y = y+h;   v[3].z = 0.0f; v[3].w = 1.0f; v[3].u = u1; v[3].v = v1; v[3].color = color;
    m_vertexCount += 4;
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
    dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    dev->SetTexture(0, (D3DTextureX*)m_currentTexture);

    // Programmable pipeline only: explicit shaders + declaration + scalars.
    dev->SetVertexDeclaration((IDirect3DVertexDeclaration9*)m_vertexDecl);
    dev->SetVertexShader((IDirect3DVertexShader9*)m_vertexShader);
    dev->SetPixelShader((IDirect3DPixelShader9*)m_pixelShader);
    dev->SetVertexShaderConstantF(0, kVSScaleConst, 1);
    dev->SetVertexShaderConstantF(1, kVSOffsetConst, 1);
}

void SpriteBatch::Flush()
{
    if (!m_ready || m_vertexCount == 0) return;

    SetupRenderState();

    D3DDeviceX* dev = (D3DDeviceX*)m_device;

    const int spriteCount = m_vertexCount / 4;
    int base = 0;
    for (int i = 0; i < spriteCount; ++i) {
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2,
                             (SpriteVertex*)m_vertices + base,
                             sizeof(SpriteVertex));
        base += 4;
    }

    m_vertexCount = 0;
}

} // namespace fnaf
