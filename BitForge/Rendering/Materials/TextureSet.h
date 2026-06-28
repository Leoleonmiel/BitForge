#pragma once
#include <cstdint>

struct TextureSet
{
    static constexpr uint32_t kNone = 0xFFFFFFFFu;

    uint32_t albedo    = kNone;
    uint32_t normal    = kNone;
    uint32_t metallic  = kNone;
    uint32_t roughness = kNone;
    uint32_t ao        = kNone;
};
