#pragma once
#include "AssetTypes.h"
#include "../Cache/AssetCache.h"
#include "../Streaming/AssetStreamer.h"
#include <deque>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <string>

namespace Assets
{
    class AssetManager
    {
    public:
        using TextureUploader = std::function<uint32_t(const uint8_t*, uint32_t, uint32_t)>;

        void Initialize(unsigned workerThreads = 0);
        void Shutdown();

        void SetTextureUploader(TextureUploader up) { m_uploader = std::move(up); }
        void SetFallbackTexture(uint32_t srvIndex)  { m_fallbackSrv = srvIndex; }

        TextureHandle LoadTextureAsync(const std::string& absPath);
        ModelHandle   LoadModelAsync(const std::string& absPath);

        void Pump(uint32_t maxUploads = 16);
        void WaitAll();
        void WaitModel(ModelHandle h);

        AssetState      TextureState(TextureHandle h) const;
        uint32_t        TextureSrv(TextureHandle h) const;
        bool            ModelReady(ModelHandle h) const;
        const GLTFModel* GetModel(ModelHandle h) const;
        AssetStats      Stats() const;

    private:
        AssetStreamer m_streamer;
        TextureUploader m_uploader;
        uint32_t m_fallbackSrv = 0;

        mutable std::mutex   m_mutex;
        std::deque<TextureAsset> m_textures;
        std::deque<ModelAsset>   m_models;
        AssetCache m_textureCache;
        AssetCache m_modelCache;
        std::queue<uint32_t> m_decoded;

        std::atomic<int> m_inFlight{ 0 };
    };
}
