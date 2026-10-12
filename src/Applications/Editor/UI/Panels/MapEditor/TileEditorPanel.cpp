#include "TileEditorPanel.h"
#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgetsCombo.h"
#include "Core/Utils/UiUtils.h"
#include <algorithm>
#include <imgui.h>
#include "imgui_internal.h"
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace Editor::UI {
    namespace {
        using TypeCounts = std::unordered_map<uint8_t, size_t>;

        TypeCounts CountTilesPerType(const ECS::Components::MapComponent &mapComp) {
            TypeCounts counts;
            for (const auto &[coords, tile] : mapComp.grid.tiles) {
                (void)coords;
                counts[tile.type]++;
            }
            return counts;
        }

        size_t CountFor(const TypeCounts &counts, const uint8_t id) {
            const auto it = counts.find(id);
            return it != counts.end() ? it->second : 0;
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

        auto FindTypeMat(auto &typeMats, const uint8_t id) {
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

        bool TypeCombo(const char *label, const ECS::Components::MapComponent &mapComp, Core::ResourceManager &resources, const TypeCounts &counts, const std::optional<uint8_t> current, uint8_t &picked) {
            const auto &typeMats = mapComp.typeMats;

            std::string preview = "Mixed";
            if (current) {
                preview = "Unknown type";
                if (const auto it = FindTypeMat(typeMats, *current); it != typeMats.end()) {
                    preview = FormatTypeLabel(*current, *it->second, resources, CountFor(counts, *current));
                }
            }

            bool changed = false;
            if (ImGui::BeginCombo(label, preview.c_str())) {
                for (const auto &[id, mat] : typeMats) {
                    ImGui::PushID(id);
                    const bool isSelected = current && id == *current;
                    const std::string labelStr = FormatTypeLabel(id, *mat, resources, CountFor(counts, id));

                    if (const ImVec4 swatchCol(mat->color.r, mat->color.g, mat->color.b, mat->color.a);
                            ImGui::ColorButton("##swatch", swatchCol, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(16, 16))) {
                        picked = id;
                        changed = true;
                    }
                    ImGui::SameLine();

                    if (ImGui::Selectable(labelStr.c_str(), isSelected)) {
                        picked = id;
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
    TileEditorPanel::SelectionSummary TileEditorPanel::SummarizeSelection() const {
        SelectionSummary summary;
        if (!m_Selection || !m_MapComp) {
            return summary;
        }

        std::unordered_set<uint8_t> types;
        for (const auto &hex : *m_Selection) {
            const Map::Tile *tile = m_MapComp->grid.Get(hex);
            if (!tile) {
                continue;
            }
            if (summary.count == 0) {
                summary.commonType = tile->type;
                summary.commonWalkable = tile->walkable;
            } else {
                if (summary.commonType && *summary.commonType != tile->type) {
                    summary.commonType.reset();
                }
                if (summary.commonWalkable && *summary.commonWalkable != tile->walkable) {
                    summary.commonWalkable.reset();
                }
            }
            types.insert(tile->type);
            ++summary.count;
        }
        summary.distinctTypes = types.size();
        return summary;
    }

    void TileEditorPanel::OnImGuiRender() {
        ImGui::Begin("Tile Editor");
        m_IsHovered = ImGui::IsWindowHovered();

        if (!m_MapComp || !m_EngineContext || !m_EngineContext->resources) {
            ImGui::TextDisabled("somethings cooked mane");
            ImGui::End();
            return;
        }

        auto &typeMats = m_MapComp->typeMats;
        auto &resources = *m_EngineContext->resources;
        const TypeCounts counts = CountTilesPerType(*m_MapComp);

        if (m_CurrentTool == MapTool::Paint) {
            if (!m_BrushInitialized) {
                InitBrushFromFirstType();
            }

            ImGui::SeparatorText("Brush");

            if (typeMats.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "No tile types defined, no map will be visible!");
                ImGui::TextDisabled("Use 'Save as New Type' below to create one.");
            }

            ImGui::BeginGroup();

            DrawSwatch(m_BrushColor, m_BrushTexture, 56.0f);

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("Type %d", static_cast<int>(m_BrushType));

            if (const size_t brushCount = CountFor(counts, m_BrushType); brushCount > 0) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%zu tile(s) on map", brushCount);
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "not yet on map");
            }

            ImGui::PushID("BrushType");
            if (uint8_t picked = m_BrushType; TypeCombo("##BrushType", *m_MapComp, resources, counts, m_BrushType, picked)) {
                SetBrushType(picked);
            }
            ImGui::PopID();

            ImGui::EndGroup();
            ImGui::EndGroup();

            ImGui::PushID("BrushMat");
            ImGui::Spacing();
            TextureCombo("Texture", m_EngineContext, m_BrushTexture);

            ImGui::ColorEdit4("Color", &m_BrushColor.x, ImGuiColorEditFlags_NoInputs);
            ImGui::PopID();

            bool brushMatChanged = true;
            if (const auto it = FindTypeMat(typeMats, m_BrushType); it != typeMats.end()) {
                brushMatChanged = m_BrushTexture != it->second->texture || m_BrushColor != it->second->color;
            }

            ImGui::BeginDisabled(!brushMatChanged);

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

            ImGui::EndDisabled();

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
        if (m_CurrentTool == MapTool::Select) {
            const SelectionSummary summary = SummarizeSelection();

            ImGui::SeparatorText("Selection");

            if (summary.count == 0) {
                m_EditInitialized = false;
                m_EditSourceType.reset();
                ImGui::TextDisabled("Click or drag in the viewport to select tiles.");
                ImGui::End();
                return;
            }

            ImGui::Text("%zu tile%s, %zu type%s", summary.count, summary.count == 1 ? "" : "s", summary.distinctTypes, summary.distinctTypes == 1 ? "" : "s");
            ImGui::Spacing();

            ImGui::PushID("SelType");
            if (uint8_t picked = 0; TypeCombo("Type", *m_MapComp, resources, counts, summary.commonType, picked) && m_ApplyToSelectionFn) {
                m_ApplyToSelectionFn(picked, std::nullopt);
            }
            ImGui::PopID();

            {
                bool walkable = summary.commonWalkable.value_or(false);
                const bool mixed = !summary.commonWalkable.has_value();
                if (mixed) {
                    ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
                }
                if (ImGui::Checkbox("Is Walkable", &walkable) && m_ApplyToSelectionFn) {
                    m_ApplyToSelectionFn(std::nullopt, walkable);
                }
                if (mixed) {
                    ImGui::PopItemFlag();
                }
            }

            if (!summary.commonType) {
                m_EditInitialized = false;
                m_EditSourceType.reset();
                ImGui::Spacing();
                ImGui::TextDisabled("Select tiles of a single type to edit its material.");
                ImGui::End();
                return;
            }

            const uint8_t commonType = *summary.commonType;
            const auto typeIt = FindTypeMat(typeMats, commonType);
            if (typeIt == typeMats.end()) {
                ImGui::TextDisabled("Tiles have an unknown type id (%u).", static_cast<unsigned>(commonType));
                ImGui::End();
                return;
            }

            const auto currentMat = typeIt->second;

            if (!m_EditInitialized || m_EditSourceType != summary.commonType) {
                m_EditTexture = currentMat->texture;
                m_EditColor = currentMat->color;
                m_EditSourceType = summary.commonType;
                m_EditInitialized = true;
            }

            ImGui::PushID("EditMat");
            ImGui::Spacing();
            TextureCombo("Texture", m_EngineContext, m_EditTexture);
            ImGui::ColorEdit4("Color", &m_EditColor.x, ImGuiColorEditFlags_NoInputs);
            ImGui::PopID();

            const bool materialChanged = m_EditTexture != currentMat->texture || m_EditColor != currentMat->color;
            const size_t typeCount = CountFor(counts, commonType);

            ImGui::Spacing();
            ImGui::BeginDisabled(!materialChanged);

            if (ImGui::Button("Override Current Type")) {
                currentMat->texture = m_EditTexture;
                currentMat->color = m_EditColor;
                m_MapComp->needsMeshUpdate = true;
                m_MapComp->mapDirty = true;
                if (m_SceneContext)
                    m_SceneContext->MarkAsChanged();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && materialChanged) {
                ImGui::SetTooltip("Replaces the material of type %d for all %zu tile%s using it.", static_cast<int>(commonType), typeCount, typeCount != 1 ? "s" : "");
            }

            ImGui::SameLine();

            if (ImGui::Button("Save as New Type##tile")) {
                if (m_CreateTypeFn && m_ApplyToSelectionFn) {
                    const uint8_t newId = m_CreateTypeFn(m_EditTexture, m_EditColor);
                    m_ApplyToSelectionFn(newId, std::nullopt);
                }
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Creates a new type with this material and assigns it to the selected tiles only.");
            }

            ImGui::EndDisabled();

            if (materialChanged) {
                if (uint8_t existingId = 0; FindTypeIdForMaterial(*m_MapComp, m_EditTexture, m_EditColor, existingId) && existingId != commonType) {
                    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), "Same material as type %u. Use 'Override Current Type' to share it, or change texture/colour.", static_cast<unsigned>(existingId));
                }
            }
        }

        ImGui::End();
    }
} // namespace Editor::UI
