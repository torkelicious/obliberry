// ReSharper disable CppDFAUnreachableCode
#include "ViewportPanel.h"
#include "Rendering/Renderer.h"
#include <imgui.h>
#include <ImGuizmo.h>

#include "Platform/Input/InputManager.h"

void Editor::UI::ViewportPanel::OnImGuiRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar();

    // gizmos

    ImGuizmo::SetDrawlist();

    m_IsHovered = ImGui::IsWindowHovered();

    const ImVec2 boundsMin = ImGui::GetCursorScreenPos();
    m_ViewportBoundsMin = boundsMin;
    const ImVec2 imguiMousePos = ImGui::GetMousePos();

    const double localMouseX = imguiMousePos.x - boundsMin.x;
    const double localMouseY = imguiMousePos.y - boundsMin.y;

    if (m_EngineContext && m_EngineContext->input) {
        if (m_IsHovered) {
            const double offsetX = m_EngineContext->input->RawMousePosX() - localMouseX;
            const double offsetY = m_EngineContext->input->RawMousePosY() - localMouseY;

            m_EngineContext->input->SetViewportOffset(offsetX, offsetY);
        } else {
            m_EngineContext->input->SetViewportOffset(0.0, 0.0);
        }
    }

    if (m_EngineContext && m_EngineContext->renderer) {
        // if gizmo is being actively dragged, cancel any pending pick
        if (ImGuizmo::IsUsing() && m_ExpectingPick) {
            m_ExpectingPick = false;
            m_EngineContext->renderer->ClearPixelReadResult();
        }

        if (m_ExpectingPick && !m_EngineContext->renderer->IsPixelReadRequested() && !ImGuizmo::IsUsing()) {
            if (const int pickedEntity = m_EngineContext->renderer->GetLastReadPixel(); pickedEntity != -1) {
                m_SelectedEntityID = pickedEntity;
                m_HadEmptyClick = false;
            } else {
                m_SelectedEntityID = -1;
                m_HadEmptyClick = true;
            }
            m_EngineContext->renderer->ClearPixelReadResult();
            m_ExpectingPick = false;
        }
    }

    if (const ImVec2 viewportSize = ImGui::GetContentRegionAvail(); viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
        m_ViewportWidth = viewportSize.x;
        m_ViewportHeight = viewportSize.y;

        ImGuizmo::SetRect(boundsMin.x, boundsMin.y, viewportSize.x, viewportSize.y);

        if (m_EngineContext && m_EngineContext->renderer) {
            m_EngineContext->renderer->EnsureFramebufferSize(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));

            if (const auto fbo = m_EngineContext->renderer->GetEditorFramebuffer()) {
                const uint32_t texId = fbo->GetColorAttID();

                ImGui::Image(texId, viewportSize, ImVec2{0, 1}, ImVec2{1, 0});

                // play mode badge in top-left corner
                if (m_ShowPlayIndicator) {
                    ImDrawList *drawList = ImGui::GetWindowDrawList();
                    constexpr auto label = "Playing";
                    const ImVec2 textSize = ImGui::CalcTextSize(label);
                    constexpr float padding = 6.0f;
                    const ImVec2 badgeMin(boundsMin.x + 8.0f, boundsMin.y + 8.0f);
                    const ImVec2 badgeMax(badgeMin.x + textSize.x + padding * 2.0f, badgeMin.y + textSize.y + padding * 2.0f);
                    drawList->AddRectFilled(badgeMin, badgeMax, IM_COL32(200, 40, 40, 200), 4.0f);
                    drawList->AddText(ImVec2(badgeMin.x + padding, badgeMin.y + padding), IM_COL32(255, 255, 255, 220), label);
                }

                if (m_IsHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing() && !m_UIHandleHover) {
                    const ImVec2 mousePos = ImGui::GetMousePos();

                    const int mouseX = static_cast<int>(mousePos.x - boundsMin.x);
                    const int mouseY = static_cast<int>(mousePos.y - boundsMin.y);

                    if (const int glY = static_cast<int>(m_ViewportHeight) - mouseY - 1; mouseX >= 0 && mouseY >= 0 && mouseX < static_cast<int>(m_ViewportWidth) && glY >= 0 && glY < static_cast<int>(m_ViewportHeight)) {
                        m_EngineContext->renderer->RequestPixelRead(mouseX, glY);
                        m_ExpectingPick = true;
                    }
                }
            }
        }
    }
    ImGui::End();
}

ImVec2 Editor::UI::ViewportPanel::GetLocalMousePos() const {
    const ImVec2 mousePos = ImGui::GetMousePos();
    return {mousePos.x - m_ViewportBoundsMin.x, mousePos.y - m_ViewportBoundsMin.y};
}

glm::vec2 Editor::UI::ViewportPanel::MousePosToWorld(const Rendering::Camera &camera) const {
    const ImVec2 mousePos = ImGui::GetMousePos();
    const float localX = mousePos.x - m_ViewportBoundsMin.x;
    const float localY = mousePos.y - m_ViewportBoundsMin.y;
    return camera.MouseToWorld(localX, localY, m_ViewportWidth, m_ViewportHeight);
}
