cbuffer CB : register(b0)
{
    float4x4 viewProj;
    float4x4 _unused;
    float4 lightPosRadius[3];
    float4 lightColor[3];
    float4 cameraPos;
    float4 ssaoParams;
    float  ambient;
    float3 pad;
};

cbuffer Kernel : register(b1)
{
    float4 kernel[64];
};

Texture2D    gPosition : register(t0);
Texture2D    gNormal   : register(t1);
Texture2D    noiseTex  : register(t2);
SamplerState gSmp      : register(s0);
SamplerState noiseSmp  : register(s1);

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float main(I i) : SV_TARGET
{
    float4 P4 = gPosition.Sample(gSmp, i.uv);
    if (P4.w < 0.5) return 1.0;
    float3 P = P4.xyz;
    float3 N = normalize(gNormal.Sample(gSmp, i.uv).xyz);

    uint w, h; gPosition.GetDimensions(w, h);
    float2 noiseScale = float2(w, h) / 4.0;
    float3 rnd = noiseTex.Sample(noiseSmp, i.uv * noiseScale).xyz * 2.0 - 1.0;
    float3 T = normalize(rnd - N * dot(rnd, N));
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    float radius    = ssaoParams.x;
    float bias      = ssaoParams.y;
    float intensity = ssaoParams.z;
    int   ksize     = (int)ssaoParams.w;

    float centerDist = length(P - cameraPos.xyz);

    float occ = 0.0;
    for (int s = 0; s < ksize; s++)
    {
        float3 dir = mul(kernel[s].xyz, TBN);
        float3 samplePos = P + dir * radius;

        float4 clip = mul(float4(samplePos, 1.0), viewProj);
        if (clip.w <= 0.0) continue;
        float2 suv = (clip.xy / clip.w) * float2(0.5, -0.5) + 0.5;
        if (suv.x < 0 || suv.x > 1 || suv.y < 0 || suv.y > 1) continue;

        float4 sceneP = gPosition.Sample(gSmp, suv);
        if (sceneP.w < 0.5) continue;

        float sceneDist  = length(sceneP.xyz - cameraPos.xyz);
        float sampleDist = length(samplePos   - cameraPos.xyz);

        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(centerDist - sceneDist), 1e-4));
        occ += ((sceneDist <= sampleDist - bias) ? 1.0 : 0.0) * rangeCheck;
    }

    occ = occ / max((float)ksize, 1.0);
    return saturate(1.0 - occ * intensity);
}
