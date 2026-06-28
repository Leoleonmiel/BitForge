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

Texture2D    tex0 : register(t0);
SamplerState smp0 : register(s0);

struct I
{
    float4 p  : SV_POSITION;
    float3 wp : TEXCOORD0;
    float3 n  : TEXCOORD1;
    float3 c  : COLOR;
    float2 uv : TEXCOORD2;
};

struct PSOutput
{
    float4 position : SV_Target0;
    float4 normal   : SV_Target1;
    float4 albedo   : SV_Target2;
    float4 material : SV_Target3;
};

PSOutput main(I i)
{
    PSOutput o;
    o.position = float4(i.wp, 1.0);
    o.normal   = float4(normalize(i.n), 0.0);

    float3 tex = tex0.Sample(smp0, i.uv).rgb;
    o.albedo   = float4(tex * i.c, 1.0);

    o.material = float4(specColor.r, specColor.g, specColor.b, 1.0);
    return o;
}
