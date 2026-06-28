#pragma once
#include <cstdint>


namespace Assets
{
    enum class AssetState : uint8_t
    {
        Unloaded,
        Loading,
        Decoded,
        Loaded,
        Failed
    };

    template <typename Tag>
    struct AssetHandle
    {
        static constexpr uint32_t kInvalid = 0xFFFFFFFFu;
        uint32_t id = kInvalid;
        bool Valid() const { return id != kInvalid; }
    };

    struct TextureTag;
    struct ModelTag;
    using TextureHandle = AssetHandle<TextureTag>;
    using ModelHandle   = AssetHandle<ModelTag>;
}
