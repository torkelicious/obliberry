#include "EditorWidgetsCombo.h"

#include <filesystem>

#include "IO/VFS/VFS.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "UI/Text/Font.h"

namespace Editor::UI {

    bool TextureCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Texture> &current) {
        return AssetComboImpl(label, resources, current, [](const std::shared_ptr<Rendering::Texture> &tex) { ImGui::TextDisabled("%s", tex->GetPath().c_str()); });
    }

    bool ShaderCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Shader> &current) {
        return AssetComboImpl(label, resources, current,
                              [](const std::shared_ptr<Rendering::Shader> &shader) { ImGui::TextDisabled("Vert: %s  Frag: %s", shader->GetVertexPath().c_str(), shader->GetFragmentPath().c_str()); });
    }

    bool MeshCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Mesh> &current) {
        return AssetComboImpl(label, resources, current, [](const std::shared_ptr<Rendering::Mesh> &mesh) { ImGui::TextDisabled("%s, %u indices", mesh->GetFactoryId().c_str(), mesh->GetIndexCount()); });
    }

    bool MaterialCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<Rendering::Material> &current) {
        return AssetComboImpl(label, resources, current, [&resources](const std::shared_ptr<Rendering::Material> &mat) {
            const std::string texKey = mat->texture ? resources.GetKey(mat->texture) : "none";
            const std::string shaderKey = mat->shader ? resources.GetKey(mat->shader) : "none";
            ImGui::TextDisabled("Texture: %s  Shader: %s", texKey.c_str(), shaderKey.c_str());
        });
    }

    bool FontCombo(const char *label, Core::ResourceManager &resources, std::shared_ptr<::UI::Font> &current) {
        const bool changed = AssetComboImpl(label, resources, current, [](const std::shared_ptr<::UI::Font> &font) { ImGui::TextDisabled("%upx%s", font->GetFontSize(), font->IsSDF() ? " SDF" : ""); });
        if (!current) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("No font selected, text will not render!");
            ImGui::PopStyleColor();
        }
        return changed;
    }

    bool FileCombo(const char *label, const std::string &subDir, const std::string &extension, std::string &current) {
        const auto resolved = IO::VFS::Resolve(subDir);
        std::vector<std::string> files;
        files.emplace_back("None");
        if (std::filesystem::exists(resolved)) {
            for (const auto &entry : std::filesystem::directory_iterator(resolved)) {
                if (entry.is_regular_file() && entry.path().extension() == extension) {
                    auto relPath = std::filesystem::relative(entry.path(), IO::VFS::GetProjectRoot());
                    std::string relStr = relPath.string();
                    for (auto &c : relStr)
                        if (c == '\\')
                            c = '/';
                    files.push_back(std::move(relStr));
                }
            }
        }

        int currentIdx = 0;
        for (int i = 1; i < static_cast<int>(files.size()); ++i) {
            if (files[i] == current) {
                currentIdx = i;
                break;
            }
        }

        const std::string preview = current.empty() ? "None" : current;
        bool changed = false;

        if (ImGui::BeginCombo(label, preview.c_str())) {
            for (int i = 0; i < static_cast<int>(files.size()); ++i) {
                const bool isSelected = currentIdx == i;
                if (ImGui::Selectable(files[i].c_str(), isSelected)) {
                    if (i == 0)
                        current.clear();
                    else
                        current = files[i];
                    changed = true;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return changed;
    }

} // namespace Editor::UI
