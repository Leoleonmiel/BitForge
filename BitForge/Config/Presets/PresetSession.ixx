module;
class Dx12Renderer;

export module bf.Preset.Session;

import bf.Preset.Manager;

export namespace bf::preset
{
    class PresetSession
    {
    public:
        explicit PresetSession(Dx12Renderer& renderer);

        void DrawPanel();
        void HandleHotkeys();

    private:
        Dx12Renderer& m_renderer;
        PresetManager m_manager;
        char m_newName[128];
        int  m_selected;
        int  m_active;
        bool m_savePrev;
        bool m_loadPrev;
        bool m_cyclePrev;
    };
}
