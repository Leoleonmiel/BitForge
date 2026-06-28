#include "Dx12Renderer.h"

void Dx12Renderer::ApplyTimeOfDay()
{
    const TimeOfDayPreset preset = TimeOfDay::Get(m_timeOfDay);

    for (int i = 0; i < m_lightManager.Count(); ++i)
    {
        Light& lt = m_lightManager.GetLight(i);
        if (lt.type == (int)LightType::Directional)
        {
            lt.direction = preset.sunDirection;
            lt.color     = preset.sunColor;
            lt.intensity = preset.sunIntensity;
            lt.enabled   = true;
            break;
        }
    }

    for (int i = 0; i < m_lightManager.Count(); ++i)
    {
        Light& lt = m_lightManager.GetLight(i);
        if (lt.type != (int)LightType::Directional) { lt.enabled = preset.pointLightsOn; }
    }

    m_iblIntensity = preset.iblIntensity;
    m_camera.exposureComp = preset.exposure;
    m_nightFactor  = preset.nightFactor;
}
