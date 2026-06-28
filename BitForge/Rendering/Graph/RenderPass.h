#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <functional>
#include <string>

struct RenderContext
{
    ID3D12GraphicsCommandList* cmdList = nullptr;
    DirectX::XMMATRIX          view    = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX          proj    = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX          viewProj = DirectX::XMMatrixIdentity();
    DirectX::XMFLOAT3          cameraPos{ 0, 0, 0 };
};

using PassExecuteFn = std::function<void(const RenderContext&)>;

struct RenderPass
{
    std::string   name;
    PassExecuteFn execute;
};
