#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

class ShadowMap
{
public:
    static constexpr DXGI_FORMAT kDepthFormat   = DXGI_FORMAT_D32_FLOAT;
    static constexpr DXGI_FORMAT kTypelessFormat= DXGI_FORMAT_R32_TYPELESS;
    static constexpr DXGI_FORMAT kSrvFormat     = DXGI_FORMAT_R32_FLOAT;

    void Create(ID3D12Device* device, uint32_t size,
                ID3D12DescriptorHeap* dsvHeap, UINT dsvIndex, UINT dsvDescSize,
                ID3D12DescriptorHeap* srvHeap, UINT srvIndex, UINT srvDescSize);

    void ToDepthWrite(ID3D12GraphicsCommandList* cl);
    void ToShaderResource(ID3D12GraphicsCommandList* cl);

    D3D12_CPU_DESCRIPTOR_HANDLE Dsv() const { return m_dsv; }
    D3D12_GPU_DESCRIPTOR_HANDLE Srv() const { return m_srvGpu; }
    uint32_t Size() const { return m_size; }

private:
    void Transition(ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_STATES after);

    ComPtr<ID3D12Resource>      m_tex;
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsv{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpu{};
    uint32_t                    m_size  = 0;
    D3D12_RESOURCE_STATES       m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};
