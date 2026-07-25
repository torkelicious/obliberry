// ReSharper disable CppDFAUnreachableCode
#include "TileEditorPanel.h"
#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgetsCombo.h"
#include "Core/Utils/UiUtils.h"
#include <algorithm>
#include <imgui.h>
#include <sstream>
#include <utility>


// todo: fix map detection

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

        bool FindTypeIdForMaterial(const ECS::Components::MapComponent &mapComp, const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color, uint8_t &outId) {
            for (const auto &[id, mat] : mapComp.typeMats) {
                if (mat->texture == tex && mat->color == color) {
                    outId = id;
                    return true;
                }
            }
            return false;
        }

        auto FindTypeMat(auto &typeMats, uint8_t id) {
            return std::find_if(typeMats.begin(), typeMats.end(), [id](const auto &p) { return p.first == id; });
        }

        std::string FormatTypeLabel(const uint8_t id, const Rendering::Material &mat, Core::ResourceManager &resources, const size_t tileCount) {
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

        bool TypeCombo(const char *label, const ECS::Components::MapComponent &mapComp, Core::ResourceManager &resources, uint8_t &current) {
            const auto &typeMats = mapComp.typeMats;
            const auto counts = CountTilesPerType(mapComp);

            std::string preview = "Unknown type";
            if (const auto it = FindTypeMat(typeMats, current); it != typeMats.end()) {
                const auto cit = counts.find(current);
                const size_t count = cit != counts.end() ? cit->second : 0;
                preview = FormatTypeLabel(current, *it->second, resources, count);
            }

            bool changed = false;
            if (ImGui::BeginCombo(label, preview.c_str())) {
                for (const auto &[id, mat] : typeMats) {
                    ImGui::PushID(static_cast<int>(id));
                    const bool isSelected = id == current;
                    const auto cit = counts.find(id);
                    const size_t count = cit != counts.end() ? cit->second : 0;
                    const std::string labelStr = FormatTypeLabel(id, *mat, resources, count);

                    if (const ImVec4 swatchCol(mat->color.r, mat->color.g, mat->color.b, mat->color.a);
                        ImGui::ColorButton("##swatch", swatchCol, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(16, 16))) {
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

        void DrawSwatch(const glm::vec4 &color, const std::shared_ptr<Rendering::Texture> &texture, const float size) {
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            const ImVec2 rectEnd(cursor.x + size, cursor.y + size);

            // checkerboard background for transparency
            ImDrawList *dl = ImGui::GetWindowDrawList();
            constexpr int checkerSize = 6;
            for (int y = 0; y < static_cast<int>(size); y += checkerSize) {
                for (int x = 0; x < static_cast<int>(size); x += checkerSize) {
                    const bool light = (x / checkerSize + y / checkerSize) % 2 == 0;
                    dl->AddRectFilled(ImVec2(cursor.x + x, cursor.y + y), ImVec2(cursor.x + std::min(x + checkerSize, static_cast<int>(size)), cursor.y + std::min(y + checkerSize, static_cast<int>(size))),
                                      light ? IM_COL32(200, 200, 200, 255) : IM_COL32(120, 120, 120, 255));
                }
            }

            // Tinted colour overlay
            dl->AddRectFilled(cursor, rectEnd, IM_COL32(static_cast<int>(color.r * 255.0f), static_cast<int>(color.g * 255.0f), static_cast<int>(color.b * 255.0f), static_cast<int>(color.a * 255.0f)));

            // Border
            dl->AddRect(cursor, rectEnd, IM_COL32(255, 255, 255, 180));
            const ImVec2 swatchSize(size, size);
            if (texture) {
                Core::Utils::UI::ImGuiImageFlipped(texture->GetID(), swatchSize);
            } else {
                ImGui::Dummy(swatchSize);
            }
        }

    } // namespace

    void TileEditorPanel::SetSelectedTile(Map::Tile *tile) {
        m_CurrentTile = tile;
        m_EditInitialized = false;
    }

    void TileEditorPanel::SetBrushType(const uint8_t type) {
        m_BrushType = type;
        if (m_MapComp) {
            if (const auto it = m_MapComp->findTypeMat(type); it != m_MapComp->typeMats.end()) {
                m_BrushTexture = it->second->texture;
                m_BrushColor = it->second->color;
            }
        }
        m_BrushInitialized = true;
    }

    void TileEditorPanel::InitBrushFromFirstType() {
        if (m_MapComp && !m_MapComp->typeMats.empty()) {
            const auto &[id, mat] = *m_MapComp->typeMats.begin();
            m_BrushType = id;
            m_BrushTexture = mat->texture;
            m_BrushColor = mat->color;
        } else {
            m_BrushType = 0;
            m_BrushTexture = nullptr;
            m_BrushColor = {1.0f, 1.0f, 1.0f, 1.0f};
        }
        m_BrushInitialized = true;
    }

    void TileEditorPanel::CopyBrushFromTile(const Map::Tile &tile) {
        if (!m_MapComp)
            return;
        m_BrushType = tile.type;
        if (const auto it = m_MapComp->findTypeMat(tile.type); it != m_MapComp->typeMats.end()) {
            m_BrushTexture = it->second->texture;
            m_BrushColor = it->second->color;
        }
        m_BrushInitialized = true;
    }

    //
    // Render
    //

    void TileEditorPanel::OnImGuiRender() {
        ImGui::Begin("Tile Editor");
        m_IsHovered = ImGui::IsWindowHovered();

        if (!m_MapComp || !m_EngineContext || !m_EngineContext->resources) {
            ImGui::TextDisabled("somethings cooked mane");
            ImGui::End();
            return;
        }

        auto &typeMats = m_MapComp->typeMats;

        if (m_CurrentTool == MapTool::Paint) {
            if (!m_BrushInitialized) {
                InitBrushFromFirstType();
            }

            ImGui::SeparatorText("Brush");

            if (typeMats.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "No tile types defined.");
                ImGui::TextDisabled("Use 'Save as New Type' below to create one.");
            }

            ImGui::BeginGroup();

            DrawSwatch(m_BrushColor, m_BrushTexture, 56.0f);

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("Type %d", static_cast<int>(m_BrushType));

            const auto brushCounts = CountTilesPerType(*m_MapComp);
            if (const auto brushCountIt = brushCounts.find(m_BrushType); brushCountIt != brushCounts.end()) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%zu tile(s) on map", brushCountIt->second);
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "not yet on map");
            }

            ImGui::PushID("BrushType");
            if (uint8_t selectedBrushType = m_BrushType; TypeCombo("##BrushType", *m_MapComp, *m_EngineContext->resources, selectedBrushType)) {
                SetBrushType(selectedBrushType);
            }
            ImGui::PopID();

            ImGui::EndGroup();
            ImGui::EndGroup();

            ImGui::PushID("BrushMat");
            ImGui::Spacing();
            TextureCombo("Texture", *m_EngineContext->resources, m_BrushTexture);

            ImGui::ColorEdit4("Color", &m_BrushColor.x, ImGuiColorEditFlags_NoInputs);
            ImGui::PopID();

            bool brushMatChanged = false;
            {
                if (const auto it = FindTypeMat(typeMats, m_BrushType); it != typeMats.end()) {
                    brushMatChanged = m_BrushTexture != it->second->texture || m_BrushColor != it->second->color;
                } else {
                    brushMatChanged = true;
                }
            }

            if (!brushMatChanged) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Update Brush Type")) {
                if (const auto it = FindTypeMat(typeMats, m_BrushType); it != typeMats.end()) {
                    it->second->texture = m_BrushTexture;
                    it->second->color = m_BrushColor;
                    m_MapComp->needsMeshUpdate = true;
                    m_MapComp->mapDirty = true;
                    if (m_SceneContext)
                        m_SceneContext->MarkAsChanged();
                }
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Replace the material of the current brush type for all tiles using it.");
            }

            ImGui::SameLine();

            if (ImGui::Button("Save as New Type##brush")) {
                if (m_CreateTypeFn) {
                    const uint8_t newId = m_CreateTypeFn(m_BrushTexture, m_BrushColor);
                    SetBrushType(newId);
                    if (m_SceneContext)
                        m_SceneContext->MarkAsChanged();
                }
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Create a brand-new tile type with this texture and colour.\n"
                                  "Future Paint strokes will use this new type.");
            }

            if (!brushMatChanged) {
                ImGui::EndDisabled();
            }

            if (brushMatChanged) {
                if (uint8_t existingId = 0; FindTypeIdForMaterial(*m_MapComp, m_BrushTexture, m_BrushColor, existingId) && existingId != m_BrushType) {
                    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), "Same material as type %u. Use 'Update' to merge, or change texture/colour.", static_cast<unsigned>(existingId));
                }
            }

            ImGui::Spacing();
            ImGui::Checkbox("Walkable", &m_BrushWalkable);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Walkable state applied to all tiles painted with this brush.");
            }
        }

        //
        // Selection
        //

        if (m_CurrentTile && m_CurrentTool == MapTool::Select) {
            ImGui::SeparatorText("Selected Tile");

            const auto typeIt = FindTypeMat(typeMats, m_CurrentTile->type);
            if (typeIt == typeMats.end()) {
                ImGui::TextDisabled("Tile has an unknown type id (%u).", static_cast<unsigned>(m_CurrentTile->type));
                ImGui::End();
                return;
            }

            const auto &currentMat = typeIt->second;

            if (!m_EditInitialized || m_EditSourceType != m_CurrentTile->type) {
                m_EditTexture = currentMat->texture;
                m_EditColor = currentMat->color;
                m_EditSourceType = m_CurrentTile->type;
                m_EditInitialized = true;
            }

            const auto counts = CountTilesPerType(*m_MapComp);
            const auto countIt = counts.find(m_CurrentTile->type);
            const size_t currentTypeCount = countIt != counts.end() ? countIt->second : 0;

            ImGui::PushID("TileType");
            if (uint8_t selectedType = m_CurrentTile->type; TypeCombo("Type##TileType", *m_MapComp, *m_EngineContext->resources, selectedType)) {
                m_CurrentTile->type = selectedType;
                m_MapComp->needsMeshUpdate = true;
                m_MapComp->mapDirty = true;
                if (m_SceneContext)
                    m_SceneContext->MarkAsChanged();
            }
            ImGui::PopID();

            ImGui::PushID("EditMat");
            ImGui::Spacing();
            TextureCombo("Texture", *m_EngineContext->resources, m_EditTexture);
            ImGui::ColorEdit4("Color", &m_EditColor.x, ImGuiColorEditFlags_NoInputs);
            ImGui::PopID();

            const bool materialChanged = m_EditTexture != currentMat->texture || m_EditColor != currentMat->color;

            if (!materialChanged) {
                ImGui::BeginDisabled();
            }

            ImGui::Spacing();

            if (ImGui::Button("Override Current Type")) {
                currentMat->texture = m_EditTexture;
                currentMat->color = m_EditColor;
                m_EditSourceType = m_CurrentTile->type;
                m_MapComp->needsMeshUpdate = true;
                m_MapComp->mapDirty = true;
                if (m_SceneContext)
                    m_SceneContext->MarkAsChanged();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && materialChanged) {
                std::ostringstream tip;
                tip << "Replaces the material of type " << static_cast<int>(m_CurrentTile->type) << " for all " << currentTypeCount << " tile" << (currentTypeCount != 1 ? "s" : "") << " using it.";
                ImGui::SetTooltip("%s", tip.str().c_str());
            }

            ImGui::SameLine();

            if (ImGui::Button("Save as New Type##tile")) {
                if (m_CreateTypeFn) {
                    m_CurrentTile->type = m_CreateTypeFn(m_EditTexture, m_EditColor);
                    m_MapComp->needsMeshUpdate = true;
                    if (m_SceneContext)
                        m_SceneContext->MarkAsChanged();
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
                                           "to share it, or change texture/colour.",
                                           static_cast<unsigned>(existingId));
                    }
                }
            }

            if (ImGui::Checkbox("Is Walkable", &m_CurrentTile->walkable)) {
                m_MapComp->grid.SyncTileWalkableCache(m_CurrentTile->position);
                m_MapComp->needsMeshUpdate = true;
                m_MapComp->mapDirty = true;
                if (m_SceneContext)
                    m_SceneContext->MarkAsChanged();
            } else {
                ImGui::SeparatorText("Selected Tile");
                ImGui::TextDisabled("Click a tile in the viewport to inspect and edit it.");
            }
        }
        ImGui::End();
    }
} // namespace Editor::UI
