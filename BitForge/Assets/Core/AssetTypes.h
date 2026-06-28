#pragma once
#include "AssetHandle.h"
#include "GLTFLoader.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Assets
{
    struct TextureAsset
    {
        std::string          path;
        AssetState           state = AssetState::Unloaded;
        uint32_t             srvIndex = 0;
        int                  width = 0, height = 0;
        std::vector<uint8_t> pixels;
    };

    struct ModelAsset
    {
        std::string path;
        AssetState  state = AssetState::Unloaded;
        GLTFModel   model;
    };

    struct AssetStats
    {
        uint32_t textures = 0, models = 0;
        uint32_t loading = 0, loaded = 0, failed = 0;
        uint32_t queued = 0;
        size_t   cpuBytesInFlight = 0;
        uint32_t gpuTextures = 0;
    };
}
