#include "ViewportPanel.h"
#include "Rendering/Renderer.h"
#include <imgui.h>
#include "Core/InputManager.h"

void Editor::UI::ViewportPanel::OnImGuiRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Scene View");
    ImGui::PopStyleVar();

    m_IsHovered = ImGui::IsWindowHovered();

    const ImVec2 boundsMin = ImGui::GetCursorScreenPos();
    const ImVec2 imguiMousePos = ImGui::GetMousePos();

    const double localMouseX = imguiMousePos.x - boundsMin.x;
    const double localMouseY = imguiMousePos.y - boundsMin.y;

    if (m_EngineContext &&m_EngineContext

    ->
    input
    )
    {
        if (m_IsHovered) {
            const double offsetX = m_EngineContext->input->RawMousePosX() - localMouseX;
            const double offsetY = m_EngineContext->input->RawMousePosY() - localMouseY;

            m_EngineContext->input->SetViewportOffset(offsetX, offsetY);
        } else {
            m_EngineContext->input->SetViewportOffset(0.0, 0.0);
        }
    }

    if (m_EngineContext &&m_EngineContext

    ->
    renderer
    )
    {
        if (const int pickedEntity = m_EngineContext->renderer->GetLastReadPixel(); pickedEntity != -1) {
            m_SelectedEntityID = pickedEntity;
            m_EngineContext->renderer->ClearPixelReadResult();
        }
    }

    if (const ImVec2 viewportSize = ImGui::GetContentRegionAvail(); viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
        m_ViewportWidth = viewportSize.x;
        m_ViewportHeight = viewportSize.y;

        if (m_EngineContext &&m_EngineContext

        ->
        renderer
        )
        {
            m_EngineContext->renderer->EnsureFramebufferSize(static_cast<uint32_t>(viewportSize.x),
                                                             static_cast<uint32_t>(viewportSize.y));

            if (const auto fbo = m_EngineContext->renderer->GetEditorFramebuffer()) {
                const uint32_t texId = fbo->GetColorAttID();

                ImGui::Image(texId, viewportSize, ImVec2{0, 1}, ImVec2{1, 0});

                if (m_IsHovered &&ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    const ImVec2 mousePos = ImGui::GetMousePos();

                    const int mouseX = static_cast<int>(mousePos.x - boundsMin.x);
                    const int mouseY = static_cast<int>(mousePos.y - boundsMin.y);

                    const int glY = static_cast<int>(m_ViewportHeight) - mouseY - 1;

                    if (mouseX >= 0 && mouseY >= 0 && mouseX < static_cast<int>(m_ViewportWidth) && glY >= 0 &&
                        glY < static_cast<int>(m_ViewportHeight)) {
                        m_EngineContext->renderer->RequestPixelRead(mouseX, glY);
                    }
                }
            }
        }
    }
    ImGui::End();
}
