#pragma once

#include "Applications/Editor/EditorContext.h"
#include "Logger/ILogger.h"
#include "Logger/LogSink.h"
#include "glm/ext/vector_float4.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <imgui.h>

namespace Editor::UI {

    static constexpr size_t MAX_CONSOLE_LINES = 2000;

    struct ConsoleLogPanel {
        std::string name;
        std::vector<std::string> logs;
        std::vector<Logging::LogSeverity> severity;
        size_t previousLogCount = 0;

        // Thread-safe sink buffer shared with the Logger
        std::shared_ptr<Logging::UILogSink> uiSink = std::make_shared<Logging::UILogSink>();

        void Clear(){
            logs.clear();
            severity.clear();
        }

        void Flush() {
            if (!uiSink)
                return;

            uiSink->ConsumePending(logs, severity);

            if (logs.size() > MAX_CONSOLE_LINES) {
                logs.erase(logs.begin(), logs.end() - MAX_CONSOLE_LINES);
                severity.erase(severity.begin(), severity.end() - MAX_CONSOLE_LINES);
            }
        }

        void Draw(EditorContext &ctx) {
            ImGui::Begin(name.c_str());

            if (ImGui::Button("Clear")) {
                logs.clear();
                severity.clear();
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

                        ImVec4 color{};
                        switch (severity[static_cast<size_t>(i)]) {
                            case Logging::LogSeverity::Debug:
                                color = {0.0f, 1.0f, 0.0f, 1.0f};
                                break;
                            case Logging::LogSeverity::Info:
                                color = {1.0f, 1.0f, 1.0f, 1.0f};
                                break;
                            case Logging::LogSeverity::Warn:
                                color = {0.0f, 1.0f, 1.0f, 1.0f};
                                break;
                            case Logging::LogSeverity::Error:
                                color = {1.0f, 0.0f, 0.0f, 1.0f};
                                break;
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, color);
                        ImGui::TextUnformatted(logs[static_cast<size_t>(i)].c_str());
                        ImGui::PopStyleColor();
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
