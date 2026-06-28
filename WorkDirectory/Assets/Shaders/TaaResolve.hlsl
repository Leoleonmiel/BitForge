cbuffer CB : register(b0)
{
    float4x4 prevViewProj;
    float4x4 invCurViewProj;
    float4 _lpr[3];
    float4 _lc[3];
    float4 cameraPos;
    float4 taaParams;
    float  ambient;
    float3 pad;
};

Texture2D    currentColor : register(t0);
Texture2D    historyColor : register(t1);
Texture2D    gPosition    : register(t2);
SamplerState linClamp     : register(s0);

struct I { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 ViewRay(float2 uv)
{
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 wp = mul(float4(ndc, 1.0, 1.0), invCurViewProj);
    wp.xyz /= wp.w;
    return normalize(wp.xyz - cameraPos.xyz);
}

bool Reproject(float3 wp, out float2 uv)
{
    float4 c = mul(float4(wp, 1.0), prevViewProj);
    if (c.w <= 0.0) { uv = 0.0; return false; }
    c.xyz /= c.w;
    uv = c.xy * float2(0.5, -0.5) + 0.5;
    return (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0);
}

float4 main(I i) : SV_TARGET
{
    float3 cur = currentColor.Sample(linClamp, i.uv).rgb;
    int    dbg = (int)(cameraPos.w + 0.5);

    if (taaParams.y < 0.5)
        return float4((dbg == 2) ? 0.0.xxx : cur, 1.0);

    float4 P  = gPosition.Sample(linClamp, i.uv);
    float3 wp = (P.w > 0.5) ? P.xyz : (cameraPos.xyz + ViewRay(i.uv) * 1e7);
    float2 prevUV; bool ok = Reproject(wp, prevUV);

    if (dbg == 1)
        return float4(saturate(abs(i.uv - prevUV) * 40.0), 0.0, 1.0);

    uint w, h; currentColor.GetDimensions(w, h);
    float2 texel = 1.0 / float2(w, h);
    float3 mn = cur, mx = cur;
    [unroll] for (int y = -1; y <= 1; y++)
    [unroll] for (int x = -1; x <= 1; x++)
    {
        float3 c = currentColor.Sample(linClamp, i.uv + float2(x, y) * texel).rgb;
        mn = min(mn, c); mx = max(mx, c);
    }

    if (!ok) return float4(cur, 1.0);

    float3 hist = historyColor.Sample(linClamp, prevUV).rgb;
    hist = clamp(hist, mn, mx);

    if (dbg == 2) return float4(hist, 1.0);

    float3 res = lerp(hist, cur, taaParams.x);
    return float4(res, 1.0);
}
