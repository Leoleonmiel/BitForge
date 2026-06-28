cbuffer CB : register(b0)
{
    float4x4 lightViewProj;
    float4x4 invViewProj;
    float4 sunDir;
    float4 fogDistParams;
    float4 _lpr2;
    float4 sunColor;
    float4 ambientFogColor;
    float4 _lc2;
    float4 cameraPos;
    float4 fogParams;
    float  ambient;
    float3 pad;
};

Texture2D sceneColor : register(t0);
Texture2D gPosition  : register(t1);
Texture2D shadowMap  : register(t2);

SamplerState           gSmp          : register(s0);
SamplerComparisonState shadowSampler : register(s1);

static const float PI = 3.14159265359;

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 ViewRay(float2 uv)
{
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 wp = mul(float4(ndc, 1.0, 1.0), invViewProj);
    wp.xyz /= wp.w;
    return normalize(wp.xyz - cameraPos.xyz);
}

float SampleShadow(float3 worldPos)
{
    float4 lp = mul(float4(worldPos, 1.0), lightViewProj);
    lp.xyz /= lp.w;
    if (lp.x < -1 || lp.x > 1 || lp.y < -1 || lp.y > 1 || lp.z > 1) return 1.0;
    float2 uv = lp.xy * float2(0.5, -0.5) + 0.5;
    return shadowMap.SampleCmpLevelZero(shadowSampler, uv, lp.z - 0.0025);
}

float PhaseHG(float cosT, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(max(1.0 + g2 - 2.0 * g * cosT, 1e-4), 1.5));
}

float IGN(float2 p)
{
    return frac(52.9829189 * frac(dot(p, float2(0.06711056, 0.00583715))));
}

float4 main(I i) : SV_TARGET
{
    float3 scene = sceneColor.Sample(gSmp, i.uv).rgb;

    float3 ro = cameraPos.xyz;
    float3 rd = ViewRay(i.uv);

    float4 P = gPosition.Sample(gSmp, i.uv);
    float  maxDist = fogDistParams.x;
    float  surface = (P.w > 0.5) ? length(P.xyz - ro) : maxDist;
    float  marchDist = min(surface, maxDist);

    int   steps    = max((int)fogParams.w, 1);
    float stepSize = marchDist / steps;
    float density0 = fogParams.x;
    float heightF  = fogParams.y;
    float baseY    = fogDistParams.y;
    float g        = fogParams.z;
    float extinct  = fogDistParams.z;
    bool  shadowsOn = cameraPos.w > 0.5;

    float cosT  = dot(rd, normalize(sunDir.xyz));
    float phase = PhaseHG(cosT, g);
    float3 ambientTerm = ambientFogColor.rgb * fogDistParams.w;

    float jitter = IGN(i.pos.xy);

    float3 fog = 0.0;
    float  T   = 1.0;
    [loop] for (int s = 0; s < steps; s++)
    {
        float  t  = (s + jitter) * stepSize;
        float3 sp = ro + rd * t;

        float h = max(sp.y - baseY, 0.0);
        float density = density0 * exp(-h * heightF);
        if (density > 1e-6)
        {
            float shadow = shadowsOn ? SampleShadow(sp) : 1.0;
            float3 inScatter = (sunColor.rgb * phase * shadow + ambientTerm) * density;

            fog += T * inScatter * stepSize;
            T   *= exp(-density * extinct * stepSize);
            if (T < 0.02) break;
        }
    }

    return float4(scene * T + fog, 1.0);
}
