#include "MapEditState.h"
#include "Rendering/Renderer.h"
#include <algorithm>
#include <filesystem>
#include "Core/Constants.h"
#include "Applications/Editor/EditorLayer.h"
#include "Platform/Input/InputManager.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "Math/HexMath.h"
#include <imgui.h>
#include "imgui_internal.h"
#include "Applications/Editor/Platform/FileDialogs.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "IO/MapSerialization.h"
#include "IO/VFS/VFS.h"

namespace {
    std::string GetMapsDirectory() {
        const std::string dir = IO::VFS::Resolve(std::string(Core::MAP_PATH)).string();
        std::filesystem::create_directories(dir);
        return dir;
    }
} // namespace


#pragma push_macro("LOG_WHO")
#define LOG_WHO "MapEditState"

void Editor::States::MapEditState::OnEnter() {

    m_MapState = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapStateComponent>();
    m_MapComp = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapComponent>();

    if (!m_MapComp) {
        LOG_ERROR(LOG_WHO, "No MapComponent found in scene, cannot enter map editor");
        return;
    }

    m_CurrentGrid = &m_MapComp->grid;
    m_MapComp->pathToMat->color = {0.0, 0.8, 1.0, 0.50};   // cyan
    m_MapComp->outlineMat->color = {1.0, 0.85, 0.0, 0.55}; // gold / yellowish
    m_selectedTile = m_CurrentGrid->Get(m_selectedHex);
    m_TileEditorPanel.SetContext(m_EditorLayer->m_Scene, m_EditorLayer->m_Context);
    m_TileEditorPanel.SetMapComponent(m_MapComp);
    m_TileEditorPanel.SetCreateTypeFn([this](const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color) { return GetOrCreateTypeForMaterial(tex, color); });
    if (!m_MapComp->typeMats.empty()) {
        m_TileEditorPanel.InitBrushFromFirstType();
    }
    m_TileEditorPanel.SetSelectedTile(m_selectedTile);
    SetWindowTitle(m_MapComp->mapFilePath);
}

void Editor::States::MapEditState::OnUpdate(const float dt) {}

void Editor::States::MapEditState::OnHandleInput(const float dt) {

    const bool viewportHovered = m_EditorLayer->m_ViewportPanel.IsHovered();

    if (m_EditorLayer->m_Input->IsKeyPressed("V")) {
        m_EditorLayer->m_Camera.ToggleViewMode();
    }

    // Reset keyboard pan velocity when not hovering
    float kbPanX = 0.0f;
    float kbPanY = 0.0f;
    m_EditorLayer->m_Camera.StopKeyboardPan();

    if (!viewportHovered) {
        if (m_EditorLayer->m_Input->IsMouseReleased(0)) {
            if (m_MapDragging) {
                CommitMapChanges();
                m_MapDragging = false;
            }
            m_IsDragging = false;
        }
        return;
    }

    m_hoveredHex = Math::HexMath::PixelToHex(m_EditorLayer->m_ViewportPanel.MousePosToWorld(m_EditorLayer->m_Camera));

    if (!m_MapState || !m_MapComp) {
        return;
    }

    m_MapState->hasSelection = true;
    // The component names are misleading: "selectedHex" is actually the hovered hex,
    // while "pathTo" represents the real clicked selection.
    m_MapState->selectedHex = m_hoveredHex;

    // click &/or drag

    if (m_EditorLayer->m_Input->IsMouseDown(0)) {
        if (!m_MapDragging) {
            m_MapDragging = true;
            m_PreDragState.clear();
            m_AccumulatedNew.clear();
        }
        ApplyToolAt(m_hoveredHex);
        m_IsDragging = true;
    }
    if (m_EditorLayer->m_Input->IsMouseReleased(0)) {
        if (m_MapDragging) {
            CommitMapChanges();
            m_MapDragging = false;
        }
        m_IsDragging = false;
    }

    // Scroll zoom
    if (const auto scrollDelta = static_cast<float>(m_EditorLayer->m_Input->ScrollY()); scrollDelta != 0.0f) {
        m_EditorLayer->m_Camera.AdjustZoom(scrollDelta * 0.2f);
    }

    // Mouse pan
    const auto mouseDeltaX = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaX());
    const auto mouseDeltaY = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaY());

    if (m_EditorLayer->m_Input->IsMouseDown("MouseMiddle") || m_EditorLayer->m_Input->IsMouseDown("MouseRight")) {
        m_EditorLayer->m_Camera.Pan(-mouseDeltaX, mouseDeltaY, 0.025f);
    }

    // Keyboard pan
    if (m_EditorLayer->m_Input->IsKeyDown("W"))
        kbPanY += 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("S"))
        kbPanY -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("A"))
        kbPanX -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("D"))
        kbPanX += 1.0f;

    if (kbPanX != 0.0f || kbPanY != 0.0f) {
        const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
        kbPanX /= length;
        kbPanY /= length;
    }

    const float speedMod = m_EditorLayer->m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
    constexpr float moveAmount = 3.0f;
    const float vpHeight = m_EditorLayer->m_ViewportPanel.GetHeight();
    m_EditorLayer->m_Camera.KeyboardPan(kbPanX, kbPanY, 15.0f * speedMod * moveAmount * (600.0f / vpHeight));
}

void Editor::States::MapEditState::OnDrawPanels() {}

void Editor::States::MapEditState::OnRender() {
    m_EditorLayer->DrawEditorUI();
    m_TileEditorPanel.OnImGuiRender();

    auto *renderer = m_EditorLayer->m_Context.renderer;
    renderer->BeginFrame();

    const glm::mat4 &vp = renderer->GetCurrentVP();
    const Math::Frustum::ViewFrustum frustum = Math::Frustum::FromCameraVP(vp, Core::HEX_SIZE * 2.0f);

    ECS::Systems::MapRenderSystem::RenderAll(*m_EditorLayer->m_Registry, m_EditorLayer->m_Context, frustum);

    if (m_MapComp && m_CurrentTool != MapTool::Select && m_BrushRadius > 1 && m_MapComp->hexMesh && m_MapComp->outlineMat) {
        ForEachHexInRing(m_hoveredHex, m_BrushRadius - 1, [&](const Map::HexCoords &hex) {
            const auto worldPos = Map::HexGrid::GetWorldPos(hex);
            Rendering::Transform t;
            t.SetPosition({worldPos.x, worldPos.y, 0.01f});
            t.SetScale({1.08f, 1.08f, 1.0f});
            renderer->Submit(m_MapComp->hexMesh, m_MapComp->outlineMat, t);
        });
    }

    renderer->SetLightmap(nullptr);
}

void Editor::States::MapEditState::OnDrawModeToolbar() {
    {
        if (m_MapComp) {
            const std::string &mapPath = m_MapComp->mapFilePath;
            if (mapPath.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%sUnsaved Map", m_MapComp->mapDirty ? "* " : "");
            } else if (m_MapComp->mapDirty) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "* %s", mapPath.c_str());
            } else {
                ImGui::TextDisabled("%s", mapPath.c_str());
            }
            ImGui::SameLine();
        }

        if (ImGui::Button("Map##MapMenuBtn")) {
            ImGui::OpenPopup("MapMenuPopup");
        }
        if (ImGui::BeginPopup("MapMenuPopup")) {
            if (ImGui::MenuItem("Create New Map", nullptr, false, m_MapComp != nullptr)) {
                m_EditorLayer->PromptSaveDirtyMap([this] {
                    if (!m_CurrentGrid || !m_MapComp)
                        return;
                    m_CurrentGrid->Clear();
                    m_MapComp->typeMats.clear();
                    m_MapComp->mapFilePath.clear();
                    m_MapComp->mapDirty = false;
                    m_MapComp->needsMeshUpdate = true;
                    m_selectedTile = nullptr;
                    m_TileEditorPanel.SetSelectedTile(nullptr);
                    m_TileEditorPanel.InitBrushFromFirstType();
                    // old undo commands reference tiles from the wiped grid
                    m_EditorLayer->m_UndoManager.Clear();
                    SetWindowTitle("Unsaved Map");
                });
            }
            if (ImGui::MenuItem("Save Map", nullptr, false, m_MapComp != nullptr)) {
                SaveMap();
            }
            if (ImGui::MenuItem("Save Map As", nullptr, false, m_MapComp != nullptr)) {
                LOG_INFO(LOG_WHO, "Save as requested");
                const std::string mapsDir = GetMapsDirectory();
                if (const auto path = Platform::FileDialogs::SaveFile(m_EditorLayer->m_Context, {.filterName = "Map File", .filterExt = "obmap", .defaultPath = mapsDir.c_str(), .defaultName = "map.obmap"})) {
                    m_MapComp->mapFilePath = IO::VFS::ToRelative(*path);
                    m_EditorLayer->SaveScene();
                    m_MapComp->mapDirty = false;
                }
            }
            if (ImGui::MenuItem("Load Map from file", nullptr, false, m_MapComp != nullptr)) {
                LOG_INFO(LOG_WHO, "Load requested");
                const std::string mapsDir = GetMapsDirectory();
                if (const auto path = Platform::FileDialogs::OpenFile(m_EditorLayer->m_Context, {.filterName = "Map File", .filterExt = "obmap", .defaultPath = mapsDir.c_str()})) {
                    if (IO::MapIO::Deserialize(*path, *m_CurrentGrid)) {
                        m_MapComp->mapFilePath = IO::VFS::ToRelative(*path);
                        m_MapComp->needsMeshUpdate = true;
                        m_MapComp->mapDirty = false;
                        LOG_INFO(LOG_WHO, "Loaded from file: " + *path);
                        SetWindowTitle(m_MapComp->mapFilePath);
                    } else {
                        LOG_ERROR(LOG_WHO, "Failed to load map into editor");
                    }
                }
            }
            ImGui::EndPopup();
        }

        constexpr auto activeCol = ImVec4(0.3f, 0.6f, 1.0f, 0.7f);
        constexpr auto inactiveCol = ImVec4(0.2f, 0.2f, 0.2f, 0.5f);

        auto drawToolBtn = [&](const char *label, const MapTool tool, const char *tip) {
            if (m_CurrentTool == tool) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeCol);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, inactiveCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.5f, 0.9f, 0.6f));
            }
            if (ImGui::Button(label)) {
                m_CurrentTool = tool;
                m_TileEditorPanel.SetMapTool(m_CurrentTool);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", tip);
            }
            ImGui::PopStyleColor(3);
        };

        drawToolBtn("  Paint  ", MapTool::Paint, "Left-click to place tiles. Drag to paint continuously.");
        ImGui::SameLine();
        drawToolBtn("  Erase  ", MapTool::Erase, "Left-click to remove tiles. Drag to erase continuously.");
        ImGui::SameLine();
        drawToolBtn("  Select ", MapTool::Select, "Click a tile to inspect and edit its properties.");
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::Text("Size");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    int radius = m_BrushRadius;
    if (ImGui::SliderInt("##Radius", &radius, 1, 5, "%d")) {
        m_BrushRadius = radius;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Brush radius in hexes. Affects Paint and Erase tools.");
    }
}

void Editor::States::MapEditState::OnDrawUtilityWindows() {}

void Editor::States::MapEditState::OnSceneLoaded() {
    m_MapState = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapStateComponent>();
    m_MapComp = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapComponent>();

    if (m_MapComp) {
        m_CurrentGrid = &m_MapComp->grid;
        m_selectedTile = m_CurrentGrid->Get(m_selectedHex);
        m_TileEditorPanel.SetMapComponent(m_MapComp);
        m_TileEditorPanel.SetSelectedTile(m_selectedTile);
    } else {
        m_CurrentGrid = nullptr;
        m_selectedTile = nullptr;
    }
}

void Editor::States::MapEditState::SaveMap() {
    if (m_MapComp->mapFilePath.empty()) {
        // no file yet
        const std::string mapsDir = GetMapsDirectory();
        if (const auto path = Platform::FileDialogs::SaveFile(m_EditorLayer->m_Context, {.filterName = "Map File", .filterExt = "obmap", .defaultPath = mapsDir.c_str(), .defaultName = "map.obmap"})) {
            m_MapComp->mapFilePath = IO::VFS::ToRelative(*path);
            m_EditorLayer->SaveScene();
            m_MapComp->mapDirty = false;
            SetWindowTitle(m_MapComp->mapFilePath);
        }
    } else {
        LOG_INFO(LOG_WHO, "Save requested");
        m_EditorLayer->SaveScene();
        m_MapComp->mapDirty = false;
    }
}

void Editor::States::MapEditState::OnSaveKey() { SaveMap(); }

void Editor::States::MapEditState::OnExit() {
    m_MapState = nullptr;
    m_MapComp = nullptr;
    m_CurrentGrid = nullptr;

    m_EditorLayer->m_Registry->ForEach<ECS::Components::MapStateComponent>([&](ECS::Entity, ECS::Components::MapStateComponent *state) { state->hasSelection = false; });
}

void Editor::States::MapEditState::CapturePreDragState(const Map::HexCoords &hex) {
    ForEachHexInRing(hex, m_BrushRadius - 1, [&](const Map::HexCoords &h) {
        if (!m_PreDragState.contains(h)) {
            if (const auto *tile = m_CurrentGrid->Get(h)) {
                m_PreDragState[h] = TileState{tile->type, tile->walkable};
            } else {
                m_PreDragState[h] = std::nullopt;
            }
        }
    });
}

void Editor::States::MapEditState::CommitMapChanges() {
    if (m_PreDragState.empty())
        return;
    m_EditorLayer->m_UndoManager.Execute(std::make_unique<Commands::MapChangeTileCommand>(m_PreDragState, m_AccumulatedNew, m_CurrentGrid, &m_MapComp->needsMeshUpdate), m_EditorLayer->m_Context);
    m_MapComp->needsMeshUpdate = true;
    m_MapComp->mapDirty = true;
    if (m_EditorLayer->m_Scene)
        m_EditorLayer->m_Scene->MarkAsChanged();
}

void Editor::States::MapEditState::ApplyToolAt(const Map::HexCoords &hex) {
    m_MapState->hasPathTo = false;
    switch (m_CurrentTool) {
        case MapTool::Select:
            m_selectedHex = hex;
            m_MapState->pathTo = hex;
            m_MapState->hasPathTo = true;
            m_selectedTile = m_CurrentGrid->Get(hex);
            m_TileEditorPanel.SetSelectedTile(m_selectedTile);
            if (m_selectedTile) {
                m_TileEditorPanel.CopyBrushFromTile(*m_selectedTile);
            }
            break;

        case MapTool::Paint: {
            const uint8_t brushType = m_TileEditorPanel.GetSourceType();
            const bool brushWalkable = m_TileEditorPanel.GetBrushWalkable();
            CapturePreDragState(hex);
            bool changed = false;
            ForEachHexInRing(hex, m_BrushRadius - 1, [&](const Map::HexCoords &h) {
                if (const Map::Tile *existing = m_CurrentGrid->Get(h); existing && existing->type == brushType && existing->walkable == brushWalkable) {
                    m_AccumulatedNew[h] = TileState{brushType, brushWalkable};
                    return;
                }
                m_CurrentGrid->RemoveTileAt(h);
                m_CurrentGrid->EmplaceTile(h, brushType, brushWalkable);
                m_AccumulatedNew[h] = TileState{brushType, brushWalkable};
                changed = true;
            });
            if (changed)
                m_MapComp->needsMeshUpdate = true;
            break;
        }

        case MapTool::Erase: {
            CapturePreDragState(hex);
            ForEachHexInRing(hex, m_BrushRadius - 1, [&](const Map::HexCoords &h) {
                if (m_CurrentGrid->HasTile(h)) {
                    m_CurrentGrid->RemoveTileAt(h);
                    m_AccumulatedNew[h] = std::nullopt; // erased
                }
            });
            m_MapComp->needsMeshUpdate = true;
            break;
        }
    }
}

void Editor::States::MapEditState::ForEachHexInRing(const Map::HexCoords center, const int radius, const std::function<void(const Map::HexCoords &)> &callback) {
    const auto [cx, cy, cz] = Math::HexMath::OddRToCube(center);

    for (int dx = -radius; dx <= radius; ++dx) {
        const int dyMin = std::max(-radius, -dx - radius);
        const int dyMax = std::min(radius, -dx + radius);
        for (int dy = dyMin; dy <= dyMax; ++dy) {
            const int dz = -dx - dy;
            const int r = cz + dz;
            const int q = cx + dx + (r - (r & 1)) / 2;
            callback(Map::HexCoords(q, r));
        }
    }
}

uint8_t Editor::States::MapEditState::GetOrCreateTypeForMaterial(const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color) const {
    auto &typeMats = m_MapComp->typeMats;
    for (const auto &[id, mat] : typeMats) {
        if (mat->texture == tex && mat->color == color) {
            return id;
        }
    }
    // allocate newid
    uint8_t newId = 0;
    while (std::ranges::any_of(typeMats, [newId](const auto &p) { return p.first == newId; })) {
        newId++;
    }

    const auto shader = !typeMats.empty() ? typeMats.begin()->second->shader : m_EditorLayer->m_Context.resources->Get<Rendering::Shader>("[Engine] Base");

    typeMats.emplace_back(newId, std::make_shared<Rendering::Material>(Rendering::Material{shader, tex, color}));
    m_MapComp->needsMeshUpdate = true;
    if (m_EditorLayer->m_Scene)
        m_EditorLayer->m_Scene->MarkAsChanged();
    return newId;
}
#pragma pop_macro("LOG_WHO")
