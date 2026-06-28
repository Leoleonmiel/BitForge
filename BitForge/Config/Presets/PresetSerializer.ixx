export module bf.Preset.Serializer;

import bf.Preset.Types;

export namespace bf::preset
{
    bool SavePresetToFile(const char* path, const ScenePreset& preset);
    bool LoadPresetFromFile(const char* path, ScenePreset& outPreset);
}
