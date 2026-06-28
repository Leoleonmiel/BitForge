cbuffer CB : register(b0)
{
    float4x4 _0;
    float4x4 _1;
    float4 lightPosRadius[3];
    float4 lightColor[3];
    float4 cameraPosAF;
    float4 dofParams;
    float  ambient;
    float3 pad;
};

Texture2D    hdrColor  : register(t0);
Texture2D    gPosition : register(t1);
SamplerState smp       : register(s0);

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float SceneDist(float2 uv, float3 camPos)
{
    float4 P = gPosition.Sample(smp, uv);
    return (P.w > 0.5) ? length(P.xyz - camPos) : -1.0;
}

float4 main(I i) : SV_TARGET
{
    float3 camPos = cameraPosAF.xyz;
    float3 color  = hdrColor.Sample(smp, i.uv).rgb;

    float focus = dofParams.x;
    if (cameraPosAF.w > 0.5)
    {
        float c = SceneDist(float2(0.5, 0.5), camPos);
        if (c > 0.0) focus = c;
    }

    float d     = SceneDist(i.uv, camPos);
    float dist  = (d > 0.0) ? d : focus * 3.0;
    float range = max(focus * dofParams.y * 0.08, 0.001);
    float coc   = saturate(abs(dist - focus) / range) * dofParams.z;

    const float maxBlur = 14.0;
    float radius = coc * maxBlur;
    if (radius < 0.5) return float4(color, 1.0);

    uint w, h; hdrColor.GetDimensions(w, h);
    float2 texel = 1.0 / float2(w, h);

    float3 sum = color; float wsum = 1.0;
    [unroll] for (int k = 0; k < 16; k++)
    {
        float ang = k * (6.2831853 / 16.0);
        float2 dir = float2(cos(ang), sin(ang)) * texel * radius;
        sum += hdrColor.Sample(smp, i.uv + dir).rgb;        wsum += 1.0;
        sum += hdrColor.Sample(smp, i.uv + dir * 0.5).rgb;  wsum += 1.0;
    }
    return float4(sum / wsum, 1.0);
}
