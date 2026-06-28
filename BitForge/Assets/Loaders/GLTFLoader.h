#pragma once
#include "Vertex.h"
#include <vector>
#include <string>
#include <cstdint>

struct GLTFPrimitive
{
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    std::string           texturePath;
    float metallicFactor  = 0.0f;
    float roughnessFactor = 0.8f;
};

struct GLTFModel
{
    std::vector<GLTFPrimitive> primitives;
};

GLTFModel LoadGLTF(const std::string& path);
