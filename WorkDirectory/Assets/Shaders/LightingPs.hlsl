cbuffer CB : register(b0)
{
    float4x4 lightViewProj;
    float4x4 invViewProj;
    float4 sunDirParam;
    float4 _legacyPad[5];
    float4 cameraPosSpecPower;
    float4 specColor;
    float  iblEnabled;
    float  ssaoEnabled;
    float2 pad;
};

struct GPULight
{
    float3 position;  float radius;
    float3 color;     float intensity;
    float3 direction; int   type;
};

Texture2D gPosition : register(t0);
Texture2D gNormal   : register(t1);
Texture2D gAlbedo   : register(t2);
Texture2D gMaterial : register(t3);
Texture2D shadowMap : register(t4);
StructuredBuffer<GPULight> lights : register(t5);
Texture2D ssaoTex   : register(t6);

SamplerState           gSmp          : register(s0);
SamplerComparisonState shadowSampler : register(s1);

static const float PI = 3.14159265359;

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 EnvSky(float3 dir)
{
    float3 sunDir = normalize(sunDirParam.xyz);
    float  night = saturate(specColor.w);
    float  t = saturate(dir.y * 0.5 + 0.5);

    float3 horizon = lerp(float3(0.52,0.60,0.74), float3(0.04,0.05,0.10), night);
    float3 zenith  = lerp(float3(0.12,0.28,0.58), float3(0.01,0.02,0.06), night);
    float3 ground  = lerp(float3(0.21,0.18,0.15), float3(0.02,0.02,0.03), night);
    float3 sky = (dir.y >= 0.0) ? lerp(horizon, zenith, pow(t, 0.5))
                                : lerp(horizon, ground, saturate(-dir.y * 2.0));

    float sd   = max(dot(dir, sunDir), 0.0);
    float disc = pow(sd, 2200.0) * 70.0;
    float glow = pow(sd, 8.0) * 0.6;
    float3 sunCol  = float3(1.0, 0.96, 0.86);
    float3 moonCol = float3(0.55, 0.65, 0.90);
    sky += lerp(sunCol * (disc + glow), moonCol * (pow(sd,3000.0)*8.0), night);

    return sky * lerp(3.0, 0.6, night);
}

float3 EnvIrradiance(float3 N)
{
    float3 sunDir = normalize(sunDirParam.xyz);
    float  night = saturate(specColor.w);
    float  up = saturate(N.y * 0.5 + 0.5);
    float3 skyAmb = lerp(float3(0.34,0.46,0.68), float3(0.03,0.05,0.12), night);
    float3 gndAmb = lerp(float3(0.18,0.15,0.12), float3(0.02,0.02,0.04), night);
    float3 amb = lerp(gndAmb, skyAmb, up);
    float  sn = saturate(dot(N, sunDir) * 0.5 + 0.5);
    float3 bounce = lerp(float3(0.95,0.88,0.74), float3(0.30,0.40,0.65), night);
    amb += bounce * pow(sn, 1.5) * lerp(0.45, 0.15, night);
    return amb;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness*roughness, a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float d = (NdotH*NdotH) * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-6);
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N,V),0.0), roughness)
         * GeometrySchlickGGX(max(dot(N,L),0.0), roughness);
}
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 Fr = max((1.0 - roughness).xxx, F0);
    return F0 + (Fr - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}
float2 EnvBRDFApprox(float roughness, float NoV)
{
    const float4 c0 = float4(-1, -0.0275, -0.572, 0.022);
    const float4 c1 = float4( 1,  0.0425,  1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}

float SampleShadow(float3 worldPos)
{
    float4 lp = mul(float4(worldPos, 1.0), lightViewProj);
    lp.xyz /= lp.w;
    if (lp.x < -1 || lp.x > 1 || lp.y < -1 || lp.y > 1 || lp.z > 1) return 1.0;
    float2 uv = lp.xy * float2(0.5, -0.5) + 0.5;
    float depth = lp.z - 0.0025;
    uint w, h; shadowMap.GetDimensions(w, h);
    float2 texel = 1.0 / float2(w, h);
    float lit = 0.0;
    [unroll] for (int y=-1;y<=1;y++) [unroll] for (int x=-1;x<=1;x++)
        lit += shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(x,y)*texel, depth);
    return lit / 9.0;
}

float3 ViewRay(float2 uv)
{
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 wp = mul(float4(ndc, 1.0, 1.0), invViewProj);
    wp.xyz /= wp.w;
    return normalize(wp.xyz - cameraPosSpecPower.xyz);
}

float4 main(I i) : SV_TARGET
{
    float4 P   = gPosition.Sample(gSmp, i.uv);
    float3 N   = normalize(gNormal.Sample(gSmp, i.uv).xyz);
    float3 albedo = gAlbedo.Sample(gSmp, i.uv).rgb;
    float3 mat = gMaterial.Sample(gSmp, i.uv).rgb;
    float metallic  = mat.r;
    float roughness = clamp(mat.g, 0.04, 1.0);
    float ao        = mat.b;
    bool iblOn = iblEnabled > 0.5;

    float occlusion = (ssaoEnabled > 0.5) ? ssaoTex.Sample(gSmp, i.uv).r : 1.0;

    int mode = (int)(cameraPosSpecPower.w + 0.5);
    if (mode == 1) return float4(P.xyz * 0.5 + 0.5, 1);
    if (mode == 2) return float4(N * 0.5 + 0.5, 1);
    if (mode == 3) return float4(albedo, 1);
    if (mode == 4) return float4(metallic.xxx, 1);
    if (mode == 5) return float4(roughness.xxx, 1);
    if (mode == 6) return float4(EnvIrradiance(N), 1);
    if (mode == 9) return float4(occlusion.xxx, 1);
    if (mode == 10) return float4(SampleShadow(P.xyz).xxx, 1);

    if (P.w < 0.5)
        return float4(EnvSky(ViewRay(i.uv)), 1.0);

    float3 worldPos = P.xyz;
    float3 V = normalize(cameraPosSpecPower.xyz - worldPos);
    float  NoV = max(dot(N, V), 1e-4);
    bool shadowsOn  = specColor.x > 0.5;
    int   lightCount = (int)(specColor.y + 0.5);

    float3 F0 = lerp(float3(0.04,0.04,0.04), albedo, metallic);

    float3 Lo = 0.0;
    for (int li = 0; li < lightCount; li++)
    {
        GPULight lt = lights[li];
        float3 L; float3 radiance; float shadow = 1.0;
        if (lt.type == 0)
        {
            L = normalize(lt.direction);
            radiance = lt.color * lt.intensity;
            if (shadowsOn) shadow = SampleShadow(worldPos);
        }
        else
        {
            float3 toL = lt.position - worldPos;
            float dist = length(toL);
            if (dist > lt.radius) continue;
            L = toL / max(dist, 1e-4);
            float atten = saturate(1.0 - dist / lt.radius); atten *= atten;
            if (lt.type == 2)
                atten *= smoothstep(0.8, 0.95, dot(normalize(-toL), normalize(lt.direction)));
            radiance = lt.color * lt.intensity * atten;
        }
        float3 H = normalize(V + L);
        float  NdotL = max(dot(N, L), 0.0);
        float  NDF = DistributionGGX(N, H, roughness);
        float  G   = GeometrySmith(N, V, L, roughness);
        float3 F   = FresnelSchlick(max(dot(H,V),0.0), F0);
        float3 spec = (NDF * G * F) / (4.0 * NoV * NdotL + 0.001);
        float3 kD = (1.0 - F) * (1.0 - metallic);
        Lo += (kD * albedo / PI + spec) * radiance * NdotL * shadow;
    }

    float3 ambient;
    if (iblOn)
    {
        float iblScale = specColor.z;

        float3 kS = FresnelSchlickRoughness(NoV, F0, roughness);
        float3 kD = (1.0 - kS) * (1.0 - metallic);
        float3 diffuseIBL = kD * EnvIrradiance(N) * albedo;

        float3 R = reflect(-V, N);
        float3 prefiltered = lerp(EnvSky(R), EnvIrradiance(R), roughness);
        float2 brdf = EnvBRDFApprox(roughness, NoV);
        float3 specularIBL = prefiltered * (F0 * brdf.x + brdf.y);

        ambient = (diffuseIBL + specularIBL) * ao * iblScale;
    }
    else
    {
        float hemi = N.y * 0.5 + 0.5;
        ambient = lerp(float3(0.20,0.18,0.15), float3(0.35,0.42,0.55), hemi) * albedo * ao;
    }

    ambient *= occlusion;

    return float4(ambient + Lo, 1.0);
}
