#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "Vertex.h"
#include "InstanceData.h"
#include "../Graph/RenderPass.h"
#include <d3dcompiler.h>
#include <vector>
#include <cstring>

using namespace DirectX;

void Dx12Renderer::CreateIndirectPipeline()
{
    {
        D3D12_DESCRIPTOR_RANGE texRange{};
        texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        texRange.NumDescriptors = MaxTextures;
        texRange.BaseShaderRegister = 1;
        texRange.RegisterSpace = 0;
        texRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER rp[4]{};
        rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rp[0].Descriptor.ShaderRegister = 0;
        rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rp[1].Constants.ShaderRegister = 1;
        rp[1].Constants.RegisterSpace = 0;
        rp[1].Constants.Num32BitValues = 1;
        rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rp[2].Descriptor.ShaderRegister = 0;
        rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[3].DescriptorTable.NumDescriptorRanges = 1;
        rp[3].DescriptorTable.pDescriptorRanges = &texRange;
        rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.ShaderRegister = 0;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rs{};
        rs.NumParameters = 4; rs.pParameters = rp;
        rs.NumStaticSamplers = 1; rs.pStaticSamplers = &samp;
        rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> sig, err;
        HRESULT hr = D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1,
                                                 &sig, &err);
        if (FAILED(hr))
        {
            if (err)
            {
                OutputDebugStringA((const char*)err->GetBufferPointer());
            }
            HR(hr);
        }
        HR(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
            IID_PPV_ARGS(&m_indirectRootSig)));
    }

    {
        ComPtr<ID3DBlob> vs = LoadShader(L"Assets/Shaders/GBufferIndirectVs.hlsl", "main", "vs_5_1");
        ComPtr<ID3DBlob> ps = LoadShader(L"Assets/Shaders/GBufferIndirectPs.hlsl", "main", "ps_5_1");

        D3D12_INPUT_ELEMENT_DESC il[] =
        {
            {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0 ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"NORMAL"  ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"COLOR"   ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT   ,0,36,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        };

        D3D12_RASTERIZER_DESC rast{};
        rast.FillMode = D3D12_FILL_MODE_SOLID;
        rast.CullMode = D3D12_CULL_MODE_NONE;
        rast.FrontCounterClockwise = TRUE;
        rast.DepthClipEnable = TRUE;

        D3D12_BLEND_DESC blend{};
        for (int i = 0; i < GBuffer::kCount; i++) { blend.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; }

        D3D12_DEPTH_STENCIL_DESC ds{};
        ds.DepthEnable = TRUE;
        ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.InputLayout = { il, 4 };
        pso.pRootSignature = m_indirectRootSig.Get();
        pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
        pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
        pso.RasterizerState = rast;
        pso.BlendState = blend;
        pso.DepthStencilState = ds;
        pso.SampleMask = UINT_MAX;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets = GBuffer::kCount;
        for (int i = 0; i < GBuffer::kCount; i++) { pso.RTVFormats[i] = GBuffer::kFormat; }
        pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pso.SampleDesc.Count = 1;

        for (int fill = 0; fill < 2; fill++)
        {
            for (int cull = 0; cull < 2; cull++)
            {
                pso.RasterizerState.FillMode = fill ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
                pso.RasterizerState.CullMode = cull ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
                HR(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_indirectGbufferPsoVariant[fill][cull])));
            }
        }
    }

    {
        D3D12_INDIRECT_ARGUMENT_DESC args[2]{};
        args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        args[0].Constant.RootParameterIndex = 1;
        args[0].Constant.DestOffsetIn32BitValues = 0;
        args[0].Constant.Num32BitValuesToSet = 1;
        args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC desc{};
        desc.ByteStride = sizeof(IndirectDrawCommand);
        desc.NumArgumentDescs = 2;
        desc.pArgumentDescs = args;
        HR(device->CreateCommandSignature(&desc, m_indirectRootSig.Get(),
                                          IID_PPV_ARGS(&m_cmdSignature)));
    }
}

void Dx12Renderer::BuildGpuDrivenScene(const std::vector<Vertex>& verts,
                                       const std::vector<uint32_t>& indices,
                                       const std::vector<InstanceData>& instances,
                                       const std::vector<IndirectDrawCommand>& cmds)
{
    if (verts.empty() || cmds.empty()) { return; }

    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    void* mapped = nullptr;

    const size_t vbBytes = verts.size() * sizeof(Vertex);
    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(vbBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_unifiedVB)));
    m_unifiedVB->Map(0, nullptr, &mapped); memcpy(mapped, verts.data(), vbBytes); m_unifiedVB->Unmap(0, nullptr);
    m_unifiedVBView = { m_unifiedVB->GetGPUVirtualAddress(), (UINT)vbBytes, sizeof(Vertex) };

    const size_t ibBytes = indices.size() * sizeof(uint32_t);
    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(ibBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_unifiedIB)));
    m_unifiedIB->Map(0, nullptr, &mapped); memcpy(mapped, indices.data(), ibBytes); m_unifiedIB->Unmap(0, nullptr);
    m_unifiedIBView = { m_unifiedIB->GetGPUVirtualAddress(), (UINT)ibBytes, DXGI_FORMAT_R32_UINT };

    const size_t instBytes = instances.size() * sizeof(InstanceData);
    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(instBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer)));
    m_instanceBuffer->Map(0, nullptr, &mapped); memcpy(mapped, instances.data(), instBytes); m_instanceBuffer->Unmap(0, nullptr);

    const size_t cmdBytes = cmds.size() * sizeof(IndirectDrawCommand);
    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(cmdBytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indirectCmdBuffer)));
    m_indirectCmdBuffer->Map(0, nullptr, &mapped); memcpy(mapped, cmds.data(), cmdBytes); m_indirectCmdBuffer->Unmap(0, nullptr);

    m_indirectDrawCount = (UINT)cmds.size();
    m_totalInstances    = (UINT)instances.size();
    m_visibleInstances  = m_totalInstances;
}

void Dx12Renderer::PassGBufferIndirect(const RenderContext& ctx)
{
    auto* cl = ctx.cmdList;

    m_gbuffer.ToRenderTargets(cl);
    auto dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs = m_gbuffer.RtvHandles();
    cl->OMSetRenderTargets(GBuffer::kCount, rtvs, FALSE, &dsv);

    const float zero[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < GBuffer::kCount; i++)
    {
        cl->ClearRenderTargetView(rtvs[i], zero, 0, nullptr);
    }
    cl->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

    cl->SetGraphicsRootSignature(m_indirectRootSig.Get());
    cl->SetPipelineState(m_indirectGbufferPsoVariant[m_wireframe ? 1 : 0][m_backfaceCull ? 1 : 0].Get());

    CBData cb{};
    XMMATRIX vp = XMMatrixTranspose(ctx.viewProj);
    memcpy(cb.mvp, &vp, sizeof(cb.mvp));
    const UINT idx = cbRingIndex++;
    memcpy(cbMapped + size_t(idx) * cbStride, &cb, sizeof(cb));
    cl->SetGraphicsRootConstantBufferView(0,
        constantBuffer->GetGPUVirtualAddress() + UINT64(idx) * cbStride);

    cl->SetGraphicsRootShaderResourceView(2, m_instanceBuffer->GetGPUVirtualAddress());
    cl->SetGraphicsRootDescriptorTable(3, srvHeap->GetGPUDescriptorHandleForHeapStart());

    cl->IASetVertexBuffers(0, 1, &m_unifiedVBView);
    cl->IASetIndexBuffer(&m_unifiedIBView);

    cl->ExecuteIndirect(m_cmdSignature.Get(), m_indirectDrawCount,
                        m_indirectCmdBuffer.Get(), 0, nullptr, 0);
}
