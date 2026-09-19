#pragma once
#include "Logger/LoggerService.h"
#include "imgui.h"
#include <random>
namespace Core::Platform::Funny {

    inline bool RandomChance(float percentage = 0.5f) {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::bernoulli_distribution chance(percentage / 100);
        if (chance(rng)) {
            return true;
        }
        return false;
    }

    inline void AnInconspicuousFunction(bool force = false) {
        if (!force) {
            if (!RandomChance()) {
                return;
            }
        }
        ImGui::OpenPopup("Error");
        if (ImGui::BeginPopupModal("FATAL ERROR", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("This project is ass. Session Terminated");
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                LOG_ERROR("FATAL", "Project Was Ass. Terminating.");
                std::abort();
            }
            ImGui::EndPopup();
        }
    }

} // namespace Core::Platform::Funny
