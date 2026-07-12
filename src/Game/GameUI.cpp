#include "GameLayer.h"

#include "imgui.h"

static ImVec4 GetThresholdColor(const float frameMs) {
    if (frameMs >= 33.0f)
        return {1.0f, 0.3f, 0.3f, 1.0f}; // critical (sub-30 FPS)
    if (frameMs >= 20.0f)
        return {1.0f, 0.8f, 0.2f, 1.0f}; // warning (sub-50 FPS)
    return {0.4f, 1.0f, 0.4f, 1.0f};     // Good
}

void Game::GameLayer::DrawInterface() const {
    constexpr float PADDING = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - PADDING, viewport->WorkPos.y + PADDING), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.40f);
    constexpr ImGuiWindowFlags overlayFlags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("FPS", nullptr, overlayFlags)) {
        ImGui::TextColored(GetThresholdColor(m_Context->deltaTime * 1000.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
    }
    ImGui::End();
}
