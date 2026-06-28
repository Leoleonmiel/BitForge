#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Assets
{
    class AssetCache
    {
    public:
        static constexpr uint32_t kMiss = 0xFFFFFFFFu;

        uint32_t Find(const std::string& path) const
        {
            auto it = m_map.find(path);
            return (it == m_map.end()) ? kMiss : it->second;
        }
        void Insert(const std::string& path, uint32_t id) { m_map[path] = id; }
        size_t Count() const { return m_map.size(); }

    private:
        std::unordered_map<std::string, uint32_t> m_map;
    };
}
