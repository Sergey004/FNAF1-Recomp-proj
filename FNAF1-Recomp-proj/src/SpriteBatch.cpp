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

// v2.8 clean-room Panorama pixel shader. Re-implements the HWA version of
// Andos' Perspective extension (RPanorama.fx) from its documented math:
// for the HORIZONTAL direction, each output column reads a vertically
// squeezed band of the source, following a parabola. The serialized object
// is 1324x754 at (-22,-22) over a 1280x720 window; the vertex UVs are the
// object-normalized coords, and this shader maps them back into the
// 1280x720 window/capture space before sampling.
//
//   fB   = 1.0 - zoom / 754              (edge vertical scale; 1/754 = pixel height)
//   fC   = max(0.02, 1.0 + (fB-1)*curve*(u-0.5)^2)
//   dst  -> src:  src.xy = (u, (v - pivot) * fC + pivot)
//   object -> window:  win = (src*obj + origin) / windowSize
//
// Defaults (the serialized EDATA): zoom=300, pivot=0.5 (centerY=355),
// curve=4.0 -> fC(edge)=0.621. 'curve' replaces the literal 4.0 so the
// perspective tuner can flatten (0) or sharpen (>4) the bend live.
static const char* kPanoramaPS_HLSL =
    "sampler2D tex0 : register(s0);\n"
    "struct PS_IN { float4 color : COLOR0; float2 uv : TEXCOORD0; };\n"
    "float4 gParams : register(c0);\n"   // x = zoom, y = pivot (obj-normalized), z = curve
    "float4 main(PS_IN i) : COLOR0\n"
    "{\n"
    "    float zoom = gParams.x;\n"
    "    float vc   = gParams.y;\n"
    "    float kc   = gParams.z;\n"
    "    float fB = 1.0 - zoom / 754.0;\n"
    "    float a  = i.uv.x - 0.5;\n"
    "    float fC = max(0.02, 1.0 + (fB - 1.0) * kc * a * a);\n"
    "    float2 src = float2(i.uv.x, (i.uv.y - vc) * fC + vc);\n"
    "    float2 win = float2((src.x * 1324.0 - 22.0) / 1280.0,\n"
    "                        (src.y *  754.0 - 22.0) /  720.0);\n"
    "    return tex2D(tex0, win) * i.color;\n"
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
    , m_panRT(0)
    , m_panTex(0)
    , m_panPS(0)
    , m_backRT(0)
    , m_panReady(false)
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

    // --- v2.8 Panorama pixel shader (parabola, RPanorama.fx math) ---
    if (!CompileShaderFromMemory(dev, kPanoramaPS_HLSL, "ps_3_0", false,
                                 &m_panPS, err, sizeof(err))) {
        _snprintf(m_initError, sizeof(m_initError) - 1, "Panorama PS: %s", err);
        m_initError[sizeof(m_initError) - 1] = '\0';
        return false;
    }

    // --- v2.8 capture render target + resolve texture ---
    // The capture lives in EDRAM (render targets are EDRAM-only on 360);
    // Resolve() copies it into m_panTex so it can be sampled as a texture.
    // The device's auto depth-stencil is disabled in main.cpp: back buffer
    // (3.7MB) + capture target (3.7MB) already nearly fill the 10MB EDRAM.
    {
        IDirect3DSurface9* rt  = 0;
        IDirect3DTexture9* tex = 0;
        HRESULT hrt = dev->CreateRenderTarget(1280, 720, D3DFMT_A8R8G8B8,
                                              D3DMULTISAMPLE_NONE, 0, FALSE,
                                              &rt, NULL);
        HRESULT htx = dev->CreateTexture(1280, 720, 1, 0, D3DFMT_A8R8G8B8,
                                         (D3DPOOL)0, &tex, NULL);
        if (FAILED(hrt) || !rt || FAILED(htx) || !tex) {
            _snprintf(m_initError, sizeof(m_initError) - 1,
                      "Perspective capture RT failed (rt=0x%08X tex=0x%08X)",
                      (unsigned)hrt, (unsigned)htx);
            m_initError[sizeof(m_initError) - 1] = '\0';
            if (rt)  rt->Release();
            if (tex) tex->Release();
            return false;
        }
        m_panRT  = rt;
        m_panTex = tex;
    }

    m_panReady = true;
    m_ready = true;
    return true;
}

void SpriteBatch::Shutdown()
{
    if (m_vertexShader) { ((IDirect3DVertexShader9*)m_vertexShader)->Release(); m_vertexShader = 0; }
    if (m_pixelShader)  { ((IDirect3DPixelShader9*)m_pixelShader)->Release();   m_pixelShader = 0; }
    if (m_panPS)        { ((IDirect3DPixelShader9*)m_panPS)->Release();         m_panPS = 0; }
    if (m_vertexDecl)   { ((IDirect3DVertexDeclaration9*)m_vertexDecl)->Release(); m_vertexDecl = 0; }
    if (m_panRT)        { ((IDirect3DSurface9*)m_panRT)->Release();  m_panRT = 0; }
    if (m_panTex)       { ((IDirect3DTexture9*)m_panTex)->Release(); m_panTex = 0; }
    if (m_backRT)       { ((IDirect3DSurface9*)m_backRT)->Release(); m_backRT = 0; }
    if (m_vertices)     { delete[] (SpriteVertex*)m_vertices; m_vertices = 0; }
    m_device = 0;
    m_vertexCount = 0;
    m_currentTexture = 0;
    m_panReady = false;
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

// General-purpose full-mesh primitive: one DrawPrimitiveUP for an explicit
// triangle list in screen space. Same shaders / declaration / blend state as
// the quad path (SetupRenderState). No longer used by the Perspective path
// (v2.8 replaced the per-sprite bent mesh with a render-target capture +
// parabola shader), but kept as a batch primitive.
void SpriteBatch::DrawTriangles(void* tex, const SpriteVertex* verts, int vertexCount)
{
    if (!m_ready || !verts || vertexCount < 3) return;
    if (m_vertexCount > 0) {
        Flush();
    }
    m_currentTexture = tex;
    SetupRenderState();

    D3DDeviceX* dev = (D3DDeviceX*)m_device;
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, vertexCount / 3,
                         (const void*)verts, sizeof(SpriteVertex));
}

// v2.8 -- Perspective post-process capture. Called from GameRender::Render*
// right before drawing the flat layer-0 scene: saves the back-buffer render
// target, switches to the 1280x720 EDRAM capture, clears it, and lets the
// following Draw*() calls land in the capture instead of the screen.
void SpriteBatch::BeginSceneCapture(u32 clearColor)
{
    if (!m_ready || !m_panReady) return;
    if (m_vertexCount > 0) Flush();
    D3DDeviceX* dev = (D3DDeviceX*)m_device;
    dev->GetRenderTarget(0, (D3DSurface**)&m_backRT);
    dev->SetRenderTarget(0, (D3DSurface*)m_panRT);
    dev->Clear(0, NULL, D3DCLEAR_TARGET, clearColor, 1.0f, 0);
}

// Flushes the queued layer-0 scene into the capture target, resolves the
// EDRAM capture into the sampleable m_panTex, then restores the back buffer.
void SpriteBatch::EndSceneCapture()
{
    if (!m_ready || !m_panReady) return;
    if (m_vertexCount > 0) Flush();
    D3DDeviceX* dev = (D3DDeviceX*)m_device;
    dev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, (D3DBaseTexture*)m_panTex,
                 NULL, 0, 0, NULL, 0.0f, 0, 0, NULL);
    dev->SetRenderTarget(0, (D3DSurface*)m_backRT);
    if (m_backRT) {
        ((D3DSurface*)m_backRT)->Release();
        m_backRT = 0;
    }
}

// Draws the capture through the panorama parabola shader, full screen.
// zoom/centerY/curve mirror the serialized EDATA (300 / 355 / 4.0) and are
// the live Perspective-tuner knobs from GameRender.cpp.
void SpriteBatch::DrawPerspective(float zoom, float centerY, float curve)
{
    if (!m_ready || !m_panReady || !m_panTex || !m_panPS) return;
    if (m_vertexCount > 0) Flush();
    D3DDeviceX* dev = (D3DDeviceX*)m_device;

    // Same state as SetupRenderState, except the panorama PS + capture.
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
    dev->SetTexture(0, (D3DTextureX*)m_panTex);
    dev->SetVertexDeclaration((IDirect3DVertexDeclaration9*)m_vertexDecl);
    dev->SetVertexShader((IDirect3DVertexShader9*)m_vertexShader);
    dev->SetPixelShader((IDirect3DPixelShader9*)m_panPS);
    dev->SetVertexShaderConstantF(0, kVSScaleConst, 1);
    dev->SetVertexShaderConstantF(1, kVSOffsetConst, 1);

    float p[4];
    p[0] = zoom;
    p[1] = (centerY + 22.0f) / 754.0f;   // object-normalized vertical pivot
    p[2] = curve;
    p[3] = 0.0f;
    dev->SetPixelShaderConstantF(0, p, 1);

    // Full-screen quad whose UVs are the object-normalized coords of the
    // 1324x754 object at (-22,-22): the visible 1280x720 window sits in the
    // interior, so the parabola is sampled only over its real sub-range.
    SpriteVertex q[4];
    const float u0 = 22.0f  / 1324.0f;
    const float v0 = 22.0f  / 754.0f;
    const float u1 = 1302.0f / 1324.0f;
    const float v1 = 742.0f  / 754.0f;
    const u32 white = 0xFFFFFFFFu;

    q[0].x = 0.0f;    q[0].y = 0.0f;    q[0].z = 0.0f; q[0].w = 1.0f; q[0].u = u0; q[0].v = v0; q[0].color = white;
    q[1].x = 1280.0f; q[1].y = 0.0f;    q[1].z = 0.0f; q[1].w = 1.0f; q[1].u = u1; q[1].v = v0; q[1].color = white;
    q[2].x = 0.0f;    q[2].y = 720.0f;  q[2].z = 0.0f; q[2].w = 1.0f; q[2].u = u0; q[2].v = v1; q[2].color = white;
    q[3].x = 1280.0f; q[3].y = 720.0f;  q[3].z = 0.0f; q[3].w = 1.0f; q[3].u = u1; q[3].v = v1; q[3].color = white;

    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(SpriteVertex));
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
