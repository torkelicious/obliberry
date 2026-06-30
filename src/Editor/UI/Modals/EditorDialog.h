#pragma once
#include <imgui.h>
#include <string>
#include <functional>
#include <memory>
#include <utility>

namespace Editor::UI {
    class EditorDialog {
    public:
        EditorDialog(std::string title) : m_Title(std::move(title)), m_IsOpen(false), m_ShouldOpen(false) {
        }

        virtual ~EditorDialog() = default;

        void Open() {
            m_ShouldOpen = true;
            m_IsOpen = true;
        }

        void Close() { m_IsOpen = false; }

        // callback to execute on success
        void SetOnConfirm(const std::function<void()> &callback) { m_OnConfirm = callback; }

        void Update() {
            if (!m_IsOpen) return;

            if (m_ShouldOpen) {
                ImGui::OpenPopup(m_Title.c_str());
                m_ShouldOpen = false;
            }

            if (ImGui::BeginPopupModal(m_Title.c_str(), &m_IsOpen, GetFlags())) {
                DrawContent();
                ImGui::EndPopup();
            }
        }

    protected:
        virtual void DrawContent() = 0;

        [[nodiscard]] virtual ImGuiWindowFlags GetFlags() const { return ImGuiWindowFlags_AlwaysAutoResize; }

        std::string m_Title;
        std::function<void()> m_OnConfirm = nullptr;
        bool m_IsOpen;
        bool m_ShouldOpen;
    };

    template<typename DialogType>
    class ModalButton {
    public:
        // label for the button, and the callback for when the dialog succeeds
        ModalButton(std::string buttonLabel, std::function<void()> action)
            : m_Label(std::move(buttonLabel)) {
            m_Dialog = std::make_unique<DialogType>();
            m_Dialog->SetOnConfirm(action);
        }

        void OnImGuiRender() {
            if (ImGui::Button(m_Label.c_str())) {
                m_Dialog->Open();
            }
            m_Dialog->Update();
        }

        DialogType *GetDialog() { return m_Dialog.get(); }

    private:
        std::string m_Label;
        std::unique_ptr<DialogType> m_Dialog;
    };
}
