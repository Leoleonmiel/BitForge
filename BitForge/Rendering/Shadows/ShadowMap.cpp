#include "ShadowMap.h"


static void Trap(HRESULT hr) { if (FAILED(hr)) __debugbreak(); }

void ShadowMap::Create(ID3D12Device* device, uint32_t size,
                       ID3D12DescriptorHeap* dsvHeap, UINT dsvIndex, UINT dsvDescSize,
                       ID3D12DescriptorHeap* srvHeap, UINT srvIndex, UINT srvDescSize)
{
    m_size = size;

    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC td{};
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = size;
    td.Height = size;
    td.DepthOrArraySize = 1;
    td.MipLevels = 1;
    td.Format = kTypelessFormat;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    td.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear{};
    clear.Format = kDepthFormat;
    clear.DepthStencil.Depth = 1.0f;

    Trap(device->CreateCommittedResource(
        &heap, D3D12_HEAP_FLAG_NONE, &td,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear, IID_PPV_ARGS(&m_tex)));

    m_dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_dsv.ptr += (SIZE_T)dsvIndex * dsvDescSize;
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
    dsv.Format = kDepthFormat;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(m_tex.Get(), &dsv, m_dsv);

    auto srvCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
    auto srvGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
    srvCpu.ptr += (SIZE_T)srvIndex * srvDescSize;
    srvGpu.ptr += (UINT64)srvIndex * srvDescSize;
    m_srvGpu = srvGpu;

    D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = kSrvFormat;
    sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sd.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_tex.Get(), &sd, srvCpu);

    m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void ShadowMap::Transition(ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_STATES after)
{
    if (m_state == after) { return; }
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_tex.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = m_state;
    barrier.Transition.StateAfter = after;
    cl->ResourceBarrier(1, &barrier);
    m_state = after;
}

void ShadowMap::ToDepthWrite(ID3D12GraphicsCommandList* cl)
{
    Transition(cl, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void ShadowMap::ToShaderResource(ID3D12GraphicsCommandList* cl)
{
    Transition(cl, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
