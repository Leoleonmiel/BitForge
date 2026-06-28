cbuffer CB : register(b0)
{
    float4x4 viewProj;
};

cbuffer InstanceConst : register(b1)
{
    uint gInstanceIndex;
};

struct InstanceData
{
    float4x4 world;
    uint  texIndex;
    float metallic;
    float roughness;
    float ao;
};
StructuredBuffer<InstanceData> instances : register(t0);

struct VSIn  { float3 p : POSITION; float3 n : NORMAL; float3 c : COLOR; float2 uv : TEXCOORD; };
struct VSOut
{
    float4 pos : SV_POSITION;
    float3 wp  : TEXCOORD0;
    float3 n   : TEXCOORD1;
    float3 c   : COLOR;
    float2 uv  : TEXCOORD2;
    nointerpolation uint  texIndex : TEXCOORD3;
    nointerpolation float3 mat      : TEXCOORD4;
};

VSOut main(VSIn i)
{
    InstanceData inst = instances[gInstanceIndex];

    float4 worldPos = mul(float4(i.p, 1.0), inst.world);

    VSOut o;
    o.wp  = worldPos.xyz;
    o.n   = mul(i.n, (float3x3)inst.world);
    o.pos = mul(worldPos, viewProj);
    o.c   = i.c;
    o.uv  = i.uv;
    o.texIndex = inst.texIndex;
    o.mat = float3(inst.metallic, inst.roughness, inst.ao);
    return o;
}
