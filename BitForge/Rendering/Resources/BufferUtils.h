#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

namespace BufferUtils
{
    D3D12_RESOURCE_DESC BufferDesc(UINT64 size);
    D3D12_RESOURCE_DESC DepthDesc(UINT width, UINT height);

    ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device* device,
                                              UINT64 size,
                                              const void* initData = nullptr);
}
