#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "../Graph/RenderPass.h"

using namespace DirectX;

void Dx12Renderer::PassVolumetricFog(const RenderContext& ctx)
{
    if (!m_fog.enabled) { return; }

    auto* cl = ctx.cmdList;

    HdrTarget& scene = m_camera.dofEnabled ? m_hdr : m_ssr;
    scene.ToShaderResource(cl);
    m_shadowMap.ToShaderResource(cl);
    m_fogTarget.ToRenderTarget(cl);
    auto rtv = m_fogTarget.Rtv();
    cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(fogRootSig.Get());

    const float scale = (m_sceneRadius > 1.0f) ? m_sceneRadius : 50.0f;
    const float maxDistWorld = m_fog.maxDistance * scale * 0.02f;
    const float densityWorld = m_fog.density / (scale * 0.03f);
    const float heightFWorld = m_fog.heightFactor / (scale * 0.50f);
    const float baseY        = m_sceneCenter.y - scale * 0.50f;

    XMFLOAT3 sunDir{ 0.5f, 1.0f, 0.4f };
    XMFLOAT3 sunCol{ 1.0f, 0.95f, 0.9f };
    float    sunInt = 3.0f;
    for (const Light& lt : m_lightManager.GetLights())
    {
        if (lt.type == (int)LightType::Directional)
        {
            sunDir = lt.direction;
            sunCol = lt.color;
            sunInt = lt.intensity;
            break;
        }
    }

    CBData cb{};
    XMMATRIX lvp   = XMMatrixTranspose(m_lightViewProj);
    XMMATRIX invVP = XMMatrixTranspose(XMMatrixInverse(nullptr, ctx.viewProj));
    memcpy(cb.mvp,   &lvp,   sizeof(cb.mvp));
    memcpy(cb.world, &invVP, sizeof(cb.world));

    cb.lightPosRadius[0][0] = sunDir.x;
    cb.lightPosRadius[0][1] = sunDir.y;
    cb.lightPosRadius[0][2] = sunDir.z;
    cb.lightPosRadius[1][0] = maxDistWorld;
    cb.lightPosRadius[1][1] = baseY;
    cb.lightPosRadius[1][2] = 1.0f;
    cb.lightPosRadius[1][3] = m_fog.ambientIntensity;

    cb.lightColor[0][0] = sunCol.x * sunInt;
    cb.lightColor[0][1] = sunCol.y * sunInt;
    cb.lightColor[0][2] = sunCol.z * sunInt;
    cb.lightColor[1][0] = m_fog.fogColor.x;
    cb.lightColor[1][1] = m_fog.fogColor.y;
    cb.lightColor[1][2] = m_fog.fogColor.z;

    cb.cameraPosSpecPower[0] = ctx.cameraPos.x;
    cb.cameraPosSpecPower[1] = ctx.cameraPos.y;
    cb.cameraPosSpecPower[2] = ctx.cameraPos.z;
    cb.cameraPosSpecPower[3] = m_shadowsEnabled ? 1.0f : 0.0f;

    cb.specColor[0] = densityWorld;
    cb.specColor[1] = heightFWorld;
    cb.specColor[2] = m_fog.anisotropy;
    cb.specColor[3] = float(m_fog.stepCount);
    PushConstants(cb);

    cl->SetGraphicsRootDescriptorTable(1, scene.Srv());
    cl->SetGraphicsRootDescriptorTable(2, m_gbuffer.SrvTableStart());

    cl->SetPipelineState(fogPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);
}
