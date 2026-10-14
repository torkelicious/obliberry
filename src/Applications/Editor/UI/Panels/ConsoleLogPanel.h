#pragma once

#include "Applications/Editor/EditorContext.h"
#include <cstddef>
#include <string>
#include <vector>
#include <sstream>
#include <imgui.h>

namespace Editor::UI {

    static constexpr size_t MAX_CONSOLE_LINES = 2000;

    struct ConsoleLogPanel {
        std::string name;
        std::vector<std::string> logs;
        size_t previousLogCount = 0;
        std::stringstream stream;

        void Flush() {
            std::string line;
            while (std::getline(stream, line)) {
                logs.push_back(std::move(line));
                if (logs.size() > MAX_CONSOLE_LINES)
                    logs.erase(logs.begin(), logs.end() - MAX_CONSOLE_LINES);
            }
            stream.clear();
        }

        void Draw(EditorContext &ctx) {
            ImGui::Begin(name.c_str());

            if (ImGui::Button("Clear")) {
                logs.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy")) {
                std::string allText;
                for (const auto &line : logs) {
                    allText += line;
                    allText += '\n';
                }
                ImGui::SetClipboardText(allText.c_str());
            }

            ImGui::Separator();

            ImGui::PushFont(ctx.fontset.FindFont(UI::Theme::FontRole::Monospace));
            ImGui::BeginChild("##console_scroll", ImVec2(-1, -1), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
            {
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(logs.size()));
                while (clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                        ImGui::TextUnformatted(logs[static_cast<size_t>(i)].c_str());
                    }
                }
            }

            if (logs.size() != previousLogCount) {
                ImGui::SetScrollHereY(1.0f);
                previousLogCount = logs.size();
            }
            ImGui::EndChild();
            ImGui::PopFont();

            ImGui::End();
        }
    };

} // namespace Editor::UI
