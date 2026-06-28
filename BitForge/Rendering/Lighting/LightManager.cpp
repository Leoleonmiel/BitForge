#include "LightManager.h"


int LightManager::AddLight(const Light& light)
{
    if ((int)m_lights.size() >= kMaxLights) { return -1; }
    m_lights.push_back(light);
    return (int)m_lights.size() - 1;
}

void LightManager::RemoveLight(int index)
{
    if (index >= 0 && index < (int)m_lights.size())
    {
        m_lights.erase(m_lights.begin() + index);
    }
}
