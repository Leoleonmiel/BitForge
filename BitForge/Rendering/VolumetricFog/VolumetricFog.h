#pragma once
#include <DirectXMath.h>


struct FogSettings
{
    bool  enabled      = true;
    float density      = 0.06f;
    float heightFactor = 1.0f;
    float anisotropy   = 0.72f;
    int   stepCount    = 48;
    float maxDistance  = 100.0f;
    float ambientIntensity = 0.30f;
    DirectX::XMFLOAT3 fogColor = { 0.58f, 0.66f, 0.80f };
};
