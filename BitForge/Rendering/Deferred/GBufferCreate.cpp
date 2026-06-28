#include "GBuffer.h"


static void Trap(HRESULT hr) { if (FAILED(hr)) __debugbreak(); }

void GBuffer::Create(ID3D12Device* device, UINT width, UINT height,
                     ID3D12DescriptorHeap* rtvHeap, UINT rtvDescSize,
                     ID3D12DescriptorHeap* srvHeap, UINT srvBaseIndex, UINT srvDescSize)
{
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC td{};
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = width;
    td.Height = height;
    td.DepthOrArraySize = 1;
    td.MipLevels = 1;
    td.Format = kFormat;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    td.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clear{};
    clear.Format = kFormat;
    clear.Color[0] = clear.Color[1] = clear.Color[2] = clear.Color[3] = 0.0f;

    auto rtvCpu = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    auto srvCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
    auto srvGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
    srvCpu.ptr += (SIZE_T)srvBaseIndex * srvDescSize;
    srvGpu.ptr += (UINT64)srvBaseIndex * srvDescSize;
    m_srvGpuStart = srvGpu;

    for (int i = 0; i < kCount; i++)
    {
        Trap(device->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE, &td,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&m_tex[i])));

        m_rtv[i] = rtvCpu;
        m_rtv[i].ptr += (SIZE_T)i * rtvDescSize;
        device->CreateRenderTargetView(m_tex[i].Get(), nullptr, m_rtv[i]);

        D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = kFormat;
        sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        sd.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_tex[i].Get(), &sd, srvCpu);
        srvCpu.ptr += srvDescSize;
    }

    m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
}
