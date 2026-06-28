#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;


class SsaoTarget
{
public:
    void Create(ID3D12Device* device, UINT width, UINT height,
                ID3D12DescriptorHeap* rtvHeap, UINT rtvIndex, UINT rtvDescSize,
                ID3D12DescriptorHeap* srvHeap, UINT srvIndex, UINT srvDescSize);

    void ToRenderTarget(ID3D12GraphicsCommandList* cl);
    void ToShaderResource(ID3D12GraphicsCommandList* cl);

    D3D12_CPU_DESCRIPTOR_HANDLE Rtv() const { return m_rtv; }
    D3D12_GPU_DESCRIPTOR_HANDLE Srv() const { return m_srvGpu; }

private:
    void Transition(ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_STATES after);

    ComPtr<ID3D12Resource>      m_tex;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpu{};
    D3D12_RESOURCE_STATES       m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
};

struct SsaoSettings
{
    bool  enabled   = true;
    bool  blur      = true;
    float radius    = 0.6f;
    float bias      = 0.03f;
    float intensity = 1.4f;
    int   kernelSize = 32;
};

void SsaoGenerateKernel(DirectX::XMFLOAT4* out, int count);

void SsaoGenerateNoise(uint8_t* outRGBA16px);
