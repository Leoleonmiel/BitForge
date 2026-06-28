#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

class GBuffer
{
public:
    static constexpr int kCount = 4;
    static constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    void Create(ID3D12Device* device, UINT width, UINT height,
                ID3D12DescriptorHeap* rtvHeap, UINT rtvDescSize,
                ID3D12DescriptorHeap* srvHeap, UINT srvBaseIndex, UINT srvDescSize);

    void ToRenderTargets(ID3D12GraphicsCommandList* cl);
    void ToShaderResources(ID3D12GraphicsCommandList* cl);

    const D3D12_CPU_DESCRIPTOR_HANDLE* RtvHandles() const { return m_rtv; }
    D3D12_GPU_DESCRIPTOR_HANDLE SrvTableStart() const { return m_srvGpuStart; }

private:
    void Transition(ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_STATES after);

    ComPtr<ID3D12Resource>      m_tex[kCount];
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv[kCount]{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuStart{};
    D3D12_RESOURCE_STATES       m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
};
