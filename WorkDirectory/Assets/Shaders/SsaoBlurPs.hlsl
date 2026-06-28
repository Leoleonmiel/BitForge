cbuffer CB : register(b0)
{
    float4x4 _0;
    float4x4 _1;
    float4 lightPosRadius[3];
    float4 lightColor[3];
    float4 cameraPos;
    float4 blurParams;
    float  ambient;
    float3 pad;
};

Texture2D    ssaoTex   : register(t0);
Texture2D    gPosition : register(t1);
SamplerState gSmp      : register(s0);

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float main(I i) : SV_TARGET
{
    uint w, h; ssaoTex.GetDimensions(w, h);
    float2 texel = 1.0 / float2(w, h);

    float4 Pc = gPosition.Sample(gSmp, i.uv);
    float  cDist = length(Pc.xyz - cameraPos.xyz);
    float  edge  = max(blurParams.x, 1e-4);

    float sum = 0.0, wsum = 0.0;
    [unroll] for (int y = -2; y <= 1; y++)
    [unroll] for (int x = -2; x <= 1; x++)
    {
        float2 uv = i.uv + float2(x, y) * texel;
        float  a  = ssaoTex.Sample(gSmp, uv).r;
        float4 Ps = gPosition.Sample(gSmp, uv);
        float sDist = length(Ps.xyz - cameraPos.xyz);
        float wgt = (Ps.w > 0.5 && abs(sDist - cDist) < edge) ? 1.0 : 0.0;
        sum += a * wgt; wsum += wgt;
    }
    return (wsum > 0.0) ? sum / wsum : ssaoTex.Sample(gSmp, i.uv).r;
}
