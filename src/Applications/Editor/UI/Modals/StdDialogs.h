#pragma once
#include "Core/Project.h"
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <imgui.h>
#include "EditorDialog.h"

// New Project Dialog
namespace Editor::UI {
    class NewProjectDialog : public EditorDialog {
    public:
        NewProjectDialog() : EditorDialog("New Project") {}

        void SetDirectory(const std::filesystem::path &dir) {
            m_Dir = dir;
            strncpy(m_NameBuf, "UntitledProject", sizeof(m_NameBuf));
        }

        void SetTemplates(std::vector<Core::Project::TemplateInfo> templates) {
            m_Templates = std::move(templates);
            m_SelectedTemplate = 0;
            for (size_t i = 0; i < m_Templates.size(); ++i) {
                if (m_Templates[i].id == "Default") {
                    m_SelectedTemplate = i;
                    break;
                }
            }
        }

        void SetOnConfirm(const std::function<void(std::filesystem::path, std::string, std::string)> &cb) { m_OnConfirmCb = cb; }

    protected:
        void DrawContent() override {
            ImGui::Text("Create project in: %s", m_Dir.string().c_str());
            ImGui::InputText("Project Name", m_NameBuf, sizeof(m_NameBuf));

            if (!m_Templates.empty()) {
                if (m_SelectedTemplate >= m_Templates.size())
                    m_SelectedTemplate = 0;

                ImGui::Spacing();
                ImGui::Text("Template");
                if (ImGui::BeginCombo("##template", m_Templates[m_SelectedTemplate].displayName.c_str())) {
                    for (size_t i = 0; i < m_Templates.size(); ++i) {
                        const bool selected = (m_SelectedTemplate == i);
                        if (ImGui::Selectable(m_Templates[i].displayName.c_str(), selected))
                            m_SelectedTemplate = i;
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            if (ImGui::Button("Create")) {
                if (m_OnConfirmCb) {
                    const std::string templateId = m_Templates.empty() ? "Default" : m_Templates[m_SelectedTemplate].id;
                    m_OnConfirmCb(m_Dir, m_NameBuf, templateId);
                }
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                Close();
        }

    private:
        std::filesystem::path m_Dir;
        char m_NameBuf[128] = "UntitledProject";
        std::vector<Core::Project::TemplateInfo> m_Templates;
        size_t m_SelectedTemplate = 0;
        std::function<void(std::filesystem::path, std::string, std::string)> m_OnConfirmCb;
    };

    // Create Scene Dialog
    class CreateSceneDialog : public EditorDialog {
    public:
        CreateSceneDialog() : EditorDialog("Create Scene") {}

        void Reset() { m_NameBuf[0] = '\0'; }
        void SetOnConfirm(const std::function<void(std::string)> &cb) { m_OnConfirmCb = cb; }

    protected:
        void DrawContent() override {
            ImGui::InputText("Scene Name", m_NameBuf, sizeof(m_NameBuf));
            if (ImGui::Button("Create")) {
                if (m_OnConfirmCb)
                    m_OnConfirmCb(m_NameBuf);
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                Close();
        }

    private:
        char m_NameBuf[128] = "";
        std::function<void(std::string)> m_OnConfirmCb;
    };

    // Generic Save Changes Dialog
    class SaveChangesDialog : public EditorDialog {
    public:
        SaveChangesDialog() : EditorDialog("Unsaved Changes") {}

        void SetMessage(const std::string &msg) { m_Message = msg; }

        void SetOnSave(const std::function<void()> &cb) { m_OnSave = cb; }
        void SetOnDiscard(const std::function<void()> &cb) { m_OnDiscard = cb; }

    protected:
        void DrawContent() override {
            ImGui::TextWrapped("%s", m_Message.c_str());
            ImGui::Separator();

            if (ImGui::Button("Save")) {
                if (m_OnSave)
                    m_OnSave();
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Don't Save")) {
                if (m_OnDiscard)
                    m_OnDiscard();
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                Close();
            }
        }

    private:
        std::string m_Message;
        std::function<void()> m_OnSave;
        std::function<void()> m_OnDiscard;
    };

    // Save Scene As Dialog
    class SaveSceneAsDialog : public EditorDialog {
    public:
        SaveSceneAsDialog() : EditorDialog("Save Scene As") {}

        void SetCurrentName(const std::string &name) { strncpy(m_NameBuf, name.c_str(), sizeof(m_NameBuf) - 1); }

        void SetOnConfirm(const std::function<void(std::string)> &cb) { m_OnConfirmCb = cb; }

    protected:
        void DrawContent() override {
            ImGui::InputText("Scene Name", m_NameBuf, sizeof(m_NameBuf));
            if (ImGui::Button("Save")) {
                if (m_OnConfirmCb)
                    m_OnConfirmCb(m_NameBuf);
                Close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                Close();
        }

    private:
        char m_NameBuf[128] = "";
        std::function<void(std::string)> m_OnConfirmCb;
    };
} // namespace Editor::UI
