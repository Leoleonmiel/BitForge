#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "../Graph/RenderPass.h"
#include <d3dcompiler.h>

using namespace DirectX;

void Dx12Renderer::CreateDepthOfField()
{
    ComPtr<ID3DBlob> vs = LoadShader(L"Assets/Shaders/LightingVs.hlsl", "main", "vs_5_0");
    ComPtr<ID3DBlob> ps = LoadShader(L"Assets/Shaders/DofPs.hlsl",      "main", "ps_5_0");

    {
        D3D12_DESCRIPTOR_RANGE ranges[2]{};
        ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[0].NumDescriptors = 1;
        ranges[0].BaseShaderRegister = 0;
        ranges[0].OffsetInDescriptorsFromTableStart = 0;
        ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[1].NumDescriptors = 1;
        ranges[1].BaseShaderRegister = 1;
        ranges[1].OffsetInDescriptorsFromTableStart = 2;

        D3D12_ROOT_PARAMETER rp[2]{};
        rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rp[0].Descriptor.ShaderRegister = 0;
        rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[1].DescriptorTable.NumDescriptorRanges = 2;
        rp[1].DescriptorTable.pDescriptorRanges = ranges;
        rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.ShaderRegister = 0;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rs{};
        rs.NumParameters = 2; rs.pParameters = rp;
        rs.NumStaticSamplers = 1; rs.pStaticSamplers = &samp;
        rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ComPtr<ID3DBlob> sig;
        HR(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, nullptr));
        HR(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
            IID_PPV_ARGS(&dofRootSig)));
    }

    {
        D3D12_RASTERIZER_DESC rast{};
        rast.FillMode = D3D12_FILL_MODE_SOLID;
        rast.CullMode = D3D12_CULL_MODE_NONE;
        rast.DepthClipEnable = TRUE;

        D3D12_BLEND_DESC blend{};
        blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_DEPTH_STENCIL_DESC ds{}; ds.DepthEnable = FALSE;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.InputLayout = { nullptr, 0 };
        pso.pRootSignature = dofRootSig.Get();
        pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
        pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
        pso.RasterizerState = rast;
        pso.BlendState = blend;
        pso.DepthStencilState = ds;
        pso.SampleMask = UINT_MAX;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets = 1;
        pso.RTVFormats[0] = HdrTarget::kFormat;
        pso.DSVFormat = DXGI_FORMAT_UNKNOWN;
        pso.SampleDesc.Count = 1;
        HR(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&dofPso)));
    }
}

void Dx12Renderer::PassDepthOfField(const RenderContext& ctx)
{
    if (!m_camera.dofEnabled) { return; }

    auto* cl = ctx.cmdList;

    m_ssr.ToShaderResource(cl);
    m_hdr.ToRenderTarget(cl);
    auto rtv = m_hdr.Rtv();
    cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(dofRootSig.Get());

    CBData cb{};
    cb.cameraPosSpecPower[0] = ctx.cameraPos.x;
    cb.cameraPosSpecPower[1] = ctx.cameraPos.y;
    cb.cameraPosSpecPower[2] = ctx.cameraPos.z;
    cb.cameraPosSpecPower[3] = m_camera.autoFocus ? 1.0f : 0.0f;
    cb.specColor[0] = m_camera.focusDistance;
    cb.specColor[1] = m_camera.aperture;
    cb.specColor[2] = m_camera.depthOfFieldStrength;
    cb.specColor[3] = m_camera.farPlane;
    PushConstants(cb);

    cl->SetGraphicsRootDescriptorTable(1, m_ssr.Srv());

    cl->SetPipelineState(dofPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);
}
