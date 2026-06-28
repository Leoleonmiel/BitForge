#pragma once
#include "Material.h"
#include <vector>
#include <cstdint>

class MaterialManager
{
public:
    uint32_t AddMaterial(const Material& mat);
    Material&       Get(uint32_t id)       { return m_materials[id]; }
    const Material& Get(uint32_t id) const { return m_materials[id]; }
    uint32_t Count() const { return (uint32_t)m_materials.size(); }

private:
    std::vector<Material> m_materials;
};
