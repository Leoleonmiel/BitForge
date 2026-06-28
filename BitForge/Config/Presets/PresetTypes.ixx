export module bf.Preset.Types;

export namespace bf::preset
{
    struct Float3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct ScenePreset
    {
        Float3 cameraPosition{ 0.0f, 0.0f, 0.0f };
        Float3 cameraForward{ 0.0f, 0.0f, 1.0f };
        float  focalLength = 35.0f;
        float  aperture    = 2.8f;
        float  ISO         = 100.0f;

        Float3 sunDirection{ 0.5f, 1.0f, 0.4f };
        Float3 sunColor{ 1.0f, 0.95f, 0.9f };
        float  sunIntensity = 3.0f;

        float  fogDensity = 0.06f;
        float  fogHeight  = 1.0f;

        float  exposure       = 1.2f;
        float  bloomIntensity = 0.0f;

        bool   enableSSAO = true;
        bool   enableFog  = true;
        bool   enableTAA  = true;
    };
}
