#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "../Graph/RenderPass.h"
#include <d3dcompiler.h>

using namespace DirectX;

static void MakeFullscreenPso(ID3D12Device* device, ID3D12RootSignature* rs,
                              ID3DBlob* vs, ID3DBlob* ps, ID3D12PipelineState** out)
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
    pso.pRootSignature = rs;
    pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    pso.RasterizerState = rast;
    pso.BlendState = blend;
    pso.DepthStencilState = ds;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_UNKNOWN;
    pso.SampleDesc.Count = 1;
    HR(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(out)));
}

void Dx12Renderer::CreateSsao()
{
    m_ssaoRaw.Create(device.Get(), m_width, m_height,
                     gbufferRtvHeap.Get(), GBuffer::kCount + 2, rtvDescriptorSize,
                     srvHeap.Get(), kSsaoSrvIndex, srvDescriptorSize);
    m_ssaoBlur.Create(device.Get(), m_width, m_height,
                      gbufferRtvHeap.Get(), GBuffer::kCount + 3, rtvDescriptorSize,
                      srvHeap.Get(), kSsaoBlurSrvIndex, srvDescriptorSize);

    XMFLOAT4 kernel[64];
    SsaoGenerateKernel(kernel, 64);
    D3D12_HEAP_PROPERTIES up{}; up.Type = D3D12_HEAP_TYPE_UPLOAD;
    HR(device->CreateCommittedResource(&up, D3D12_HEAP_FLAG_NONE, &BufferDesc(sizeof(kernel)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ssaoKernelCB)));
    void* mapped = nullptr;
    ssaoKernelCB->Map(0, nullptr, &mapped);
    memcpy(mapped, kernel, sizeof(kernel));
    ssaoKernelCB->Unmap(0, nullptr);

    uint8_t noise[16 * 4];
    SsaoGenerateNoise(noise);
    Texture noiseTex = CreateTextureFromMemory(noise, 4, 4);
    m_ssaoNoiseSrv = srvHeap->GetGPUDescriptorHandleForHeapStart();
    m_ssaoNoiseSrv.ptr += (UINT64)noiseTex.srvIndex * srvDescriptorSize;

    {
        ComPtr<ID3DBlob> vs = LoadShader(L"Assets/Shaders/LightingVs.hlsl", "main", "vs_5_0");
        ComPtr<ID3DBlob> ps = LoadShader(L"Assets/Shaders/SsaoPs.hlsl",     "main", "ps_5_0");

        D3D12_DESCRIPTOR_RANGE gbufRange{};
        gbufRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        gbufRange.NumDescriptors = 2;
        gbufRange.BaseShaderRegister = 0;
        gbufRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_DESCRIPTOR_RANGE noiseRange{};
        noiseRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        noiseRange.NumDescriptors = 1;
        noiseRange.BaseShaderRegister = 2;
        noiseRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER rp[4]{};
        rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rp[0].Descriptor.ShaderRegister = 0;
        rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rp[1].Descriptor.ShaderRegister = 1;
        rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[2].DescriptorTable.NumDescriptorRanges = 1;
        rp[2].DescriptorTable.pDescriptorRanges = &gbufRange;
        rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[3].DescriptorTable.NumDescriptorRanges = 1;
        rp[3].DescriptorTable.pDescriptorRanges = &noiseRange;
        rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp[2]{};
        samp[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        samp[0].AddressU = samp[0].AddressV = samp[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp[0].ShaderRegister = 0; samp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samp[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        samp[1].AddressU = samp[1].AddressV = samp[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp[1].ShaderRegister = 1; samp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rs{};
        rs.NumParameters = 4; rs.pParameters = rp;
        rs.NumStaticSamplers = 2; rs.pStaticSamplers = samp;
        rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ComPtr<ID3DBlob> sig;
        HR(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, nullptr));
        HR(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
            IID_PPV_ARGS(&ssaoRootSig)));
        MakeFullscreenPso(device.Get(), ssaoRootSig.Get(), vs.Get(), ps.Get(), &ssaoPso);
    }

    {
        ComPtr<ID3DBlob> vs = LoadShader(L"Assets/Shaders/LightingVs.hlsl",   "main", "vs_5_0");
        ComPtr<ID3DBlob> ps = LoadShader(L"Assets/Shaders/SsaoBlurPs.hlsl",  "main", "ps_5_0");

        D3D12_DESCRIPTOR_RANGE rawRange{};
        rawRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rawRange.NumDescriptors = 1; rawRange.BaseShaderRegister = 0;
        rawRange.OffsetInDescriptorsFromTableStart = 0;
        D3D12_DESCRIPTOR_RANGE posRange{};
        posRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        posRange.NumDescriptors = 1; posRange.BaseShaderRegister = 1;
        posRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER rp[3]{};
        rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rp[0].Descriptor.ShaderRegister = 0;
        rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[1].DescriptorTable.NumDescriptorRanges = 1;
        rp[1].DescriptorTable.pDescriptorRanges = &rawRange;
        rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[2].DescriptorTable.NumDescriptorRanges = 1;
        rp[2].DescriptorTable.pDescriptorRanges = &posRange;
        rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.ShaderRegister = 0; samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rs{};
        rs.NumParameters = 3; rs.pParameters = rp;
        rs.NumStaticSamplers = 1; rs.pStaticSamplers = &samp;
        rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ComPtr<ID3DBlob> sig;
        HR(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, nullptr));
        HR(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
            IID_PPV_ARGS(&ssaoBlurRootSig)));
        MakeFullscreenPso(device.Get(), ssaoBlurRootSig.Get(), vs.Get(), ps.Get(), &ssaoBlurPso);
    }
}

static float SsaoSceneScale(float sceneRadius)
{
    return (sceneRadius > 1.0f ? sceneRadius : 50.0f) * 0.06f;
}

void Dx12Renderer::PassSsao(const RenderContext& ctx)
{
    auto* cl = ctx.cmdList;

    if (!m_ssao.enabled)
    {
        m_ssaoBlur.ToRenderTarget(cl);
        const float white[4] = { 1, 1, 1, 1 };
        cl->ClearRenderTargetView(m_ssaoBlur.Rtv(), white, 0, nullptr);
        return;
    }

    m_gbuffer.ToShaderResources(cl);
    m_ssaoRaw.ToRenderTarget(cl);
    auto rtv = m_ssaoRaw.Rtv();
    cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    cl->SetGraphicsRootSignature(ssaoRootSig.Get());

    const float scale = SsaoSceneScale(m_sceneRadius);
    CBData cb{};
    XMMATRIX vp = XMMatrixTranspose(ctx.viewProj);
    memcpy(cb.mvp, &vp, sizeof(cb.mvp));
    cb.cameraPosSpecPower[0] = ctx.cameraPos.x;
    cb.cameraPosSpecPower[1] = ctx.cameraPos.y;
    cb.cameraPosSpecPower[2] = ctx.cameraPos.z;
    cb.specColor[0] = m_ssao.radius * scale;
    cb.specColor[1] = m_ssao.bias   * scale;
    cb.specColor[2] = m_ssao.intensity;
    cb.specColor[3] = float(m_ssao.kernelSize);
    PushConstants(cb);

    cl->SetGraphicsRootConstantBufferView(1, ssaoKernelCB->GetGPUVirtualAddress());
    cl->SetGraphicsRootDescriptorTable(2, m_gbuffer.SrvTableStart());
    cl->SetGraphicsRootDescriptorTable(3, m_ssaoNoiseSrv);

    cl->SetPipelineState(ssaoPso.Get());
    cl->DrawInstanced(3, 1, 0, 0);
}
