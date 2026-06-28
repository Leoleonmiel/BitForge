#pragma once
#include <DirectXMath.h>

struct Ray
{
    DirectX::XMFLOAT3 origin{ 0, 0, 0 };
    DirectX::XMFLOAT3 direction{ 0, 0, 1 };
};

struct RayReconstructionSettings
{
    bool  enabled   = true;
    float stepSize  = 0.0f;
    int   maxSteps  = 48;
    float intensity = 1.0f;
};
