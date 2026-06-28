#pragma once
#include "Light.h"
#include <vector>

class LightManager
{
public:
    static constexpr int kMaxLights = 128;

    int  AddLight(const Light& light);
    void RemoveLight(int index);
    Light&       GetLight(int i)       { return m_lights[i]; }
    const Light& GetLight(int i) const { return m_lights[i]; }
    const std::vector<Light>& GetLights() const { return m_lights; }
    int  Count() const { return (int)m_lights.size(); }
    void Clear() { m_lights.clear(); }

private:
    std::vector<Light> m_lights;
};
