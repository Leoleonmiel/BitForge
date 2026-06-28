module;
class Dx12Renderer;

export module bf.Preset.Binding;

import bf.Preset.Types;

export namespace bf::preset
{
    ScenePreset CaptureScenePreset(const Dx12Renderer& renderer);
    void        ApplyScenePreset(Dx12Renderer& renderer, const ScenePreset& preset);
}
