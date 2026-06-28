#pragma once
#include "Vertex.h"
#include <vector>
#include <string>
#include <cstdint>

struct MeshData
{
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;
};

MeshData LoadOBJ(const std::string& path);
