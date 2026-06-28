Texture2D    gTextures[1024] : register(t1);
SamplerState gSmp        : register(s0);

struct PSIn
{
    float4 pos : SV_POSITION;
    float3 wp  : TEXCOORD0;
    float3 n   : TEXCOORD1;
    float3 c   : COLOR;
    float2 uv  : TEXCOORD2;
    nointerpolation uint  texIndex : TEXCOORD3;
    nointerpolation float3 mat      : TEXCOORD4;
};

struct PSOut
{
    float4 position : SV_Target0;
    float4 normal   : SV_Target1;
    float4 albedo   : SV_Target2;
    float4 material : SV_Target3;
};

PSOut main(PSIn i)
{
    PSOut o;
    o.position = float4(i.wp, 1.0);
    o.normal   = float4(normalize(i.n), 0.0);
    float3 tex = gTextures[NonUniformResourceIndex(i.texIndex)].Sample(gSmp, i.uv).rgb;
    o.albedo   = float4(tex * i.c, 1.0);
    o.material = float4(i.mat, 1.0);
    return o;
}
