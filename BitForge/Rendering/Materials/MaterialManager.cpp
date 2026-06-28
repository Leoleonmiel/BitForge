#include "MaterialManager.h"

uint32_t MaterialManager::AddMaterial(const Material& mat)
{
    m_materials.push_back(mat);
    return (uint32_t)m_materials.size() - 1;
}
