#include "UIPanel.h"
#include "imgui.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIButton.h"
#include "UI/Elements/UIImage.h"
#include "UI/Elements/UIRect.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgetsCombo.h"
#include <cstring>

namespace Editor::UI {

    void UIPanel::DrawElementNode(::UI::UIElement *element) {
        if (!element)
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (element->Children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (element == m_SelectedElement)
            flags |= ImGuiTreeNodeFlags_Selected;

        const bool nodeOpen = ImGui::TreeNodeEx(element, flags, "%s", element->Name.c_str());

        if (ImGui::IsItemClicked()) {
            m_SelectedElement = element;
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Text("Element: %s", element->Name.c_str());
            ImGui::Separator();

            bool visible = element->HasFlag(::UI::VISIBLE);
            if (ImGui::Checkbox("Visible", &visible)) {
                if (visible) element->AddFlag(::UI::VISIBLE);
                else element->RemoveFlag(::UI::VISIBLE);
            }

            bool enabled = element->HasFlag(::UI::ENABLED);
            if (ImGui::Checkbox("Enabled", &enabled)) {
                if (enabled) element->AddFlag(::UI::ENABLED);
                else element->RemoveFlag(::UI::ENABLED);
            }

            ImGui::Separator();
            ImGui::Text("Pos: (%.1f, %.1f)", element->Rect.Position.x, element->Rect.Position.y);
            ImGui::Text("Size: (%.1f, %.1f)", element->Rect.Scale.x, element->Rect.Scale.y);

            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", element->Name.c_str());
            ImGui::Text("Pos: (%.1f, %.1f)", element->Rect.Position.x, element->Rect.Position.y);
            ImGui::Text("Size: (%.1f, %.1f)", element->Rect.Scale.x, element->Rect.Scale.y);
            ImGui::Text("Children: %zu", element->Children.size());
            ImGui::Text("Flags: %s%s",
                        element->HasFlag(::UI::VISIBLE) ? "VISIBLE " : "",
                        element->HasFlag(::UI::ENABLED) ? "ENABLED " : "");
            ImGui::EndTooltip();
        }

        if (nodeOpen) {
            for (auto *child : element->Children) {
                DrawElementNode(child);
            }
            ImGui::TreePop();
        }
    }

    void UIPanel::OnImGuiRender() {
        ImGui::Begin("UI Hierarchy");
        m_IsHovered = ImGui::IsWindowHovered();

        auto *uiSys = m_EngineContext ? m_EngineContext->uiSystem : nullptr;
        if (!uiSys) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "UISystem not available");
            ImGui::End();
            return;
        }

        auto *root = uiSys->GetRoot();
        if (!root) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No root element");
            ImGui::End();
            return;
        }

        DrawElementNode(root);

        ImGui::Separator();

        ImGui::Text("Add Element:");
        const char *typeNames[] = {"Rect", "Text", "Button", "Image"};
        ImGui::Combo("Type", &m_AddType, typeNames, 4);

        if (ImGui::Button("Add")) {
            ::UI::UIElement *parent = (m_SelectedElement && m_SelectedElement != root) ? m_SelectedElement : root;

            Commands::UIElementSnapshot snap;
            snap.type = static_cast<Commands::UIElementSnapshot::Type>(m_AddType);
            snap.position = {100.0f, 100.0f};
            snap.flags = ::UI::VISIBLE | ::UI::ENABLED;

            switch (m_AddType) {
                case 0: // Rect
                    snap.name = "NewRect";
                    snap.scale = {80.0f, 40.0f};
                    snap.color = {0.3f, 0.3f, 0.3f, 1.0f};
                    break;
                case 1: // Text
                    snap.name = "NewText";
                    snap.scale = {80.0f, 30.0f};
                    snap.text = "Text";
                    snap.color = {1.0f, 1.0f, 1.0f, 1.0f};
                    break;
                case 2: // Button
                    snap.name = "NewButton";
                    snap.scale = {120.0f, 40.0f};
                    snap.text = "Button";
                    snap.color = {1.0f, 1.0f, 1.0f, 1.0f};
                    snap.bgColor = {0.25f, 0.25f, 0.25f, 1.0f};
                    break;
                case 3: // Image
                    snap.name = "NewImage";
                    snap.scale = {100.0f, 100.0f};
                    snap.color = {0.5f, 0.5f, 0.5f, 1.0f};
                    break;
            }

            auto cmd = std::make_unique<Commands::AddUIElementCommand>(uiSys, parent, std::move(snap));
            if (m_UndoManager) {
                m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
            } else {
                cmd->Execute(*m_EngineContext);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove") && m_SelectedElement && m_SelectedElement != root) {
            ::UI::UIElement *parent = m_SelectedElement->Parent;
            if (parent) {
                auto cmd = std::make_unique<Commands::RemoveUIElementCommand>(uiSys, parent, m_SelectedElement);
                if (m_UndoManager) {
                    m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
                } else {
                    cmd->Execute(*m_EngineContext);
                }
                m_SelectedElement = nullptr;
            }
        }

        ImGui::Separator();
        ImGui::Text("Total: %zu", root->Children.size());

        if (m_SelectedElement) {
            ImGui::Separator();
            ImGui::Text("Selected: %s", m_SelectedElement->Name.c_str());

            char nameBuf[128] = {};
            std::strncpy(nameBuf, m_SelectedElement->Name.c_str(), sizeof(nameBuf) - 1);
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                m_SelectedElement->Name = nameBuf;
            }
            if (ImGui::IsItemActivated()) {
                m_EditibNameStart = m_SelectedElement->Name;
                m_EditingName = true;
            }
            if (ImGui::IsItemDeactivatedAfterEdit() && m_EditingName) {
                if (m_EditibNameStart != m_SelectedElement->Name && m_UndoManager) {
                    auto cmd = std::make_unique<Commands::SetUIElementNameCommand>(
                        m_SelectedElement, m_EditibNameStart, m_SelectedElement->Name);
                    m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
                }
                m_EditingName = false;
            }

            ImGui::DragFloat2("Position", &m_SelectedElement->Rect.Position.x, 1.0f);
            if (ImGui::IsItemActivated()) {
                m_DragStartPos = m_SelectedElement->Rect.Position;
                m_DraggingPos = true;
            }
            if (ImGui::IsItemDeactivatedAfterEdit() && m_DraggingPos) {
                if (m_DragStartPos != m_SelectedElement->Rect.Position && m_UndoManager) {
                    auto cmd = std::make_unique<Commands::SetUIElementPositionCommand>(
                        m_SelectedElement, m_DragStartPos, m_SelectedElement->Rect.Position);
                    m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
                }
                m_DraggingPos = false;
            }

            ImGui::DragFloat2("Size", &m_SelectedElement->Rect.Scale.x, 1.0f);
            if (ImGui::IsItemActivated()) {
                m_DragStartScale = m_SelectedElement->Rect.Scale;
                m_DraggingScale = true;
            }
            if (ImGui::IsItemDeactivatedAfterEdit() && m_DraggingScale) {
                if (m_DragStartScale != m_SelectedElement->Rect.Scale && m_UndoManager) {
                    auto cmd = std::make_unique<Commands::SetUIElementScaleCommand>(
                        m_SelectedElement, m_DragStartScale, m_SelectedElement->Rect.Scale);
                    m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
                }
                m_DraggingScale = false;
            }

            bool visible = m_SelectedElement->HasFlag(::UI::VISIBLE);
            if (ImGui::Checkbox("Visible", &visible)) {
                if (visible) m_SelectedElement->AddFlag(::UI::VISIBLE);
                else m_SelectedElement->RemoveFlag(::UI::VISIBLE);
            }
            ImGui::SameLine();
            bool enabled = m_SelectedElement->HasFlag(::UI::ENABLED);
            if (ImGui::Checkbox("Enabled", &enabled)) {
                if (enabled) m_SelectedElement->AddFlag(::UI::ENABLED);
                else m_SelectedElement->RemoveFlag(::UI::ENABLED);
            }

            ImGui::Separator();

            if (auto *image = dynamic_cast<::UI::UIImage *>(m_SelectedElement)) {
                ImGui::Text("Image Properties");
                auto &tex = image->GetImage();
                TextureCombo("Texture", *m_EngineContext->resources, tex);

                auto imgColor = image->GetColor();
                if (ImGui::ColorEdit4("Color", &imgColor.x)) {
                    image->SetColor(imgColor);
                }
            }

            if (auto *text = dynamic_cast<::UI::UIText *>(m_SelectedElement)) {
                ImGui::Text("Text Properties");
                char textBuf[512] = {};
                std::strncpy(textBuf, text->GetText().c_str(), sizeof(textBuf) - 1);
                if (ImGui::InputTextMultiline("Text", textBuf, sizeof(textBuf), ImVec2(-1, 60))) {
                    text->SetText(textBuf);
                }

                auto textColor = text->GetColor();
                if (ImGui::ColorEdit4("Text Color", &textColor.x)) {
                    text->SetColor(textColor);
                }

                auto font = text->GetFont();
                if (FontCombo("Font", *m_EngineContext->resources, font)) {
                    text->SetFont(font);
                }
            }

            if (auto *btn = dynamic_cast<::UI::UIButton *>(m_SelectedElement)) {
                ImGui::Text("Button Properties");
                char btnTextBuf[256] = {};
                std::strncpy(btnTextBuf, btn->GetText().c_str(), sizeof(btnTextBuf) - 1);
                if (ImGui::InputText("Label", btnTextBuf, sizeof(btnTextBuf))) {
                    btn->SetText(btnTextBuf);
                }

                auto btnColor = btn->GetColor();
                if (ImGui::ColorEdit4("Text Color", &btnColor.x)) {
                    btn->SetColor(btnColor);
                }

                auto bgColor = btn->GetBackgroundColor();
                if (ImGui::ColorEdit4("Background", &bgColor.x)) {
                    btn->SetBackgroundColor(bgColor);
                }

                auto font = btn->GetFont();
                if (FontCombo("Font", *m_EngineContext->resources, font)) {
                    btn->SetFont(font);
                }
            }

            if (auto *rect = dynamic_cast<::UI::UIRect *>(m_SelectedElement)) {
                ImGui::Text("Rect Properties");
                auto rectColor = rect->GetColor();
                if (ImGui::ColorEdit4("Color", &rectColor.x)) {
                    rect->SetColor(rectColor);
                }
            }
        }

        ImGui::End();
    }

} // namespace Editor::UI
