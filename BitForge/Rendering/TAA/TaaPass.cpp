#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "../Graph/RenderPass.h"
#include <d3dcompiler.h>

using namespace DirectX;

void Dx12Renderer::CreateTaa()
{
    m_taaHistory[0].Create(device.Get(), m_width, m_height,
                           gbufferRtvHeap.Get(), GBuffer::kCount + 5, rtvDescriptorSize,
                           srvHeap.Get(), kTaaHistSrvA, srvDescriptorSize);
    m_taaHistory[1].Create(device.Get(), m_width, m_height,
                           gbufferRtvHeap.Get(), GBuffer::kCount + 6, rtvDescriptorSize,
                           srvHeap.Get(), kTaaHistSrvB, srvDescriptorSize);

    ComPtr<ID3DBlob> vs = LoadShader(L"Assets/Shaders/LightingVs.hlsl",  "main", "vs_5_0");
    ComPtr<ID3DBlob> ps = LoadShader(L"Assets/Shaders/TaaResolve.hlsl",  "main", "ps_5_0");

    {
        D3D12_DESCRIPTOR_RANGE curR{};
        curR.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        curR.NumDescriptors = 1; curR.BaseShaderRegister = 0;
        D3D12_DESCRIPTOR_RANGE histR{};
        histR.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        histR.NumDescriptors = 1; histR.BaseShaderRegister = 1;
        D3D12_DESCRIPTOR_RANGE posR{};
        posR.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        posR.NumDescriptors = 1; posR.BaseShaderRegister = 2;

        D3D12_ROOT_PARAMETER rp[4]{};
        rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rp[0].Descriptor.ShaderRegister = 0;
        rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[1].DescriptorTable.NumDescriptorRanges = 1;
        rp[1].DescriptorTable.pDescriptorRanges = &curR;
        rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[2].DescriptorTable.NumDescriptorRanges = 1;
        rp[2].DescriptorTable.pDescriptorRanges = &histR;
        rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[3].DescriptorTable.NumDescriptorRanges = 1;
        rp[3].DescriptorTable.pDescriptorRanges = &posR;
        rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.ShaderRegister = 0; samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rs{};
        rs.NumParameters = 4; rs.pParameters = rp;
        rs.NumStaticSamplers = 1; rs.pStaticSamplers = &samp;
        rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ComPtr<ID3DBlob> sig;
        HR(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, nullptr));
        HR(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
            IID_PPV_ARGS(&taaRootSig)));
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
        pso.pRootSignature = taaRootSig.Get();
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
        HR(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&taaPso)));
    }
}

void Dx12Renderer::PassTaa(const RenderContext& ctx)
{
    if (!m_taa.enabled)
    {
        m_taaHistoryValid = false;
        return;
    }

    auto* cl = ctx.cmdList;

    HdrTarget& curSrc = m_fog.enabled ? m_fogTarget
                                      : (m_camera.dofEnabled ? m_hdr : m_ssr);
    HdrTarget& history = m_taaHistory[1 - m_taaWrite];
    HdrTarget& output  = m_taaHistory[m_taaWrite];

    curSrc.ToShaderResource(cl);
    history.ToShaderResource(cl);
    output.ToRenderTarget(cl);
    auto rtv = output.Rtv();
    cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(taaRootSig.Get());

    CBData cb{};
    XMMATRIX prevVP = XMMatrixTranspose(m_prevViewProj);
    XMMATRIX invCur = XMMatrixTranspose(XMMatrixInverse(nullptr, m_curUnjitteredVP));
    memcpy(cb.mvp,   &prevVP, sizeof(cb.mvp));
    memcpy(cb.world, &invCur, sizeof(cb.world));
    cb.cameraPosSpecPower[0] = ctx.cameraPos.x;
    cb.cameraPosSpecPower[1] = ctx.cameraPos.y;
    cb.cameraPosSpecPower[2] = ctx.cameraPos.z;
    cb.cameraPosSpecPower[3] = float(m_taa.debug);
    cb.specColor[0] = m_taa.blend;
    cb.specColor[1] = m_taaHistoryValid ? 1.0f : 0.0f;
    PushConstants(cb);

    cl->SetGraphicsRootDescriptorTable(1, curSrc.Srv());
    cl->SetGraphicsRootDescriptorTable(2, history.Srv());
    cl->SetGraphicsRootDescriptorTable(3, m_gbuffer.SrvTableStart());

    cl->SetPipelineState(taaPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);

    m_taaWrite = 1 - m_taaWrite;
    m_taaHistoryValid = true;
}
