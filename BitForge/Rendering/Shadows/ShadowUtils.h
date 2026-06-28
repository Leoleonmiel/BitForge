#pragma once
#include <DirectXMath.h>

namespace ShadowUtils
{
    DirectX::XMMATRIX DirectionalLightViewProj(
        DirectX::XMFLOAT3 sceneCenter,
        float             sceneRadius,
        DirectX::XMFLOAT3 lightDir);
}
