#include "ImGuiLayer.h"

#include "../thirdparty/imgui/imgui.h"
#include "../thirdparty/imgui/backends/imgui_impl_win32.h"
#include "../thirdparty/imgui/backends/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    WNDPROC g_originalProc = nullptr;

    LRESULT CALLBACK BitForgeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui::GetCurrentContext() != nullptr &&
            ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) { return 1; }

        return CallWindowProc(g_originalProc, hWnd, msg, wParam, lParam);
    }
}

void ImGuiLayer::Initialize(HWND hwnd,
                            ID3D12Device* device,
                            ID3D12DescriptorHeap* srvHeap,
                            UINT srvDescriptorSize,
                            UINT fontSrvIndex,
                            int framesInFlight,
                            DXGI_FORMAT rtvFormat)
{
    m_srvHeap = srvHeap;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
    cpu.ptr += (SIZE_T)fontSrvIndex * srvDescriptorSize;
    gpu.ptr += (UINT64)fontSrvIndex * srvDescriptorSize;

    ImGui_ImplDX12_Init(
        device,
        framesInFlight,
        rtvFormat,
        srvHeap,
        cpu,
        gpu);

    g_originalProc = (WNDPROC)SetWindowLongPtr(
        hwnd, GWLP_WNDPROC, (LONG_PTR)BitForgeWndProc);

    m_initialized = true;
}

void ImGuiLayer::BeginFrame()
{
    if (!m_initialized) { return; }
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized) { return; }
    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap };
    cmdList->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImGuiLayer::Shutdown()
{
    if (!m_initialized) { return; }
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

bool ImGuiLayer::WantCaptureMouse()
{
    return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::WantCaptureKeyboard()
{
    return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard;
}
