#pragma once
#include "Vertex.h"
#include <windows.h>
#include <d3d12.h>
#include <string>
#include <cstdint>

inline void HR(HRESULT hr) { if (FAILED(hr)) __debugbreak(); }

inline std::wstring ExeDirW()
{
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p(buf);
    size_t slash = p.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? L"" : p.substr(0, slash + 1);
}

inline std::string ExeDirA()
{
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string p(buf);
    size_t slash = p.find_last_of("\\/");
    return (slash == std::string::npos) ? "" : p.substr(0, slash + 1);
}

inline D3D12_RESOURCE_DESC& BufferDesc(UINT64 size)
{
    static thread_local D3D12_RESOURCE_DESC desc{};
    desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return desc;
}

inline D3D12_RESOURCE_DESC& DepthDesc(UINT width, UINT height)
{
    static thread_local D3D12_RESOURCE_DESC desc{};
    desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    return desc;
}

extern const Vertex   cubeVertices[24];
extern const uint16_t cubeIndices[36];
extern const Vertex   groundVertices[4];
extern const uint16_t groundIndices[6];
