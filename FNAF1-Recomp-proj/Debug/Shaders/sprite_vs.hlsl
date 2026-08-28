// sprite_vs.hlsl -- Xbox 360 pass-through vertex shader for 2D sprites.
// Compiled OFFLINE at build time (see Shaders/README.md), not at runtime.
//
// Vertices are pre-transformed (POSITIONT: screen-space x,y,z,rhw), so this
// shader does no real transform work -- it just forwards position/color/uv.
// Matches the D3DVERTEXELEMENT9 layout in SpriteBatch.cpp (kSpriteVertexDecl).

struct VS_INPUT {
    float4 pos   : POSITIONT;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT IN)
{
    VS_OUTPUT OUT;
    OUT.pos   = IN.pos;
    OUT.color = IN.color;
    OUT.uv    = IN.uv;
    return OUT;
}
