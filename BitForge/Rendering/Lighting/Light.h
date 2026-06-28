#pragma once
#include "LightingTypes.h"
#include <DirectXMath.h>

struct Light
{
    DirectX::XMFLOAT3 position{ 0, 0, 0 };
    float             radius   = 10.0f;

    DirectX::XMFLOAT3 color{ 1, 1, 1 };
    float             intensity = 1.0f;

    DirectX::XMFLOAT3 direction{ 0, -1, 0 };
    int               type = (int)LightType::Point;

    bool enabled = true;
};
