#include "ViewportPanel.h"
#include "Renderer/Renderer.h"
#include <imgui.h>

void ViewportPanel::OnImGuiRender() {
    ImGui::Begin("Scene View");
    m_IsHovered = ImGui::IsWindowHovered();
    if (const ImVec2 viewportSize = ImGui::GetContentRegionAvail(); viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
        m_ViewportWidth = viewportSize.x;
        m_ViewportHeight = viewportSize.y;

        if (m_EngineContext && m_EngineContext->renderer) {
            m_EngineContext->renderer->EnsureFramebufferSize(static_cast<uint32_t>(viewportSize.x),
                                                             static_cast<uint32_t>(viewportSize.y));
            if (const auto fbo = m_EngineContext->renderer->GetEditorFramebuffer()) {
                const uint32_t texId = fbo->GetColorAttID();
                ImGui::Image(static_cast<intptr_t>(texId), viewportSize, ImVec2{0, 1},
                             ImVec2{1, 0});
            }
        }
    }
    ImGui::End();
}
