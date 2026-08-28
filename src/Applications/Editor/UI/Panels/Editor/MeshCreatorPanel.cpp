#include "MeshCreatorPanel.h"

#include "Applications/Editor/EditorLayer.h"
#include "Rendering/Types/Mesh/MeshFactory.h"
#include "Rendering/Renderer.h"
#include "Platform/Threading/SmallTask.h"
#include "Logger/LoggerService.h"
#include <algorithm>
#include <cmath>

namespace {
    void FlattenCubicBezier(const glm::vec2 &p0, const glm::vec2 &p1, const glm::vec2 &p2, const glm::vec2 &p3, const float toleranceSq, const int depth, std::vector<glm::vec2> &out) {
        if (depth >= 24) {
            out.push_back(p3);
            return;
        }

        const glm::vec2 d = p3 - p0;

        const float dDot = glm::dot(d, d);

        // avoid loop thing
        if (dDot < 1e-12f) {
            out.push_back(p3);
            return;
        }

        const float d2 = std::abs((p1.x - p3.x) * d.y - (p1.y - p3.y) * d.x);
        const float d3 = std::abs((p2.x - p3.x) * d.y - (p2.y - p3.y) * d.x);

        if ((d2 + d3) * (d2 + d3) < toleranceSq * glm::dot(d, d)) {
            out.push_back(p3);
            return;
        }

        const glm::vec2 p01 = (p0 + p1) * 0.5f, p12 = (p1 + p2) * 0.5f, p23 = (p2 + p3) * 0.5f;
        const glm::vec2 p012 = (p01 + p12) * 0.5f, p123 = (p12 + p23) * 0.5f;
        const glm::vec2 p0123 = (p012 + p123) * 0.5f;

        FlattenCubicBezier(p0, p01, p012, p0123, toleranceSq, depth + 1, out);
        FlattenCubicBezier(p0123, p123, p23, p3, toleranceSq, depth + 1, out);
    }

    std::vector<glm::vec2> FlattenPath(const ImVectorEditor::Path &path, const float tolerance) {
        std::vector<glm::vec2> points;
        const size_t n = path.anchors.size();
        if (n == 0)
            return points;

        const float toleranceSq = tolerance * tolerance;
        points.emplace_back(path.anchors[0].position.x, path.anchors[0].position.y);

        const size_t segmentCount = path.closed ? n : (n - 1);
        for (size_t i = 0; i < segmentCount; ++i) {
            const ImVectorEditor::Anchor &a = path.anchors[i];
            const ImVectorEditor::Anchor &b = path.anchors[(i + 1) % n];

            glm::vec2 p0(a.position.x, a.position.y);
            glm::vec2 p3(b.position.x, b.position.y);

            if (a.hasHandleOut || b.hasHandleIn) {
                glm::vec2 p1 = a.hasHandleOut ? glm::vec2(a.position.x + a.handleOut.x, a.position.y + a.handleOut.y) : p0;
                glm::vec2 p2 = b.hasHandleIn ? glm::vec2(b.position.x + b.handleIn.x, b.position.y + b.handleIn.y) : p3;
                FlattenCubicBezier(p0, p1, p2, p3, toleranceSq, 0, points);
            } else {
                points.push_back(p3);
            }
        }

        if (path.closed && points.size() > 1 && glm::distance(points.front(), points.back()) < 0.001f) {
            points.pop_back();
        }

        return points;
    }
} // namespace

Editor::UI::MeshCreatorPanel::MeshCreatorPanel() {
    m_IVEConfig.tool = ImVectorEditor::Tool::Pen;
    m_IVEConfig.canvasSize = ImVec2(0.0f, 320.0f);
}

void Editor::UI::MeshCreatorPanel::OnImGuiRender() {
    ImGui::Begin("2D Mesh Creator");
    m_IsHovered = ImGui::IsWindowHovered();

    DrawToolbar();
    DrawHint();

    if (m_PathEditor.GetSelectedAnchor() >= 0) {
        ImGui::Spacing();
        DrawHandleModeControls();
    }
    ImGui::Spacing();

    DrawPathOperationControls();
    ImGui::Separator();

    m_IVEConfig.canvasSize = ImVec2(0.0f, 320.0f);
    const ImVectorEditor::Result res = m_PathEditor.Draw("Shape Editor", m_Path, m_IVEConfig);

    ApplyViewDeltas(res);

    if (res.changed)
        InvalidateMesh();

    if (res.pathClosed)
        m_IVEConfig.tool = ImVectorEditor::Tool::Select;

    ImGui::Separator();
    DrawStatusBar();
    ImGui::Separator();
    DrawMeshSection();

    ImGui::End();
}

void Editor::UI::MeshCreatorPanel::ApplyViewDeltas(const ImVectorEditor::Result &res) {
    if (res.viewPanDelta.x != 0.0f || res.viewPanDelta.y != 0.0f) {
        m_IVEConfig.transform.pan.x += res.viewPanDelta.x;
        m_IVEConfig.transform.pan.y += res.viewPanDelta.y;
    }

    if (res.viewZoomFactor != 1.0f) {
        const float oldZoom = m_IVEConfig.transform.zoom;
        const float newZoom = std::clamp(oldZoom * res.viewZoomFactor, kMinZoom, kMaxZoom);
        if (newZoom != oldZoom) {
            const float appliedFactor = newZoom / oldZoom;
            const ImVec2 &c = res.viewZoomCenterCanvas;
            m_IVEConfig.transform.pan = ImVec2(c.x - (c.x - m_IVEConfig.transform.pan.x) * appliedFactor, c.y - (c.y - m_IVEConfig.transform.pan.y) * appliedFactor);
            m_IVEConfig.transform.zoom = newZoom;
        }
    }
}

void Editor::UI::MeshCreatorPanel::DrawToolbar() {
    if (ImGui::RadioButton("Select", m_IVEConfig.tool == ImVectorEditor::Tool::Select))
        m_IVEConfig.tool = ImVectorEditor::Tool::Select;
    ImGui::SameLine();
    ImGui::BeginDisabled(m_Path.closed);
    if (ImGui::RadioButton("Pen", m_IVEConfig.tool == ImVectorEditor::Tool::Pen))
        m_IVEConfig.tool = ImVectorEditor::Tool::Pen;
    ImGui::EndDisabled();
    if (m_Path.closed && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Open or clear the path first to start a new shape.");
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &m_IVEConfig.showGrid);

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(12.0f, 0.0f));
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        if (m_Path.anchors.empty()) {
            Reset();
        } else {
            m_RequestClearConfirm = true;
        }
    }

    if (m_RequestClearConfirm)
        ImGui::OpenPopup("Clear shape?");

    if (ImGui::BeginPopupModal("Clear shape?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("This deletes all %zu anchor(s). Cannot be undone.", m_Path.anchors.size());
        ImGui::Spacing();
        if (ImGui::Button("Clear it", ImVec2(120, 0))) {
            Reset();
            m_RequestClearConfirm = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_RequestClearConfirm = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Editor::UI::MeshCreatorPanel::DrawHint() const {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    if (m_IVEConfig.tool == ImVectorEditor::Tool::Pen)
        ImGui::TextUnformatted("Click to add anchors, drag for handles, click the first anchor to close.");
    else
        ImGui::TextUnformatted("Click/drag anchors, shift-click to multi-select, double-click for handles.");
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Pen: Enter=finish  Esc=cancel stroke  Backspace=remove last\n"
                          "Select: Del=remove selection\n"
                          "View: middle-drag=pan  scroll=zoom");
    }
}

void Editor::UI::MeshCreatorPanel::DrawHandleModeControls() {
    const int selected = m_PathEditor.GetSelectedAnchor();
    if (selected < 0 || selected >= static_cast<int>(m_Path.anchors.size()))
        return;

    ImGui::Text("Anchor #%d", selected);
    ImGui::SameLine();

    ImVectorEditor::Anchor &anchor = m_Path.anchors[selected];
    const bool hasAnyHandle = anchor.hasHandleIn || anchor.hasHandleOut;

    if (ImGui::RadioButton("Corner", anchor.handleMode == ImVectorEditor::HandleMode::Corner)) {
        ImVectorEditor::MakeCorner(anchor);
        InvalidateMesh();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Aligned", anchor.handleMode == ImVectorEditor::HandleMode::Aligned)) {
        ImVectorEditor::MakeAligned(anchor);
        InvalidateMesh();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Mirrored", anchor.handleMode == ImVectorEditor::HandleMode::Mirrored)) {
        ImVectorEditor::MakeMirrored(anchor);
        InvalidateMesh();
    }

    ImGui::BeginDisabled(hasAnyHandle);
    if (ImGui::Button("Add Handles")) {
        ImVectorEditor::AddHandles(anchor);
        InvalidateMesh();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!hasAnyHandle);
    if (ImGui::Button("Delete Handles")) {
        ImVectorEditor::DeleteHandles(anchor);
        InvalidateMesh();
    }
    ImGui::EndDisabled();
}

void Editor::UI::MeshCreatorPanel::DrawPathOperationControls() {
    const bool canClose = m_Path.anchors.size() >= 3;

    ImGui::BeginDisabled(!m_Path.closed && !canClose);
    if (ImGui::Button(m_Path.closed ? "Open Path" : "Close Path")) {
        m_Path.closed = !m_Path.closed;
        InvalidateMesh();
        if (m_Path.closed)
            m_IVEConfig.tool = ImVectorEditor::Tool::Select;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(m_Path.anchors.size() < 2);
    if (ImGui::Button("Reverse Path")) {
        ImVectorEditor::ReversePath(m_Path);
        InvalidateMesh();
    }
    ImGui::EndDisabled();

    if (!m_Path.closed && !m_Path.anchors.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled(canClose ? "(open: close to generate)" : "(need 3+ anchors to close)");
    }
}

void Editor::UI::MeshCreatorPanel::DrawStatusBar() const {
    ImGui::Text("Anchors: %zu", m_Path.anchors.size());
    ImGui::SameLine();
    ImGui::TextColored(m_Path.closed ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(0.9f, 0.6f, 0.3f, 1.0f), m_Path.closed ? "[closed]" : "[open]");
    const int selCount = m_PathEditor.GetSelectedAnchorCount();
    if (selCount > 1) {
        ImGui::SameLine();
        ImGui::Text("Sel: %d", selCount);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Zoom: %.0f%%", m_IVEConfig.transform.zoom * 100.0f);
}

void Editor::UI::MeshCreatorPanel::DrawMeshSection() {
    const bool canGenerate = m_Path.closed && m_Path.anchors.size() >= 3;

    if (ImGui::SliderFloat("Curve Tolerance", &m_FlattenTolerance, 0.1f, 5.0f, "%.2f")) {
        InvalidateMesh();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("How curved segments are flattened into line segments.\nLower = smoother (more points), higher = more faceted.");
    }

    ImGui::BeginDisabled(!canGenerate);
    if (ImGui::Button("Generate Mesh", ImVec2(140, 0))) {
        GenerateMesh();
    }
    ImGui::EndDisabled();

    if (!canGenerate) {
        ImGui::SameLine();
        ImGui::TextDisabled("(close with 3+ anchors first)");
    }

    if (m_MeshGenerated) {
        if (m_MeshStale) {
            ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "-> %d verts, %d tris (stale)", m_GeneratedVertexCount, m_GeneratedTriangleCount);
        } else {
            ImGui::Text("-> %d verts, %d tris", m_GeneratedVertexCount, m_GeneratedTriangleCount);
        }
        if (m_MeshTriangFailure) {
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Could not triangulate this shape!");
        }

        if (!m_GeneratedMeshData.vertices.empty() && !m_MeshTriangFailure) {
            ImGui::Spacing();
            ImGui::SeparatorText("Register Asset");
            ImGui::InputText("ID##creator", m_MeshNameBuffer, sizeof(m_MeshNameBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Save Mesh")) {
                auto &resources = Core::ResourceManager::GetInstance();
                if (const std::string id(m_MeshNameBuffer); id.empty()) {
                    LOG_ERROR("MeshCreator", "Mesh name cannot be empty");
                } else {
                    auto mesh = resources.LoadFromFactory<Rendering::Mesh>(id, [data = m_GeneratedMeshData] {
                        auto m = std::make_shared<Rendering::Mesh>(data);
                        m->SetFactoryId("Custom");
                        m->CustomDataStore(data); // preserve for serialization
                        return m;
                    });
                    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([mesh] { mesh->InitGL(); }));
                    LOG_INFO("MeshCreator", "Created mesh '" + id + "'");
                    States::EditState::HideMeshCreator();
                    Reset();
                }
            }
        }
    }
    if (ImGui::Button("Cancel")) {
        States::EditState::HideMeshCreator();
        Reset();
    }
}

void Editor::UI::MeshCreatorPanel::GenerateMesh() {
    if (!m_Path.closed || m_Path.anchors.size() < 3)
        return;

    const std::vector<glm::vec2> outline = FlattenPath(m_Path, m_FlattenTolerance);

    m_GeneratedMeshData = Rendering::MeshFactory::CreateCustomMesh2D(outline);
    m_GeneratedVertexCount = static_cast<int>(m_GeneratedMeshData.vertices.size());
    m_GeneratedTriangleCount = static_cast<int>(m_GeneratedMeshData.indices.size() / 3);
    m_MeshTriangFailure = (m_GeneratedVertexCount > 0 && m_GeneratedMeshData.indices.empty());
    m_MeshGenerated = true;
    m_MeshStale = false;
}

std::vector<glm::vec2> Editor::UI::MeshCreatorPanel::GetShapeVec2Points() const {
    std::vector<glm::vec2> points;
    points.reserve(m_Path.anchors.size());

    for (const ImVectorEditor::Anchor &anchor : m_Path.anchors)
        points.emplace_back(anchor.position.x, anchor.position.y);

    return points;
}

std::vector<glm::vec2> Editor::UI::MeshCreatorPanel::GetShapeVec2Points_CanvasSpace() const {
    std::vector<glm::vec2> points;
    points.reserve(m_Path.anchors.size());

    for (const ImVectorEditor::Anchor &anchor : m_Path.anchors) {
        ImVec2 canvasPos = m_IVEConfig.transform.LocalToCanvas(anchor.position);
        points.emplace_back(canvasPos.x, canvasPos.y);
    }

    return points;
}

void Editor::UI::MeshCreatorPanel::InvalidateMesh() {
    if (m_MeshGenerated)
        m_MeshStale = true;
}

void Editor::UI::MeshCreatorPanel::Reset() {
    m_Path.clear();
    m_PathEditor.ClearSelection();
    m_MeshGenerated = false;
    m_MeshStale = false;
    m_MeshTriangFailure = false;
    m_GeneratedVertexCount = 0;
    m_GeneratedTriangleCount = 0;
}
