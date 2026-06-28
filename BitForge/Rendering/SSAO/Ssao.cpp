#include "Ssao.h"
#include <random>

using namespace DirectX;

static void Trap(HRESULT hr) { if (FAILED(hr)) __debugbreak(); }

void SsaoTarget::Create(ID3D12Device* device, UINT width, UINT height,
                        ID3D12DescriptorHeap* rtvHeap, UINT rtvIndex, UINT rtvDescSize,
                        ID3D12DescriptorHeap* srvHeap, UINT srvIndex, UINT srvDescSize)
{
    const DXGI_FORMAT fmt = DXGI_FORMAT_R8_UNORM;

    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC td{};
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = width; td.Height = height;
    td.DepthOrArraySize = 1; td.MipLevels = 1;
    td.Format = fmt; td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    td.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clear{};
    clear.Format = fmt;
    clear.Color[0] = clear.Color[1] = clear.Color[2] = clear.Color[3] = 1.0f;

    Trap(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &td,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&m_tex)));

    m_rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_rtv.ptr += (SIZE_T)rtvIndex * rtvDescSize;
    device->CreateRenderTargetView(m_tex.Get(), nullptr, m_rtv);

    auto srvCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
    auto srvGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
    srvCpu.ptr += (SIZE_T)srvIndex * srvDescSize;
    srvGpu.ptr += (UINT64)srvIndex * srvDescSize;
    m_srvGpu = srvGpu;

    D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = fmt;
    sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sd.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_tex.Get(), &sd, srvCpu);

    m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

void SsaoTarget::Transition(ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_STATES after)
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

void SsaoTarget::ToRenderTarget(ID3D12GraphicsCommandList* cl)
{ Transition(cl, D3D12_RESOURCE_STATE_RENDER_TARGET); }

void SsaoTarget::ToShaderResource(ID3D12GraphicsCommandList* cl)
{ Transition(cl, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); }

void SsaoGenerateKernel(DirectX::XMFLOAT4* out, int count)
{
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f), u11(-1.0f, 1.0f);

    for (int i = 0; i < count; i++)
    {
        XMVECTOR sample = XMVectorSet(u11(rng), u11(rng), u01(rng), 0.0f);
        sample = XMVector3Normalize(sample);
        sample = XMVectorScale(sample, u01(rng));

        float progression = (float)i / (float)count;
        float scale = 0.1f + 0.9f * progression * progression;
        sample = XMVectorScale(sample, scale);

        XMFLOAT3 sampleVec; XMStoreFloat3(&sampleVec, sample);
        out[i] = XMFLOAT4(sampleVec.x, sampleVec.y, sampleVec.z, 0.0f);
    }
}

void SsaoGenerateNoise(uint8_t* out)
{
    std::mt19937 rng(7331);
    std::uniform_real_distribution<float> u11(-1.0f, 1.0f);
    for (int i = 0; i < 16; i++)
    {
        float randomX = u11(rng), randomY = u11(rng);
        out[i * 4 + 0] = (uint8_t)((randomX * 0.5f + 0.5f) * 255.0f);
        out[i * 4 + 1] = (uint8_t)((randomY * 0.5f + 0.5f) * 255.0f);
        out[i * 4 + 2] = 128;
        out[i * 4 + 3] = 255;
    }
}
