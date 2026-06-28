#include "Dx12Renderer.h"

using namespace DirectX;

void Dx12Renderer::SetupSceneLighting()
{
    m_lightManager.Clear();

    const XMFLOAT3 sceneCenter = m_sceneCenter;
    const float    sceneRadius = (m_sceneRadius > 1.0f) ? m_sceneRadius : 50.0f;

    {
        Light sun;
        sun.type      = (int)LightType::Directional;
        sun.direction = { 0.5f, 1.0f, 0.4f };
        sun.color     = { 1.00f, 0.95f, 0.90f };
        sun.intensity = 3.0f;
        m_lightManager.AddLight(sun);
    }

    {
        Light fill;
        fill.type      = (int)LightType::Point;
        fill.position  = { sceneCenter.x, sceneCenter.y + sceneRadius * 0.5f, sceneCenter.z };
        fill.radius    = sceneRadius * 1.4f;
        fill.color     = { 0.55f, 0.65f, 0.85f };
        fill.intensity = 0.6f;
        m_lightManager.AddLight(fill);
    }

    {
        Light bounce;
        bounce.type      = (int)LightType::Point;
        bounce.position  = { sceneCenter.x, sceneCenter.y - sceneRadius * 0.2f, sceneCenter.z };
        bounce.radius    = sceneRadius * 1.2f;
        bounce.color     = { 0.85f, 0.70f, 0.55f };
        bounce.intensity = 0.35f;
        m_lightManager.AddLight(bounce);
    }
}
