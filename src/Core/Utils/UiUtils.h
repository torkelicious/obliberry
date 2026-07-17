#pragma once
#include <imgui.h>
#include <glad/glad.h>

namespace Core::Utils::UI {
    // textures are loaded with stbi flip on so they must be unflipped
    static void ImGuiImageFlipped(const GLuint textureID, const ImVec2 &size) {
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        drawList->AddImage(textureID, cursorPos, ImVec2(cursorPos.x + size.x, cursorPos.y + size.y), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Dummy(size);
    }
} // namespace Core::Utils::UI
