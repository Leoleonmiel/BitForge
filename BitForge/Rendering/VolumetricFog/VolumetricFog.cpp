#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include <d3dcompiler.h>

using namespace DirectX;

void Dx12Renderer::CreateVolumetricFog()
{
    m_fogTarget.Create(device.Get(), m_width, m_height,
                       gbufferRtvHeap.Get(), GBuffer::kCount + 4, rtvDescriptorSize,
                       srvHeap.Get(), kFogSrvIndex, srvDescriptorSize);

    ComPtr<ID3DBlob> vs = LoadShader(L"Assets/Shaders/LightingVs.hlsl", "main", "vs_5_0");
    ComPtr<ID3DBlob> ps = LoadShader(L"Assets/Shaders/FogPs.hlsl",      "main", "ps_5_0");

    {
        D3D12_DESCRIPTOR_RANGE sceneRange{};
        sceneRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        sceneRange.NumDescriptors = 1; sceneRange.BaseShaderRegister = 0;
        sceneRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_DESCRIPTOR_RANGE gposRange{};
        gposRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        gposRange.NumDescriptors = 1; gposRange.BaseShaderRegister = 1;
        gposRange.OffsetInDescriptorsFromTableStart = 0;
        D3D12_DESCRIPTOR_RANGE shadowRange{};
        shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        shadowRange.NumDescriptors = 1; shadowRange.BaseShaderRegister = 2;
        shadowRange.OffsetInDescriptorsFromTableStart = GBuffer::kCount;
        D3D12_DESCRIPTOR_RANGE gbufRanges[2] = { gposRange, shadowRange };

        D3D12_ROOT_PARAMETER rp[3]{};
        rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rp[0].Descriptor.ShaderRegister = 0;
        rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[1].DescriptorTable.NumDescriptorRanges = 1;
        rp[1].DescriptorTable.pDescriptorRanges = &sceneRange;
        rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[2].DescriptorTable.NumDescriptorRanges = 2;
        rp[2].DescriptorTable.pDescriptorRanges = gbufRanges;
        rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp[2]{};
        samp[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp[0].AddressU = samp[0].AddressV = samp[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp[0].MaxLOD = D3D12_FLOAT32_MAX;
        samp[0].ShaderRegister = 0; samp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samp[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        samp[1].AddressU = samp[1].AddressV = samp[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        samp[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samp[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        samp[1].MaxLOD = D3D12_FLOAT32_MAX;
        samp[1].ShaderRegister = 1; samp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rs{};
        rs.NumParameters = 3; rs.pParameters = rp;
        rs.NumStaticSamplers = 2; rs.pStaticSamplers = samp;
        rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ComPtr<ID3DBlob> sig;
        HR(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, nullptr));
        HR(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
            IID_PPV_ARGS(&fogRootSig)));
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
        pso.pRootSignature = fogRootSig.Get();
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
        HR(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&fogPso)));
    }
}
