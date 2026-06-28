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

float4 main(float3 p : POSITION) : SV_POSITION
{
    return mul(float4(p, 1), mvp);
}
