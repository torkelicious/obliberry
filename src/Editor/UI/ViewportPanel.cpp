#include "ViewportPanel.h"
#include "Renderer/Renderer.h"
#include <imgui.h>

void ViewportPanel::OnImGuiRender() {
    ImGui::Begin("Scene View");
    m_IsHovered = ImGui::IsWindowHovered();

    if (m_EngineContext &&m_EngineContext

    ->
    renderer
    )
    {
        const int pickedEntity = m_EngineContext->renderer->GetLastReadPixel();
        if (pickedEntity != -1) {
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

                ImGui::Image(texId, viewportSize, ImVec2{0, 1},
                             ImVec2{1, 0});

                if (m_IsHovered &&ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    const ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
                    const ImVec2 viewportOffset = ImGui::GetWindowPos();
                    const ImVec2 boundsMin = {
                        viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y
                    };

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
