#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "../Graph/RenderPass.h"

using namespace DirectX;

void Dx12Renderer::PassSsaoBlur(const RenderContext& ctx)
{
    if (!m_ssao.enabled || !m_ssao.blur) { return; }

    auto* cl = ctx.cmdList;

    m_ssaoRaw.ToShaderResource(cl);
    m_ssaoBlur.ToRenderTarget(cl);
    auto rtv = m_ssaoBlur.Rtv();
    cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(ssaoBlurRootSig.Get());

    const float scale = (m_sceneRadius > 1.0f ? m_sceneRadius : 50.0f) * 0.02f;
    CBData cb{};
    cb.cameraPosSpecPower[0] = ctx.cameraPos.x;
    cb.cameraPosSpecPower[1] = ctx.cameraPos.y;
    cb.cameraPosSpecPower[2] = ctx.cameraPos.z;
    cb.specColor[0] = scale * 2.0f;
    PushConstants(cb);

    cl->SetGraphicsRootDescriptorTable(1, m_ssaoRaw.Srv());
    cl->SetGraphicsRootDescriptorTable(2, m_gbuffer.SrvTableStart());

    cl->SetPipelineState(ssaoBlurPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);
}
