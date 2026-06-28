cbuffer CB : register(b0)
{
    float4x4 mvp;
    float4x4 world;
    float4 lightPosRadius[3];
    float4 lightColor[3];
    float4 cameraPosSpecPower;
    float4 specColor;
    float  ambient;
    float3 pad;
};

struct V { float3 p:POSITION; float3 n:NORMAL; float3 c:COLOR; float2 uv:TEXCOORD; };
struct O
{
    float4 p  : SV_POSITION;
    float3 wp : TEXCOORD0;
    float3 n  : TEXCOORD1;
    float3 c  : COLOR;
    float2 uv : TEXCOORD2;
};

O main(V i)
{
    O o;
    float4 wp = mul(float4(i.p, 1), world);
    o.wp = wp.xyz;
    o.n  = mul(i.n, (float3x3)world);
    o.p  = mul(float4(i.p, 1), mvp);
    o.c  = i.c;
    o.uv = i.uv;
    return o;
}
