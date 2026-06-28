#pragma once
#include <cstdint>

struct InstanceData
{
    float    world[16];
    uint32_t texIndex;
    float    metallic;
    float    roughness;
    float    ao;
};

struct IndirectDrawCommand
{
    uint32_t instanceIndex;
    uint32_t indexCountPerInstance;
    uint32_t instanceCount;
    uint32_t startIndexLocation;
    int32_t  baseVertexLocation;
    uint32_t startInstanceLocation;
};
