#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <imgui.h>
#include "EditorDialog.h"

// New Project Dialog
namespace Editor::UI {
    class NewProjectDialog : public EditorDialog {
    public:
        NewProjectDialog() : EditorDialog("New Project") {
        }

        void SetDirectory(const std::filesystem::path &dir) {
            m_Dir = dir;
            strncpy(m_NameBuf, "UntitledProject", sizeof(m_NameBuf));
        }

        void SetOnConfirm(const std::function<void(std::filesystem::path, std::string)> &cb) { m_OnConfirmCb = cb; }

    protected:
        void DrawContent() override {
            ImGui::Text("Create project in: %s", m_Dir.string().c_str());
            ImGui::InputText("Project Name", m_NameBuf, sizeof(m_NameBuf));
            if (ImGui::Button("Create")) {
                if (m_OnConfirmCb) m_OnConfirmCb(m_Dir, m_NameBuf);
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) Close();
        }

    private:
        std::filesystem::path m_Dir;
        char m_NameBuf[128] = "UntitledProject";
        std::function<void(std::filesystem::path, std::string)> m_OnConfirmCb;
    };

    // Create Scene Dialog
    class CreateSceneDialog : public EditorDialog {
    public:
        CreateSceneDialog() : EditorDialog("Create Scene") {
        }

        void Reset() { m_NameBuf[0] = '\0'; }
        void SetOnConfirm(const std::function<void(std::string)> &cb) { m_OnConfirmCb = cb; }

    protected:
        void DrawContent() override {
            ImGui::InputText("Scene Name", m_NameBuf, sizeof(m_NameBuf));
            if (ImGui::Button("Create")) {
                if (m_OnConfirmCb) m_OnConfirmCb(m_NameBuf);
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) Close();
        }

    private:
        char m_NameBuf[128] = "";
        std::function<void(std::string)> m_OnConfirmCb;
    };

    // Save Scene As Dialog
    class SaveSceneAsDialog : public EditorDialog {
    public:
        SaveSceneAsDialog() : EditorDialog("Save Scene As") {
        }

        void SetCurrentName(const std::string &name) {
            strncpy(m_NameBuf, name.c_str(), sizeof(m_NameBuf) - 1);
        }

        void SetOnConfirm(const std::function<void(std::string)> &cb) { m_OnConfirmCb = cb; }

    protected:
        void DrawContent() override {
            ImGui::InputText("Scene Name", m_NameBuf, sizeof(m_NameBuf));
            if (ImGui::Button("Save")) {
                if (m_OnConfirmCb) m_OnConfirmCb(m_NameBuf);
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) Close();
        }

    private:
        char m_NameBuf[128] = "";
        std::function<void(std::string)> m_OnConfirmCb;
    };
}
