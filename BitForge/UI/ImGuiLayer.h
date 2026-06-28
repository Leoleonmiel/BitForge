#pragma once
#include <windows.h>
#include <d3d12.h>

class ImGuiLayer
{
public:
    void Initialize(HWND hwnd,
                    ID3D12Device* device,
                    ID3D12DescriptorHeap* srvHeap,
                    UINT srvDescriptorSize,
                    UINT fontSrvIndex,
                    int framesInFlight,
                    DXGI_FORMAT rtvFormat);

    void BeginFrame();
    void EndFrame(ID3D12GraphicsCommandList* cmdList);

    void Shutdown();

    static bool WantCaptureMouse();
    static bool WantCaptureKeyboard();

private:
    bool m_initialized = false;
    ID3D12DescriptorHeap* m_srvHeap = nullptr;
};
