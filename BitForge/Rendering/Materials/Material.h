#pragma once
#include "TextureSet.h"
#include <DirectXMath.h>
#include <cstdint>

struct Material
{
    TextureSet textures;

    DirectX::XMFLOAT3 albedoFactor{ 1, 1, 1 };
    float metallicFactor  = 0.0f;
    float roughnessFactor  = 0.6f;
    float aoFactor         = 1.0f;
};
