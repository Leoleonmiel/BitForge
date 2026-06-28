module;
#include "Dx12Renderer.h"
#include "Rendering/Lighting/LightingTypes.h"

module bf.Preset.Binding;

namespace bf::preset
{
    ScenePreset CaptureScenePreset(const Dx12Renderer& renderer)
    {
        ScenePreset preset;

        const CinematicCamera& camera = renderer.Camera();
        const DirectX::XMFLOAT3 position = camera.GetPosition();
        const DirectX::XMFLOAT3 forward  = camera.GetForward();
        preset.cameraPosition = { position.x, position.y, position.z };
        preset.cameraForward  = { forward.x, forward.y, forward.z };
        preset.focalLength = camera.focalLength;
        preset.aperture    = camera.aperture;
        preset.ISO         = camera.ISO;
        preset.exposure    = camera.exposureComp;

        const LightManager& lights = renderer.Lights();
        for (int i = 0; i < lights.Count(); ++i)
        {
            const Light& light = lights.GetLight(i);
            if (light.type == (int)LightType::Directional)
            {
                preset.sunDirection = { light.direction.x, light.direction.y, light.direction.z };
                preset.sunColor     = { light.color.x, light.color.y, light.color.z };
                preset.sunIntensity = light.intensity;
                break;
            }
        }

        preset.fogDensity = renderer.Fog().density;
        preset.fogHeight  = renderer.Fog().heightFactor;

        preset.enableSSAO = renderer.Ssao().enabled;
        preset.enableFog  = renderer.Fog().enabled;
        preset.enableTAA  = renderer.Taa().enabled;

        return preset;
    }

    void ApplyScenePreset(Dx12Renderer& renderer, const ScenePreset& preset)
    {
        CinematicCamera& camera = renderer.Camera();
        camera.SetPosition(preset.cameraPosition.x, preset.cameraPosition.y, preset.cameraPosition.z);
        camera.LookAt(preset.cameraPosition.x + preset.cameraForward.x,
                      preset.cameraPosition.y + preset.cameraForward.y,
                      preset.cameraPosition.z + preset.cameraForward.z);
        camera.focalLength  = preset.focalLength;
        camera.aperture     = preset.aperture;
        camera.ISO          = preset.ISO;
        camera.exposureComp = preset.exposure;

        LightManager& lights = renderer.Lights();
        for (int i = 0; i < lights.Count(); ++i)
        {
            Light& light = lights.GetLight(i);
            if (light.type == (int)LightType::Directional)
            {
                light.direction = { preset.sunDirection.x, preset.sunDirection.y, preset.sunDirection.z };
                light.color     = { preset.sunColor.x, preset.sunColor.y, preset.sunColor.z };
                light.intensity = preset.sunIntensity;
                break;
            }
        }

        renderer.Fog().density      = preset.fogDensity;
        renderer.Fog().heightFactor = preset.fogHeight;
        renderer.Fog().enabled      = preset.enableFog;
        renderer.Ssao().enabled     = preset.enableSSAO;
        renderer.Taa().enabled      = preset.enableTAA;
    }
}
