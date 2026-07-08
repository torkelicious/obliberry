// ReSharper disable CppDFAUnreachableCode
#include "TileEditorPanel.h"

#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "Editor/UI/Panels/Editor/EditorWidgetsCombo.h"

#include <imgui.h>
#include <sstream>
#include <utility>

namespace Editor::UI {
    namespace {

        std::unordered_map<uint8_t, size_t> CountTilesPerType(const ECS::Components::MapComponent &mapComp) {
            std::unordered_map<uint8_t, size_t> counts;
            for (const auto &[coords, tile] : mapComp.grid.tiles) {
                (void)coords;
                counts[tile.type]++;
            }
            return counts;
        }

        bool FindTypeIdForMaterial(const ECS::Components::MapComponent &mapComp,
                                   const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color,
                                   uint8_t &outId) {
            for (const auto &[id, mat] : mapComp.typeMats) {
                if (mat.texture == tex && mat.color == color) {
                    outId = id;
                    return true;
                }
            }
            return false;
        }

        std::string FormatTypeLabel(const uint8_t id, const Rendering::Material &mat, Core::ResourceManager &resources,
                                    const size_t tileCount) {
            std::string texName = mat.texture ? resources.GetKey(mat.texture) : "none";
            if (texName.empty())
                texName = "none";

            std::ostringstream oss;
            oss << "Type " << static_cast<int>(id) << ": " << texName << " (" << tileCount << " tile";
            if (tileCount != 1)
                oss << "s";
            oss << ")";
            return oss.str();
        }

        bool TypeCombo(const char *label, const ECS::Components::MapComponent &mapComp,
                       Core::ResourceManager &resources, uint8_t &current) {
            const auto &typeMats = mapComp.typeMats;
            const auto counts = CountTilesPerType(mapComp);

            std::string preview = "Unknown type";
            if (const auto it = typeMats.find(current); it != typeMats.end()) {
                const auto cit = counts.find(current);
                const size_t count = cit != counts.end() ? cit->second : 0;
                preview = FormatTypeLabel(current, it->second, resources, count);
            }

            bool changed = false;
            if (ImGui::BeginCombo(label, preview.c_str())) {
                for (const auto &[id, mat] : typeMats) {
                    ImGui::PushID(static_cast<int>(id));
                    const bool isSelected = (id == current);
                    const auto cit = counts.find(id);
                    const size_t count = cit != counts.end() ? cit->second : 0;
                    const std::string labelStr = FormatTypeLabel(id, mat, resources, count);

                    if (const ImVec4 swatchCol(mat.color.r, mat.color.g, mat.color.b, mat.color.a); ImGui::ColorButton(
                                "##swatch", swatchCol, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs,
                                ImVec2(16, 16))) {
                        current = id;
                        changed = true;
                    }
                    ImGui::SameLine();

                    if (ImGui::Selectable(labelStr.c_str(), isSelected)) {
                        current = id;
                        changed = true;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            return changed;
        }

    } // namespace

    void TileEditorPanel::SetSelectedTile(Map::Tile *tile) {
        m_CurrentTile = tile;
        m_EditInitialized = false;
    }

    void TileEditorPanel::OnImGuiRender() {
        ImGui::Begin("Tile Editor");
        m_IsHovered = ImGui::IsWindowHovered();

        if (!m_CurrentTile || !m_MapComp || !m_EngineContext || !m_EngineContext->resources) {
            ImGui::TextDisabled("No tile selected.");
            ImGui::End();
            return;
        }

        auto &typeMats = m_MapComp->typeMats;
        const auto typeIt = typeMats.find(m_CurrentTile->type);
        if (typeIt == typeMats.end()) {
            ImGui::TextDisabled("Tile has an unknown type id (%u).", static_cast<unsigned>(m_CurrentTile->type));
            ImGui::End();
            return;
        }

        auto &currentMat = typeIt->second;

        if (!m_EditInitialized || m_EditSourceType != m_CurrentTile->type) {
            m_EditTexture = currentMat.texture;
            m_EditColor = currentMat.color;
            m_EditSourceType = m_CurrentTile->type;
            m_EditInitialized = true;
        }

        const auto counts = CountTilesPerType(*m_MapComp);
        const auto countIt = counts.find(m_CurrentTile->type);
        const size_t currentTypeCount = countIt != counts.end() ? countIt->second : 0;

        ImGui::PushID("TileType");
        if (uint8_t selectedType = m_CurrentTile->type;
            TypeCombo("Tile Type", *m_MapComp, *m_EngineContext->resources, selectedType)) {
            m_CurrentTile->type = selectedType;
            m_MapComp->needsMeshUpdate = true;
        }
        ImGui::PopID();

        ImGui::SeparatorText("Type Material");

        ImGui::PushID("EditTexture");
        TextureCombo("Texture", *m_EngineContext->resources, m_EditTexture);
        ImGui::PopID();

        ImGui::PushID("EditColor");
        ImGui::ColorEdit4("Color", &m_EditColor.x, ImGuiColorEditFlags_NoInputs);
        ImGui::PopID();

        const bool materialChanged = (m_EditTexture != currentMat.texture) || (m_EditColor != currentMat.color);

        if (!materialChanged) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Override Current Type")) {
            currentMat.texture = m_EditTexture;
            currentMat.color = m_EditColor;
            m_EditSourceType = m_CurrentTile->type; // type stays, material now matches
            m_MapComp->needsMeshUpdate = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && materialChanged) {
            std::ostringstream tip;
            tip << "Replaces the material of type " << static_cast<int>(m_CurrentTile->type) << " for all "
                << currentTypeCount << " tile" << (currentTypeCount != 1 ? "s" : "") << " using it.";
            ImGui::SetTooltip("%s", tip.str().c_str());
        }

        ImGui::SameLine();

        if (ImGui::Button("Save as New Type")) {
            if (m_CreateTypeFn) {
                m_CurrentTile->type = m_CreateTypeFn(m_EditTexture, m_EditColor);
                m_MapComp->needsMeshUpdate = true;
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Creates a fresh type id with this material and assigns only the selected tile to it.");
        }

        if (!materialChanged) {
            ImGui::EndDisabled();
        }

        if (materialChanged) {
            if (uint8_t existingId = 0; FindTypeIdForMaterial(*m_MapComp, m_EditTexture, m_EditColor, existingId)) {
                if (existingId != m_CurrentTile->type) {
                    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f),
                                       "Same material as type %u. Use 'Override Current Type' "
                                       "to share it, or change texture/color.",
                                       static_cast<unsigned>(existingId));
                }
            }
        }

        if (ImGui::Checkbox("Is Walkable", &m_CurrentTile->walkable)) {
            m_MapComp->needsMeshUpdate = true;
        }

        ImGui::End();
    }
} // namespace Editor::UI
