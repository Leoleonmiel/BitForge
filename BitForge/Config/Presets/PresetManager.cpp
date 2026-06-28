module;
#include <string>
#include <vector>
#include <cstring>
#include <system_error>
#include <filesystem>

module bf.Preset.Manager;

import bf.Preset.Serializer;

namespace fs = std::filesystem;

namespace bf::preset
{
    static std::string PathFor(const char* directory, const char* name)
    {
        return std::string(directory) + "/" + name + ".json";
    }

    static std::vector<std::string> CollectPresetNames(const char* directory)
    {
        std::vector<std::string> names;

        std::error_code ec;
        if (!fs::exists(directory, ec)) { return names; }

        for (const fs::directory_entry& entry : fs::directory_iterator(directory, ec))
        {
            if (!entry.is_regular_file()) { continue; }

            const fs::path& path = entry.path();
            if (path.extension() == ".json")
            {
                names.push_back(path.stem().string());
            }
        }
        return names;
    }

    PresetManager::PresetManager(const char* directory)
    {
        m_directory[0] = '\0';
        if (directory)
        {
            const std::string source(directory);
            const size_t count = (source.size() < sizeof(m_directory) - 1) ? source.size() : sizeof(m_directory) - 1;
            std::memcpy(m_directory, source.data(), count);
            m_directory[count] = '\0';
        }
    }

    bool PresetManager::Save(const char* name, const ScenePreset& preset) const
    {
        std::error_code ec;
        fs::create_directories(m_directory, ec);
        return SavePresetToFile(PathFor(m_directory, name).c_str(), preset);
    }

    bool PresetManager::Load(const char* name, ScenePreset& outPreset) const
    {
        return LoadPresetFromFile(PathFor(m_directory, name).c_str(), outPreset);
    }

    int PresetManager::Count() const
    {
        return (int)CollectPresetNames(m_directory).size();
    }

    bool PresetManager::NameAt(int index, char* outBuffer, int bufferSize) const
    {
        if (!outBuffer || bufferSize <= 0) { return false; }

        std::vector<std::string> names = CollectPresetNames(m_directory);
        if (index < 0 || index >= (int)names.size()) { return false; }

        const std::string& name = names[index];
        const size_t count = (name.size() < (size_t)bufferSize - 1) ? name.size() : (size_t)bufferSize - 1;
        std::memcpy(outBuffer, name.data(), count);
        outBuffer[count] = '\0';
        return true;
    }
}
