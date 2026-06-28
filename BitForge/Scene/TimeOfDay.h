#pragma once
#include <DirectXMath.h>

enum class TimeOfDayState
{
    Day,
    Night
};

struct TimeOfDayPreset
{
    DirectX::XMFLOAT3 sunDirection;
    float             sunIntensity;
    DirectX::XMFLOAT3 sunColor;
    float             iblIntensity;
    float             exposure;
    float             nightFactor;
    bool              pointLightsOn;
};

namespace TimeOfDay
{
    inline TimeOfDayPreset DayPreset()
    {
        return {
            { 0.5f, 1.0f, 0.4f },
            3.5f,
            { 1.00f, 0.95f, 0.90f },
            1.0f,
            1.2f,
            0.0f,
            false
        };
    }

    inline TimeOfDayPreset NightPreset()
    {
        return {
            { -0.3f, 0.85f, 0.2f },
            0.30f,
            { 0.30f, 0.40f, 0.70f },
            0.2f,
            2.0f,
            1.0f,
            true
        };
    }

    inline TimeOfDayPreset Get(TimeOfDayState s)
    {
        return (s == TimeOfDayState::Day) ? DayPreset() : NightPreset();
    }
}
