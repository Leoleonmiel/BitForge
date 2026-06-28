module;
#include <string>
#include <fstream>
#include <sstream>
#include "../../thirdparty/json.hpp"

module bf.Preset.Serializer;

using json = nlohmann::json;

namespace bf::preset
{
    static json Float3ToJson(const Float3& value)
    {
        return json{ { "x", value.x }, { "y", value.y }, { "z", value.z } };
    }

    static Float3 Float3FromJson(const json& node, const Float3& fallback)
    {
        Float3 result = fallback;
        if (node.is_object())
        {
            result.x = node.value("x", fallback.x);
            result.y = node.value("y", fallback.y);
            result.z = node.value("z", fallback.z);
        }
        return result;
    }

    static std::string SerializeToJson(const ScenePreset& preset)
    {
        json root;

        root["camera"]["position"]    = Float3ToJson(preset.cameraPosition);
        root["camera"]["forward"]     = Float3ToJson(preset.cameraForward);
        root["camera"]["focalLength"] = preset.focalLength;
        root["camera"]["aperture"]    = preset.aperture;
        root["camera"]["iso"]         = preset.ISO;

        root["lighting"]["sunDirection"] = Float3ToJson(preset.sunDirection);
        root["lighting"]["sunColor"]     = Float3ToJson(preset.sunColor);
        root["lighting"]["sunIntensity"] = preset.sunIntensity;

        root["fog"]["density"] = preset.fogDensity;
        root["fog"]["height"]  = preset.fogHeight;

        root["postProcess"]["exposure"]       = preset.exposure;
        root["postProcess"]["bloomIntensity"] = preset.bloomIntensity;

        root["toggles"]["ssao"] = preset.enableSSAO;
        root["toggles"]["fog"]  = preset.enableFog;
        root["toggles"]["taa"]  = preset.enableTAA;

        return root.dump(4);
    }

    static bool DeserializeFromJson(const std::string& jsonText, ScenePreset& outPreset)
    {
        json root = json::parse(jsonText, nullptr, false);
        if (root.is_discarded()) { return false; }

        ScenePreset preset;

        const json camera = root.value("camera", json::object());
        preset.cameraPosition = Float3FromJson(camera.value("position", json::object()), preset.cameraPosition);
        preset.cameraForward  = Float3FromJson(camera.value("forward", json::object()), preset.cameraForward);
        preset.focalLength    = camera.value("focalLength", preset.focalLength);
        preset.aperture       = camera.value("aperture", preset.aperture);
        preset.ISO            = camera.value("iso", preset.ISO);

        const json lighting = root.value("lighting", json::object());
        preset.sunDirection = Float3FromJson(lighting.value("sunDirection", json::object()), preset.sunDirection);
        preset.sunColor     = Float3FromJson(lighting.value("sunColor", json::object()), preset.sunColor);
        preset.sunIntensity = lighting.value("sunIntensity", preset.sunIntensity);

        const json fog = root.value("fog", json::object());
        preset.fogDensity = fog.value("density", preset.fogDensity);
        preset.fogHeight  = fog.value("height", preset.fogHeight);

        const json postProcess = root.value("postProcess", json::object());
        preset.exposure       = postProcess.value("exposure", preset.exposure);
        preset.bloomIntensity = postProcess.value("bloomIntensity", preset.bloomIntensity);

        const json toggles = root.value("toggles", json::object());
        preset.enableSSAO = toggles.value("ssao", preset.enableSSAO);
        preset.enableFog  = toggles.value("fog", preset.enableFog);
        preset.enableTAA  = toggles.value("taa", preset.enableTAA);

        outPreset = preset;
        return true;
    }

    bool SavePresetToFile(const char* path, const ScenePreset& preset)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) { return false; }

        const std::string jsonText = SerializeToJson(preset);
        file.write(jsonText.data(), (std::streamsize)jsonText.size());
        return file.good();
    }

    bool LoadPresetFromFile(const char* path, ScenePreset& outPreset)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) { return false; }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return DeserializeFromJson(buffer.str(), outPreset);
    }
}
