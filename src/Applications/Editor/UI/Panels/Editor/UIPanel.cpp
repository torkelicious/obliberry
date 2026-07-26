#include "UIPanel.h"
#include "imgui.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIButton.h"
#include "UI/Elements/UIImage.h"
#include "UI/Elements/UIRect.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgetsCombo.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgets.h"
#include <cstring>
#include <algorithm>

namespace Editor::UI {

    static bool IsUIDescendantOf(const ::UI::UIElement *ancestor, const ::UI::UIElement *descendant) {
        if (!descendant)
            return false;
        const auto *p = descendant->Parent;
        while (p) {
            if (p == ancestor)
                return true;
            p = p->Parent;
        }
        return false;
    }

    static void ReparentUIElement(::UI::UIElement *element, ::UI::UIElement *newParent) {
        if (!element || !newParent || element == newParent)
            return;
        if (element == newParent)
            return;
        if (IsUIDescendantOf(element, newParent))
            return;

        // remove from old parent
        if (element->Parent) {
            auto &siblings = element->Parent->Children;
            std::erase(siblings, element);
        }

        // attach to new parent
        element->Parent = newParent;
        newParent->Children.push_back(element);
    }

    void UIPanel::DrawElementNode(::UI::UIElement *element) {
        if (!element)
            return;

        ImGui::PushID(element);

        const bool hasChildren = !element->Children.empty();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        if (element == m_SelectedElement)
            flags |= ImGuiTreeNodeFlags_Selected;

        const bool nodeOpen = ImGui::TreeNodeEx("##node", flags, "%s", element->Name.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_SelectedElement = element;
        }

        // Drag
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("UI_ELEMENT_DRAG", &element, sizeof(::UI::UIElement *));
            ImGui::TextUnformatted(element->Name.c_str());
            ImGui::EndDragDropSource();
        }

        // Drop
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("UI_ELEMENT_DRAG")) {
                if (auto *dragged = *static_cast<::UI::UIElement **>(payload->Data); dragged != element && !IsUIDescendantOf(element, dragged)) {
                    ReparentUIElement(dragged, element);
                    MarkSceneChanged(m_EngineContext);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // context menu
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Text("Element: %s", element->Name.c_str());
            ImGui::Separator();

            bool visible = element->HasFlag(::UI::VISIBLE);
            if (ImGui::Checkbox("Visible", &visible)) {
                if (visible)
                    element->AddFlag(::UI::VISIBLE);
                else
                    element->RemoveFlag(::UI::VISIBLE);
                MarkSceneChanged(m_EngineContext);
            }

            bool enabled = element->HasFlag(::UI::ENABLED);
            if (ImGui::Checkbox("Enabled", &enabled)) {
                if (enabled)
                    element->AddFlag(::UI::ENABLED);
                else
                    element->RemoveFlag(::UI::ENABLED);
                MarkSceneChanged(m_EngineContext);
            }

            ImGui::Separator();
            ImGui::Text("Pos: (%.1f, %.1f)", element->Rect.Position.x, element->Rect.Position.y);
            ImGui::Text("Size: (%.1f, %.1f)", element->Rect.Scale.x, element->Rect.Scale.y);

            if (element->Parent && element->Parent->Parent) {
                ImGui::Separator();
                if (ImGui::MenuItem("Detach")) {
                    auto *root = element;
                    while (root->Parent)
                        root = root->Parent;
                    ReparentUIElement(element, root);
                    MarkSceneChanged(m_EngineContext);
                }
            }

            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", element->Name.c_str());
            ImGui::Text("Pos: (%.1f, %.1f)", element->Rect.Position.x, element->Rect.Position.y);
            ImGui::Text("Size: (%.1f, %.1f)", element->Rect.Scale.x, element->Rect.Scale.y);
            ImGui::Text("Children: %zu", element->Children.size());
            ImGui::Text("Flags: %s%s", element->HasFlag(::UI::VISIBLE) ? "VISIBLE " : "", element->HasFlag(::UI::ENABLED) ? "ENABLED " : "");
            ImGui::EndTooltip();
        }

        if (nodeOpen) {
            if (hasChildren) {
                for (auto *child : element->Children) {
                    DrawElementNode(child);
                }
                ImGui::TreePop();
            }
        }

        ImGui::PopID();
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
            ::UI::UIElement *parent = m_SelectedElement && m_SelectedElement != root ? m_SelectedElement : root;

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
            MarkSceneChanged(m_EngineContext);
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
                MarkSceneChanged(m_EngineContext);
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
                    auto cmd = std::make_unique<Commands::SetUIElementNameCommand>(m_SelectedElement, m_EditibNameStart, m_SelectedElement->Name);
                    m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
                }
                MarkSceneChanged(m_EngineContext);
                m_EditingName = false;
            }

            if (m_SelectedElement != root) {
                ImGui::DragFloat2("Position", &m_SelectedElement->Rect.Position.x, 1.0f);
                if (ImGui::IsItemActivated()) {
                    m_DragStartPos = m_SelectedElement->Rect.Position;
                    m_DraggingPos = true;
                }
                if (ImGui::IsItemDeactivatedAfterEdit() && m_DraggingPos) {
                    if (m_DragStartPos != m_SelectedElement->Rect.Position && m_UndoManager) {
                        auto cmd = std::make_unique<Commands::SetUIElementPositionCommand>(m_SelectedElement, m_DragStartPos, m_SelectedElement->Rect.Position);
                        m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
                    }
                    MarkSceneChanged(m_EngineContext);
                    m_DraggingPos = false;
                }

                ImGui::DragFloat2("Size", &m_SelectedElement->Rect.Scale.x, 1.0f);
                if (ImGui::IsItemActivated()) {
                    m_DragStartScale = m_SelectedElement->Rect.Scale;
                    m_DraggingScale = true;
                }
                if (ImGui::IsItemDeactivatedAfterEdit() && m_DraggingScale) {
                    if (m_DragStartScale != m_SelectedElement->Rect.Scale && m_UndoManager) {
                        auto cmd = std::make_unique<Commands::SetUIElementScaleCommand>(m_SelectedElement, m_DragStartScale, m_SelectedElement->Rect.Scale);
                        m_UndoManager->Execute(std::move(cmd), *m_EngineContext);
                    }
                    MarkSceneChanged(m_EngineContext);
                    m_DraggingScale = false;
                }
            } else {
                ImGui::TextDisabled("Canvas is not movable/resizable");
            }

            bool visible = m_SelectedElement->HasFlag(::UI::VISIBLE);
            if (ImGui::Checkbox("Visible", &visible)) {
                if (visible)
                    m_SelectedElement->AddFlag(::UI::VISIBLE);
                else
                    m_SelectedElement->RemoveFlag(::UI::VISIBLE);
                MarkSceneChanged(m_EngineContext);
            }
            ImGui::SameLine();
            bool enabled = m_SelectedElement->HasFlag(::UI::ENABLED);
            if (ImGui::Checkbox("Enabled", &enabled)) {
                if (enabled)
                    m_SelectedElement->AddFlag(::UI::ENABLED);
                else
                    m_SelectedElement->RemoveFlag(::UI::ENABLED);
                MarkSceneChanged(m_EngineContext);
            }

            ImGui::Separator();

            if (auto *image = dynamic_cast<::UI::UIImage *>(m_SelectedElement)) {
                ImGui::Text("Image Properties");
                if (auto &tex = image->GetImage(); TextureCombo("Texture", *m_EngineContext->resources, tex)) {
                    MarkSceneChanged(m_EngineContext);
                }

                auto imgColor = image->GetColor();
                if (ImGui::ColorEdit4("Color", &imgColor.x)) {
                    image->SetColor(imgColor);
                    MarkSceneChanged(m_EngineContext);
                }
            }

            if (auto *text = dynamic_cast<::UI::UIText *>(m_SelectedElement)) {
                ImGui::Text("Text Properties");
                char textBuf[512] = {};
                std::strncpy(textBuf, text->GetText().c_str(), sizeof(textBuf) - 1);
                if (ImGui::InputTextMultiline("Text", textBuf, sizeof(textBuf), ImVec2(-1, 60))) {
                    text->SetText(textBuf);
                    MarkSceneChanged(m_EngineContext);
                }

                auto textColor = text->GetColor();
                if (ImGui::ColorEdit4("Text Color", &textColor.x)) {
                    text->SetColor(textColor);
                    MarkSceneChanged(m_EngineContext);
                }

                auto font = text->GetFont();
                if (FontCombo("Font", *m_EngineContext->resources, font)) {
                    text->SetFont(font);
                    MarkSceneChanged(m_EngineContext);
                }
            }

            if (auto *btn = dynamic_cast<::UI::UIButton *>(m_SelectedElement)) {
                ImGui::Text("Button Properties");
                char btnTextBuf[256] = {};
                std::strncpy(btnTextBuf, btn->GetText().c_str(), sizeof(btnTextBuf) - 1);
                if (ImGui::InputText("Label", btnTextBuf, sizeof(btnTextBuf))) {
                    btn->SetText(btnTextBuf);
                    MarkSceneChanged(m_EngineContext);
                }

                auto btnColor = btn->GetColor();
                if (ImGui::ColorEdit4("Text Color", &btnColor.x)) {
                    btn->SetColor(btnColor);
                    MarkSceneChanged(m_EngineContext);
                }

                auto bgColor = btn->GetBackgroundColor();
                if (ImGui::ColorEdit4("Background", &bgColor.x)) {
                    btn->SetBackgroundColor(bgColor);
                    MarkSceneChanged(m_EngineContext);
                }

                auto hoverBgColor = btn->GetHoveredBackgroundColor();
                if (ImGui::ColorEdit4("Hovered", &hoverBgColor.x)) {
                    btn->SetHoveredBackgroundColor(hoverBgColor);
                    MarkSceneChanged(m_EngineContext);
                }

                auto font = btn->GetFont();
                if (FontCombo("Font", *m_EngineContext->resources, font)) {
                    btn->SetFont(font);
                    MarkSceneChanged(m_EngineContext);
                }
            }

            if (auto *rect = dynamic_cast<::UI::UIRect *>(m_SelectedElement)) {
                ImGui::Text("Rect Properties");
                auto rectColor = rect->GetColor();
                if (ImGui::ColorEdit4("Color", &rectColor.x)) {
                    rect->SetColor(rectColor);
                    MarkSceneChanged(m_EngineContext);
                }
            }
        }

        ImGui::End();
    }

} // namespace Editor::UI
