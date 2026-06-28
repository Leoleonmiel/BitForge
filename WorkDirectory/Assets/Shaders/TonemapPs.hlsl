cbuffer CB : register(b0)
{
    float4x4 unused0;
    float4x4 unused1;
    float4 lightPosRadius[3];
    float4 lightColor[3];
    float4 cameraPosSpecPower;
    float4 specColor;
    float  ambient;
    float3 pad;
};

Texture2D    hdrBuffer : register(t0);
SamplerState smp0      : register(s0);

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 ACES(float3 x)
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 AgxContrast(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
    return  15.5 * x4 * x2 - 40.14 * x4 * x + 31.96 * x4
          - 6.868 * x2 * x + 0.4298 * x2 + 0.1191 * x - 0.00232;
}

float3 AgX(float3 val)
{
    float3 r = float3(
        dot(val, float3(0.842479062253094,  0.0423282422610123, 0.0423756549057051)),
        dot(val, float3(0.0784335999999992, 0.878468636469772,  0.0784336000000000)),
        dot(val, float3(0.0792237451477643, 0.0791661274605434, 0.879142973793104)));

    const float minEv = -12.47393, maxEv = 4.026069;
    r = clamp(log2(max(r, 1e-10)), minEv, maxEv);
    r = (r - minEv) / (maxEv - minEv);
    r = saturate(AgxContrast(r));

    float3 o = float3(
        dot(r, float3( 1.19687900512017,  -0.0528968517574562, -0.0529716355144438)),
        dot(r, float3(-0.0980208811401368, 1.15190312990417,   -0.0980434501171241)),
        dot(r, float3(-0.0990297440797205,-0.0989611768448433,  1.15107367264116)));
    return max(o, 0.0);
}

float4 main(I i) : SV_TARGET
{
    float exposure = cameraPosSpecPower.x;
    int   op       = (int)(cameraPosSpecPower.y + 0.5);

    float3 hdr = hdrBuffer.Sample(smp0, i.uv).rgb;
    hdr *= exposure;

    float3 mapped;
    if (op == 1)      mapped = hdr / (hdr + 1.0);
    else if (op == 2) mapped = ACES(hdr);
    else if (op == 3) mapped = AgX(hdr);
    else              mapped = saturate(hdr);

    mapped = pow(mapped, 1.0 / 2.2);
    return float4(mapped, 1.0);
}
