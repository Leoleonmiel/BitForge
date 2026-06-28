#include "AssetManager.h"
#include "../Loaders/TextureLoader.h"
#include "../Loaders/GltfModelLoader.h"
#include <chrono>
#include <thread>
#include <windows.h>

namespace Assets
{
    void AssetManager::Initialize(unsigned workerThreads)
    {
        m_streamer.Start(workerThreads);
    }

    void AssetManager::Shutdown()
    {
        m_streamer.Stop();
    }

    TextureHandle AssetManager::LoadTextureAsync(const std::string& absPath)
    {
        uint32_t id;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            uint32_t cached = m_textureCache.Find(absPath);
            if (cached != AssetCache::kMiss) { return TextureHandle{ cached }; }

            id = (uint32_t)m_textures.size();
            m_textures.emplace_back();
            m_textures.back().path  = absPath;
            m_textures.back().state = AssetState::Loading;
            m_textureCache.Insert(absPath, id);
        }
        m_inFlight.fetch_add(1);

        m_streamer.Enqueue([this, id, absPath]
        {
            DecodedImage img = DecodeImageFile(absPath);
            std::lock_guard<std::mutex> lk(m_mutex);
            TextureAsset& asset = m_textures[id];
            if (img.ok)
            {
                asset.width  = img.width;
                asset.height = img.height;
                asset.pixels = std::move(img.pixels);
                asset.state  = AssetState::Decoded;
                m_decoded.push(id);
            }
            else
            {
                asset.state = AssetState::Failed;
                OutputDebugStringA(("[Assets] FAILED to decode: " + absPath + "\n").c_str());
                m_inFlight.fetch_sub(1);
            }
        });

        return TextureHandle{ id };
    }

    ModelHandle AssetManager::LoadModelAsync(const std::string& absPath)
    {
        uint32_t id;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            uint32_t cached = m_modelCache.Find(absPath);
            if (cached != AssetCache::kMiss) { return ModelHandle{ cached }; }

            id = (uint32_t)m_models.size();
            m_models.emplace_back();
            m_models.back().path  = absPath;
            m_models.back().state = AssetState::Loading;
            m_modelCache.Insert(absPath, id);
        }
        m_inFlight.fetch_add(1);

        m_streamer.Enqueue([this, id, absPath]
        {
            GLTFModel parsed = ParseGltfFile(absPath);
            const bool ok = !parsed.primitives.empty();
            std::lock_guard<std::mutex> lk(m_mutex);
            ModelAsset& asset = m_models[id];
            asset.model = std::move(parsed);
            asset.state = ok ? AssetState::Loaded : AssetState::Failed;
            if (!ok)
            {
                OutputDebugStringA(("[Assets] FAILED to parse: " + absPath + "\n").c_str());
            }
            m_inFlight.fetch_sub(1);
        });

        return ModelHandle{ id };
    }

    void AssetManager::Pump(uint32_t maxUploads)
    {
        if (!m_uploader) { return; }

        for (uint32_t uploadIndex = 0; uploadIndex < maxUploads; uploadIndex++)
        {
            uint32_t id;
            std::vector<uint8_t> pixels;
            int width, height;
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                if (m_decoded.empty()) { break; }
                id = m_decoded.front();
                m_decoded.pop();
                TextureAsset& asset = m_textures[id];
                pixels = std::move(asset.pixels);
                width = asset.width; height = asset.height;
            }

            uint32_t srv = m_uploader(pixels.data(), (uint32_t)width, (uint32_t)height);

            {
                std::lock_guard<std::mutex> lk(m_mutex);
                TextureAsset& asset = m_textures[id];
                asset.srvIndex = srv;
                asset.state    = AssetState::Loaded;
                asset.pixels.clear();
                asset.pixels.shrink_to_fit();
            }
            m_inFlight.fetch_sub(1);
        }
    }

    void AssetManager::WaitAll()
    {
        while (m_inFlight.load() > 0)
        {
            Pump(64);
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
        Pump(64);
    }

    void AssetManager::WaitModel(ModelHandle h)
    {
        if (!h.Valid()) { return; }
        for (;;)
        {
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                if (h.id >= m_models.size()) { return; }
                AssetState state = m_models[h.id].state;
                if (state == AssetState::Loaded || state == AssetState::Failed) { return; }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }

    AssetState AssetManager::TextureState(TextureHandle h) const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (!h.Valid() || h.id >= m_textures.size()) { return AssetState::Unloaded; }
        return m_textures[h.id].state;
    }

    uint32_t AssetManager::TextureSrv(TextureHandle h) const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (h.Valid() && h.id < m_textures.size() &&
            m_textures[h.id].state == AssetState::Loaded) { return m_textures[h.id].srvIndex; }
        return m_fallbackSrv;
    }

    bool AssetManager::ModelReady(ModelHandle h) const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return h.Valid() && h.id < m_models.size() &&
               m_models[h.id].state == AssetState::Loaded;
    }

    const GLTFModel* AssetManager::GetModel(ModelHandle h) const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (h.Valid() && h.id < m_models.size() &&
            m_models[h.id].state == AssetState::Loaded) { return &m_models[h.id].model; }
        return nullptr;
    }

    AssetStats AssetManager::Stats() const
    {
        AssetStats stats;
        std::lock_guard<std::mutex> lk(m_mutex);
        stats.textures = (uint32_t)m_textures.size();
        stats.models   = (uint32_t)m_models.size();
        for (const TextureAsset& asset : m_textures)
        {
            switch (asset.state)
            {
            case AssetState::Loading:
            case AssetState::Decoded: stats.loading++; break;
            case AssetState::Loaded:  stats.loaded++; stats.gpuTextures++; break;
            case AssetState::Failed:  stats.failed++; break;
            default: break;
            }
            stats.cpuBytesInFlight += asset.pixels.size();
        }
        for (const ModelAsset& asset : m_models)
        {
            if (asset.state == AssetState::Loading) { stats.loading++; }
            else if (asset.state == AssetState::Loaded) { stats.loaded++; }
            else if (asset.state == AssetState::Failed) { stats.failed++; }
        }
        stats.queued = (uint32_t)m_streamer.Queued();
        return stats;
    }
}
