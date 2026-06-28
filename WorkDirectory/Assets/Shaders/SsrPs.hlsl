cbuffer CB : register(b0)
{
    float4x4 viewProj;
    float4x4 _unused;
    float4 lightPosRadius[3];
    float4 lightColor[3];
    float4 cameraPosSpecPower;
    float4 ssrParams;
    float  ambient;
    float3 pad;
};

Texture2D hdrColor : register(t0);
Texture2D gPosition: register(t1);
Texture2D gNormal  : register(t2);
Texture2D gAlbedo  : register(t3);
Texture2D gMaterial: register(t4);
SamplerState smp   : register(s0);

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 FresnelSchlick(float cosT, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosT), 5.0);
}

bool ProjectToUV(float3 worldPos, out float2 uv, out float clipZ)
{
    float4 clip = mul(float4(worldPos, 1.0), viewProj);
    if (clip.w <= 0.0) { uv = 0; clipZ = 0; return false; }
    clip.xyz /= clip.w;
    uv = clip.xy * float2(0.5, -0.5) + 0.5;
    clipZ = clip.z;
    return (uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1);
}

float4 main(I i) : SV_TARGET
{
    float3 lit = hdrColor.Sample(smp, i.uv).rgb;

    float4 P = gPosition.Sample(smp, i.uv);
    int dbg = (int)(cameraPosSpecPower.w + 0.5);
    bool enabled = ssrParams.x > 0.5;

    if (P.w < 0.5 || !enabled)
    {
        if (dbg == 7 || dbg == 8) return float4(0,0,0,1);
        return float4(lit, 1.0);
    }

    float3 worldPos = P.xyz;
    float3 N = normalize(gNormal.Sample(smp, i.uv).xyz);
    float3 mat = gMaterial.Sample(smp, i.uv).rgb;
    float metallic  = mat.r;
    float roughness = clamp(mat.g, 0.04, 1.0);

    float3 V = normalize(cameraPosSpecPower.xyz - worldPos);
    float3 R = reflect(-V, N);

    int   maxSteps = (int)ssrParams.z;
    float stepSize = ssrParams.y;
    float thickness = stepSize * 2.0;

    float3 rayPos = worldPos + N * stepSize * 0.5;
    bool   hit = false;
    float2 hitUV = i.uv;

    [loop]
    for (int s = 0; s < maxSteps; s++)
    {
        rayPos += R * stepSize;

        float2 uv; float clipZ;
        if (!ProjectToUV(rayPos, uv, clipZ)) break;

        float4 sceneP = gPosition.Sample(smp, uv);
        if (sceneP.w < 0.5) continue;

        float rayDist   = length(rayPos     - cameraPosSpecPower.xyz);
        float sceneDist = length(sceneP.xyz - cameraPosSpecPower.xyz);

        if (rayDist > sceneDist && rayDist - sceneDist < thickness)
        {
            hit = true; hitUV = uv; break;
        }
    }

    if (dbg == 7) return float4(hit ? 1.0.xxx : 0.0.xxx, 1.0);
    if (dbg == 8)
    {
        float3 c = hit ? hdrColor.Sample(smp, hitUV).rgb : 0.0.xxx;
        return float4(c, 1.0);
    }

    if (!hit) return float4(lit, 1.0);

    float3 F0 = lerp(float3(0.04,0.04,0.04), float3(1,1,1), metallic);
    float  NoV = max(dot(N, V), 1e-4);
    float3 F = FresnelSchlick(NoV, F0);

    float3 hitColor = hdrColor.Sample(smp, hitUV).rgb;
    float  smoothW = 1.0 - roughness;
    float3 reflection = hitColor * F * smoothW * ssrParams.w;

    return float4(lit + reflection, 1.0);
}
