module;
#include <cstring>
#include "Dx12Renderer.h"
#include "Input/Input.h"
#include "thirdparty/imgui/imgui.h"

module bf.Preset.Session;

import bf.Preset.Types;
import bf.Preset.Binding;

namespace bf::preset
{
    PresetSession::PresetSession(Dx12Renderer& renderer)
        : m_renderer(renderer),
          m_manager("presets"),
          m_selected(-1),
          m_active(0),
          m_savePrev(false),
          m_loadPrev(false),
          m_cyclePrev(false)
    {
        const char defaultName[] = "my_preset";
        std::memcpy(m_newName, defaultName, sizeof(defaultName));

        ScenePreset preset;
        if (m_manager.Load("default", preset))
        {
            ApplyScenePreset(m_renderer, preset);
        }
        else
        {
            m_manager.Save("default", CaptureScenePreset(m_renderer));
        }
    }

    void PresetSession::DrawPanel()
    {
        if (ImGui::Begin("Presets"))
        {
            const int presetCount = m_manager.Count();
            ImGui::Text("Saved presets: %d", presetCount);
            ImGui::TextDisabled("F5 quicksave  F6 quickload  F7 cycle");
            ImGui::Separator();

            if (ImGui::BeginListBox("##presetlist", ImVec2(0.0f, 140.0f)))
            {
                for (int i = 0; i < presetCount; ++i)
                {
                    char presetName[260];
                    if (!m_manager.NameAt(i, presetName, (int)sizeof(presetName))) { continue; }

                    const bool isSelected = (m_selected == i);
                    if (ImGui::Selectable(presetName, isSelected)) { m_selected = i; }
                    if (isSelected) { ImGui::SetItemDefaultFocus(); }
                }
                ImGui::EndListBox();
            }

            const bool hasSelection = (m_selected >= 0 && m_selected < presetCount);

            if (ImGui::Button("Previous") && presetCount > 0)
            {
                m_selected = (m_selected <= 0) ? presetCount - 1 : m_selected - 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Next") && presetCount > 0)
            {
                m_selected = (m_selected + 1) % presetCount;
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Selected") && hasSelection)
            {
                char presetName[260];
                ScenePreset preset;
                if (m_manager.NameAt(m_selected, presetName, (int)sizeof(presetName)) &&
                    m_manager.Load(presetName, preset))
                {
                    ApplyScenePreset(m_renderer, preset);
                }
            }

            ImGui::Separator();
            ImGui::InputText("Name", m_newName, sizeof(m_newName));
            if (ImGui::Button("Save Current"))
            {
                m_manager.Save(m_newName, CaptureScenePreset(m_renderer));
            }
        }
        ImGui::End();
    }

    void PresetSession::HandleHotkeys()
    {
        const bool saveKey  = Input::IsKeyDown(VK_F5);
        const bool loadKey  = Input::IsKeyDown(VK_F6);
        const bool cycleKey = Input::IsKeyDown(VK_F7);

        if (saveKey && !m_savePrev)
        {
            m_manager.Save("quicksave", CaptureScenePreset(m_renderer));
        }
        if (loadKey && !m_loadPrev)
        {
            ScenePreset preset;
            if (m_manager.Load("quicksave", preset)) { ApplyScenePreset(m_renderer, preset); }
        }
        if (cycleKey && !m_cyclePrev)
        {
            const int presetCount = m_manager.Count();
            if (presetCount > 0)
            {
                m_active = (m_active + 1) % presetCount;
                char presetName[260];
                ScenePreset preset;
                if (m_manager.NameAt(m_active, presetName, (int)sizeof(presetName)) &&
                    m_manager.Load(presetName, preset))
                {
                    ApplyScenePreset(m_renderer, preset);
                }
            }
        }

        m_savePrev  = saveKey;
        m_loadPrev  = loadKey;
        m_cyclePrev = cycleKey;
    }
}
