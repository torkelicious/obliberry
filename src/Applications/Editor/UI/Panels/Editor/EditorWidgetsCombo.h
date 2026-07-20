#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Core/ResourceManager.h"
#include <imgui.h>

namespace Rendering {
    class Texture;
    class Shader;
    class Mesh;
    class Material;
} // namespace Rendering

namespace UI {
    class Font;
}

namespace Editor::UI {
    template <typename T, typename GetPreviewText> bool AssetComboImpl(const char *label, Core::ResourceManager &resources, std::shared_ptr<T> &current, GetPreviewText &&getPreview) {
        const auto &all = resources.GetAll<T>();

        // build items and find current index
        int currentIdx = 0; // none
        std::vector<std::string> keys;
        keys.emplace_back("None");
        int idx = 1;
        for (const auto &[key, ptr] : all) {
            keys.push_back(key);
            if (ptr == current)
                currentIdx = idx;
            idx++;
        }

        std::string preview = "None";
        if (current) {
            for (const auto &[key, ptr] : all) {
                if (ptr == current) {
                    preview = key;
                    break;
                }
            }
        }

        bool changed = false;
        if (ImGui::BeginCombo(label, preview.c_str())) {
            ImGui::PushID("__none__");
            bool isSelected = currentIdx == 0;
            if (ImGui::Selectable("None", &isSelected)) {
                current.reset();
                changed = true;
            }
            if (currentIdx == 0)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();

            // Assets
            idx = 1;
            for (const auto &[key, ptr] : all) {
                bool isEngineBuiltin = !key.empty() && key.front() == '[';
                if (isEngineBuiltin)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.0f)); // subtle blue for engine builtins
                ImGui::PushID(key.c_str());
                isSelected = currentIdx == idx;
                if (ImGui::Selectable(key.c_str(), &isSelected)) {
                    current = ptr;
                    changed = true;
                }
                if (currentIdx == idx)
                    ImGui::SetItemDefaultFocus();
                ImGui::PopID();
                if (isEngineBuiltin)
                    ImGui::PopStyleColor();
                idx++;
            }
            ImGui::EndCombo();
        }

        if (current) {
            getPreview(current);
        }

        return changed;
    }

    // typed combos

    bool TextureCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Texture> &current);

    bool ShaderCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Shader> &current);

    bool MeshCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Mesh> &current);

    bool MaterialCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Material> &current);

    bool FontCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<::UI::Font> &current);

    bool FileCombo(const char *label, const std::string &subDir, const std::string &extension, std::string &current);

} // namespace Editor::UI
