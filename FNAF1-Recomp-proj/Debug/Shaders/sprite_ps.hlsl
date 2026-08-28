// sprite_ps.hlsl -- Xbox 360 pixel shader for 2D sprites.
// Compiled OFFLINE at build time (see Shaders/README.md), not at runtime.
// Samples the bound texture and modulates by the vertex color (matches the
// old fixed-function D3DTOP_MODULATE behavior).

sampler2D tex0 : register(s0);

struct PS_INPUT {
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

float4 main(PS_INPUT IN) : COLOR0
{
    return tex2D(tex0, IN.uv) * IN.color;
}
