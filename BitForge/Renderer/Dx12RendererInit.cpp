#include "Dx12Renderer.h"
#include "Dx12RendererInternal.h"
#include "Vertex.h"
#include "ModelLoader.h"
#include "GLTFLoader.h"
#include <d3dcompiler.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>

using namespace DirectX;
#include "Input/Input.h"

void Dx12Renderer::CreateDepthBuffer()
{
    D3D12_DESCRIPTOR_HEAP_DESC dsv{};
    dsv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv.NumDescriptors = 2;
    HR(device->CreateDescriptorHeap(&dsv, IID_PPV_ARGS(&dsvHeap)));

    D3D12_CLEAR_VALUE clear{};
    clear.Format = DXGI_FORMAT_D32_FLOAT;
    clear.DepthStencil.Depth = 1.0f;

    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &DepthDesc(m_width, m_height),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&depthBuffer)));

    device->CreateDepthStencilView(depthBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Renderer::Initialize(HWND hwnd, uint32_t width, uint32_t height)
{
    m_hwnd = hwnd;

    RECT client{};
    if (GetClientRect(hwnd, &client))
    {
        const uint32_t clientWidth = (uint32_t)(client.right - client.left);
        const uint32_t clientHeight = (uint32_t)(client.bottom - client.top);
        if (clientWidth > 0 && clientHeight > 0)
        {
            width = clientWidth;
            height = clientHeight;
        }
    }
    m_width = width; m_height = height;

    char dbg[128];
    sprintf_s(dbg, "[BitForge] Backbuffer: %u x %u  aspect=%.4f\n",
              m_width, m_height, float(m_width) / float(m_height));
    OutputDebugStringA(dbg);

    ComPtr<IDXGIFactory4> factory;
    HR(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    HR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HR(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)));

    DXGI_SWAP_CHAIN_DESC1 sc{};
    sc.BufferCount = FrameCount;
    sc.Width = 0;
    sc.Height = 0;
    sc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> sc1;
    HR(factory->CreateSwapChainForHwnd(queue.Get(), hwnd, &sc, nullptr, nullptr, &sc1));
    HR(sc1.As(&swapChain));

    DXGI_SWAP_CHAIN_DESC1 actual{};
    swapChain->GetDesc1(&actual);
    m_width = actual.Width;
    m_height = actual.Height;

    sprintf_s(dbg, "[BitForge] Swapchain actual: %u x %u  aspect=%.4f\n",
              m_width, m_height, float(m_width) / float(m_height));
    OutputDebugStringA(dbg);

    D3D12_DESCRIPTOR_HEAP_DESC rtv{};
    rtv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv.NumDescriptors = FrameCount;
    HR(device->CreateDescriptorHeap(&rtv, IID_PPV_ARGS(&rtvHeap)));

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    auto hrtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < FrameCount; i++)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
        device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, hrtv);
        hrtv.ptr += rtvDescriptorSize;
    }

    for (UINT i = 0; i < FrameCount; i++)
    {
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators[i]));
    }

    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocators[0].Get(), nullptr, IID_PPV_ARGS(&cmdList));
    cmdList->Close();

    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    CreateDepthBuffer();

    CreateSrvHeap();

    CreateGBuffer();
    CreateDeferredPipeline();
    CreateShadowResources();
    CreatePostProcess();
    CreateRayReconstruction();
    CreateDepthOfField();
    CreateGizmoResources();
    CreateLightBuffer();
    CreateIndirectPipeline();

    {
        Light sun;
        sun.type = (int)LightType::Directional;
        sun.direction = { 0.5f, 1.0f, 0.4f };
        sun.color = { 1.0f, 0.95f, 0.90f };
        sun.intensity = 3.0f;
        m_lightManager.AddLight(sun);
    }

    {
        const uint8_t grey[4] = { 200, 200, 200, 255 };
        m_textures.push_back(CreateTextureFromMemory(grey, 1, 1));
    }

    m_assets.Initialize();
    m_assets.SetTextureUploader(
        [this](const uint8_t* pixels, uint32_t width, uint32_t height) { return UploadTexturePixels(pixels, width, height); });
    m_assets.SetFallbackTexture(0);

    CreateSsao();
    CreateVolumetricFog();
    CreateTaa();

    cbStride = (UINT)((sizeof(CBData) + 255) & ~255);
    UINT64 total = UINT64(cbStride) * MaxCBPerFrame;

    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &BufferDesc(total),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer));

    constantBuffer->Map(0, nullptr, (void**)&cbMapped);
    memset(cbMapped, 0, (size_t)total);

    viewport = { 0,0,(float)width,(float)height,0,1 };
    scissor = { 0,0,(LONG)width,(LONG)height };

    m_camera.SetPosition(0.0f, 5.0f, -15.0f);
    m_camera.SetSpeed(10.0f);

    m_imgui.Initialize(m_hwnd, device.Get(), srvHeap.Get(),
                       srvDescriptorSize, MaxTextures - 1,
                       (int)FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM);
}
