export module bf.Preset.Manager;

import bf.Preset.Types;

export namespace bf::preset
{
    class PresetManager
    {
    public:
        explicit PresetManager(const char* directory = "presets");

        bool Save(const char* name, const ScenePreset& preset) const;
        bool Load(const char* name, ScenePreset& outPreset) const;

        int  Count() const;
        bool NameAt(int index, char* outBuffer, int bufferSize) const;

        const char* Directory() const { return m_directory; }

    private:
        char m_directory[260];
    };
}
