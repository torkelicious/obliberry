#include "EditState.h"
#include "ECS/Systems/LightingSystem.h"
#include "Applications/Editor/EditorLayer.h"
#include "Platform/Input/InputManager.h"
#include "Sound/AudioEngine.h"
#include <glm/gtc/type_ptr.hpp>
#include "ECS/Components/BillboardTagComponent.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "ECS/Systems/ParticleSystem.h"
#include "UI/UIGizmo.h"

namespace Editor::States {
    ImGuizmo::OPERATION EditState::mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE EditState::mCurrentGizmoMode = ImGuizmo::WORLD;
} // namespace Editor::States

void Editor::States::EditState::OnEnter() {
    if (!m_EditorLayer || !m_EditorLayer->m_Context->projectConfig)
        return;

    std::string title = "Obliberry: " + m_EditorLayer->m_Context->projectConfig->Title;
    if (m_EditorLayer->m_Scene)
        title += " - Scene - " + m_EditorLayer->m_CurrentScenePath;
    SetWindowTitle(title);
}

void Editor::States::EditState::OnUpdate(const float dt) {
    // Editor mode only runs lighting for accurate scene view
    // full scene update is for play
    ECS::Systems::LightingSystem::Update(*m_EditorLayer->m_Registry);
    if (EditorLayer::s_RenderParticlesInEditor) {
        ECS::Systems::ParticleSystem::Update(*m_EditorLayer->m_Registry, dt);
    }
    if (m_EditorLayer->m_Context->uiSystem) {
        m_EditorLayer->m_Context->uiSystem->Update(dt, /*interactive=*/false);
        if (m_EditorLayer->m_CurrentState && m_EditorLayer->m_CurrentState->IsPlayMode()) {
            m_EditorLayer->m_Context->uiSystem->SnapshotButtonStates();
        }
        if (m_EditorLayer->m_Context->uiCmdBuf) {
            if (m_UIDragHandle == ::UI::HandleType::None) {
                m_EditorLayer->m_Context->uiCmdBuf->flush(*m_EditorLayer->m_Context->uiSystem);
            }
        }
    }

    // Entity picking
    if (const int clickedID = m_EditorLayer->m_ViewportPanel.GetSelectedEntityID(); clickedID != -1) {
        if (const auto eID = static_cast<ECS::EntityID>(clickedID); m_EditorLayer->m_Registry->IsValid(eID)) {
            const ECS::Entity selectedEntity(eID, m_EditorLayer->m_Registry);
            m_EditorLayer->m_RegistryPanel.SetSelectedEntity(selectedEntity);
        }
    } else if (m_EditorLayer->m_ViewportPanel.HadEmptyClick()) {
        m_EditorLayer->m_RegistryPanel.SetSelectedEntity(ECS::Entity{});
        m_EditorLayer->m_ViewportPanel.ClearEmptyClick();
    }
    m_EditorLayer->m_ViewportPanel.ClearSelectedEntityID();
}

void Editor::States::EditState::OnHandleInput(const float dt) {
    // must run before the WantCaptureKeyboard earlyout below
    UI_HandleGizmoInput();

    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (m_EditorLayer->m_Input->IsKeyPressed("V")) {
        m_EditorLayer->m_Camera.ToggleViewMode();
    }

    // Gizmo operation shortcuts
    if (m_EditorLayer->m_Input->IsKeyPressed("T")) {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    if (m_EditorLayer->m_Input->IsKeyPressed("R")) {
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    }
    if (m_EditorLayer->m_Input->IsKeyPressed("E")) {
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    }

    float kbPanX = 0.0f;
    float kbPanY = 0.0f;
    m_EditorLayer->m_Camera.StopKeyboardPan();

    // Camera controls require viewport hover to avoid stealing input
    if (!m_EditorLayer->m_ViewportPanel.IsHovered())
        return;

    // Scroll zoom
    if (const auto scrollDelta = static_cast<float>(m_EditorLayer->m_Input->ScrollY()); scrollDelta != 0.0f) {
        m_EditorLayer->m_Camera.AdjustZoom(scrollDelta * 0.2f);
    }

    // Mouse pan
    const auto mouseDeltaX = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaX());
    const auto mouseDeltaY = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaY());

    if ((m_EditorLayer->m_Input->IsMouseDown("MouseMiddle") || m_EditorLayer->m_Input->IsMouseDown("MouseRight")) && m_UIDragHandle == ::UI::HandleType::None) {
        m_EditorLayer->m_Camera.Pan(-mouseDeltaX, mouseDeltaY, 0.025f);
    }

    // Keyboard pan
    if (m_EditorLayer->m_Input->IsKeyDown("W"))
        kbPanY += 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("S"))
        kbPanY -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("A"))
        kbPanX -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("D"))
        kbPanX += 1.0f;

    if (kbPanX != 0.0f || kbPanY != 0.0f) {
        const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
        kbPanX /= length;
        kbPanY /= length;
    }

    const float speedMod = m_EditorLayer->m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
    constexpr float moveAmount = 3.0f;
    const float vpHeight = m_EditorLayer->m_ViewportPanel.GetHeight();
    m_EditorLayer->m_Camera.KeyboardPan(kbPanX, kbPanY, 15.0f * speedMod * moveAmount * (600.0f / vpHeight));
}

void Editor::States::EditState::OnDrawPanels() {
    m_EditorLayer->m_RegistryPanel.OnImGuiRender();
    m_EditorLayer->m_InspectorPanel.OnImGuiRender();
    m_EditorLayer->m_UIPanel.OnImGuiRender();
    Entity_DrawGizmoForSelected();
}

void Editor::States::EditState::OnRender() {
    ImGuizmo::BeginFrame();
    m_EditorLayer->DrawEditorLayout();
    if (EditorLayer::s_RenderParticlesInEditor) {
        ECS::Systems::ParticleSystem::Render(*m_EditorLayer->m_Registry, *m_EditorLayer->m_Context->renderer, &m_EditorLayer->m_Camera);
    }
    UI_DrawGizmoForSelected();
}

void Editor::States::EditState::OnDrawModeToolbar() {
    const ImGuizmo::OPERATION currentOp = GetGizmoOperation();

    auto gizmoButton = [&](const char *label, const ImGuizmo::OPERATION op, const char *tooltip, const ImVec4 &activeColor) {
        const bool isActive = currentOp == op;
        if (isActive)
            ImGui::PushStyleColor(ImGuiCol_Text, activeColor);
        if (ImGui::Button(label)) {
            SetGizmoOperation(op);
        }
        if (isActive)
            ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip);
    };

    gizmoButton("T##Gizmo", ImGuizmo::TRANSLATE, "Translate (T)", ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
    ImGui::SameLine();
    gizmoButton("R##Gizmo", ImGuizmo::ROTATE, "Rotate (R)", ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
    ImGui::SameLine();
    gizmoButton("S##Gizmo", ImGuizmo::SCALE, "Scale (E)", ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
}

void Editor::States::EditState::OnSaveKey() {
    if (CanSaveScene()) {
        m_EditorLayer->SaveScene();
    }
}

//
// Entity transofmation
//
void Editor::States::EditState::Entity_DrawGizmoForSelected() const {
    const ECS::Entity selectedEntity = m_EditorLayer->m_RegistryPanel.GetSelectedEntity();
    if (!selectedEntity)
        return;
    if (!selectedEntity.HasComponent<ECS::Components::TransformComponent>())
        return;

    Rendering::Transform &t = selectedEntity.GetComponent<ECS::Components::TransformComponent>()->transform;
    Rendering::Transform &worldT = selectedEntity.GetComponent<ECS::Components::TransformComponent>()->worldTransform;
    const bool isBillboard = selectedEntity.HasComponent<ECS::Components::BillboardTagComponent>();

    if (isBillboard && mCurrentGizmoOperation == ImGuizmo::ROTATE) {
        ImGui::Begin("Scene View");
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        constexpr auto warningText = " Rotation has no effect on Billboard Sprites ";
        const ImVec2 textSize = ImGui::CalcTextSize(warningText);
        constexpr float padding = 6.0f;

        const ImVec2 windowPos = ImGui::GetWindowPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 badgeMin(windowPos.x + (windowSize.x - textSize.x) * 0.5f, windowPos.y + 40.0f);
        const ImVec2 badgeMax(badgeMin.x + textSize.x + padding * 2.0f, badgeMin.y + textSize.y + padding * 2.0f);

        drawList->AddRectFilled(badgeMin, badgeMax, IM_COL32(200, 150, 20, 220), 4.0f);
        drawList->AddText(ImVec2(badgeMin.x + padding, badgeMin.y + padding), IM_COL32(255, 255, 255, 255), warningText);
        ImGui::End();
    }
    const_cast<EditState *>(this)->EntityGizmoTranslate(t, worldT, isBillboard);
}

void Editor::States::EditState::EntityGizmoTranslate(Rendering::Transform &localTransform, Rendering::Transform &worldTransform, bool isBillboard) {
    const auto &camera = m_EditorLayer->m_Camera;
    const float aspect = m_EditorLayer->m_ViewportPanel.GetWidth() / m_EditorLayer->m_ViewportPanel.GetHeight();

    glm::mat4 gizmoViewMatrix = camera.GetRotation() * camera.GetViewMatrix();
    gizmoViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -50.0f)) * gizmoViewMatrix;

    const glm::mat4 projMatrix = camera.GetProjectionMatrix(aspect);

    glm::mat4 gizmoMatrix;

    if (isBillboard) {
        const glm::vec3 right = camera.GetRightVector();
        const glm::vec3 up = camera.GetUpVector();
        const glm::vec3 forward = glm::cross(right, up);

        gizmoMatrix = glm::mat4(1.0f);
        gizmoMatrix[0] = glm::vec4(right * worldTransform.GetScale().x, 0.0f);
        gizmoMatrix[1] = glm::vec4(up * worldTransform.GetScale().y, 0.0f);
        gizmoMatrix[2] = glm::vec4(forward * worldTransform.GetScale().z, 0.0f);
        gizmoMatrix[3] = glm::vec4(worldTransform.GetPosition(), 1.0f);
    } else {
        gizmoMatrix = worldTransform.GetMatrix();
    }

    auto deltaMatrix = glm::mat4(1.0f);
    ImGuizmo::SetOrthographic(true);

    ImGuizmo::MODE actualMode = mCurrentGizmoMode;

    if (mCurrentGizmoOperation == ImGuizmo::SCALE) {
        actualMode = ImGuizmo::LOCAL;
    }

    ImGuizmo::Manipulate(glm::value_ptr(gizmoViewMatrix), glm::value_ptr(projMatrix), mCurrentGizmoOperation, actualMode, glm::value_ptr(gizmoMatrix), glm::value_ptr(deltaMatrix));

    if (ImGuizmo::IsUsing()) {
        // just started dragging
        if (!m_GizmoDragging) {
            m_GizmoDragging = true;
            m_GizmoStartPos = localTransform.GetPosition();
            m_GizmoStartRot = localTransform.GetRotation();
            m_GizmoStartScale = localTransform.GetScale();
        }

        if (auto *scene = m_EditorLayer->m_Scene) {
            scene->MarkAsChanged();
        }

        if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE) {
            const auto deltaPos = glm::vec3(deltaMatrix[3]);
            localTransform.SetPosition(localTransform.GetPosition() + deltaPos);
        } else if (mCurrentGizmoOperation == ImGuizmo::ROTATE) {
            float deltaTranslation[3], deltaRotation[3], deltaScale[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(deltaMatrix), deltaTranslation, deltaRotation, deltaScale);

            glm::vec3 currentRot = localTransform.GetRotation();
            glm::vec3 dRot = glm::radians(glm::vec3(deltaRotation[0], deltaRotation[1], deltaRotation[2]));
            localTransform.SetRotation(currentRot + dRot);
        } else if (mCurrentGizmoOperation == ImGuizmo::SCALE) {
            if (isBillboard) {
                float scaleX = glm::length(glm::vec3(gizmoMatrix[0]));
                float scaleY = glm::length(glm::vec3(gizmoMatrix[1]));
                float scaleZ = glm::length(glm::vec3(gizmoMatrix[2]));

                localTransform.SetScale(glm::vec3(scaleX, scaleY, scaleZ));
            } else {
                float deltaTranslation[3], deltaRotation[3], deltaScale[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(deltaMatrix), deltaTranslation, deltaRotation, deltaScale);

                glm::vec3 currentScale = localTransform.GetScale();
                auto dScale = glm::vec3(deltaScale[0], deltaScale[1], deltaScale[2]);
                localTransform.SetScale(currentScale * dScale);
            }
        }
    } else {
        //  we stopped using gizmo, a drag was probably just completed
        if (m_GizmoDragging) {
            m_GizmoDragging = false;

            if (const ECS::Entity selectedEntity = m_EditorLayer->m_RegistryPanel.GetSelectedEntity()) {
                const auto entId = static_cast<ECS::EntityID>(selectedEntity);
                if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE) {
                    if (m_GizmoStartPos != localTransform.GetPosition()) {
                        m_EditorLayer->m_UndoManager.Execute(std::make_unique<Commands::TranslateEntityCommand>(entId, m_GizmoStartPos, localTransform.GetPosition()), *m_EditorLayer->m_Context);
                    }
                } else if (mCurrentGizmoOperation == ImGuizmo::ROTATE) {
                    if (m_GizmoStartRot != localTransform.GetRotation()) {
                        m_EditorLayer->m_UndoManager.Execute(std::make_unique<Commands::RotateEntityCommand>(entId, m_GizmoStartRot, localTransform.GetRotation()), *m_EditorLayer->m_Context);
                    }
                } else if (mCurrentGizmoOperation == ImGuizmo::SCALE) {
                    if (m_GizmoStartScale != localTransform.GetScale()) {
                        m_EditorLayer->m_UndoManager.Execute(std::make_unique<Commands::ScaleEntityCommand>(entId, m_GizmoStartScale, localTransform.GetScale()), *m_EditorLayer->m_Context);
                    }
                }
            }
        }
    }
}


//
// UI Transformation
//
glm::vec2 Editor::States::EditState::GetUIGizmoMousePos() const {
    const ImVec2 local = m_EditorLayer->m_ViewportPanel.GetLocalMousePos();
    return m_EditorLayer->m_Context->uiRenderer->WindowToGameCoords(local.x, local.y, m_EditorLayer->m_ViewportPanel.GetWidth(), m_EditorLayer->m_ViewportPanel.GetHeight());
}

void Editor::States::EditState::UI_HandleGizmoInput() {
    auto *element = m_EditorLayer->m_UIPanel.GetSelectedElement();
    if (!element || !m_EditorLayer->m_Context->uiRenderer)
        return;

    const bool viewportHovered = m_EditorLayer->m_ViewportPanel.IsHovered();

    m_UIHoveredHandle = ::UI::HandleType::None;
    if (viewportHovered && m_UIDragHandle == ::UI::HandleType::None) {
        const glm::vec2 hoverPos = GetUIGizmoMousePos();
        m_UIHoveredHandle = ::UI::HitTest(hoverPos, element);
        if (m_UIHoveredHandle != ::UI::HandleType::None && m_UIHoveredHandle != ::UI::HandleType::Translate)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    m_EditorLayer->m_ViewportPanel.SetUIHandleHover(m_UIHoveredHandle != ::UI::HandleType::None || m_UIDragHandle != ::UI::HandleType::None);

    if (!viewportHovered && m_UIDragHandle == ::UI::HandleType::None)
        return;

    const glm::vec2 mousePos = GetUIGizmoMousePos();

    if (viewportHovered && m_EditorLayer->m_Input->IsMousePressed("MouseLeft")) {
        m_UIDragHandle = ::UI::HitTest(mousePos, element);
        if (m_UIDragHandle != ::UI::HandleType::None) {
            m_UIDragStartMouse = mousePos;
            m_UIDragStartWorldPos = ::UI::GetWorldPosition(element);
            m_UIDragStartScale = element->Rect.Scale;
            m_UIDragStarted = false;
        }
        return;
    }

    // empty click
    if (viewportHovered && m_EditorLayer->m_Input->IsMouseReleased("MouseLeft") && m_UIDragHandle == ::UI::HandleType::None) {
        if (auto *uiSys = m_EditorLayer->m_Context->uiSystem) {
            if (auto *hit = uiSys->HitTest(mousePos); hit && hit != uiSys->GetRoot()) {
                m_EditorLayer->m_UIPanel.SetSelectedElement(hit);
            } else {
                m_EditorLayer->m_UIPanel.SetSelectedElement(nullptr);
            }
        }
        return;
    }

    if (m_UIDragHandle != ::UI::HandleType::None && m_EditorLayer->m_Input->IsMouseDown("MouseLeft")) {
        glm::vec2 delta = mousePos - m_UIDragStartMouse;

        // drag
        constexpr float UI_TRANSLATE_DEADZONE = 2.0f;
        if (m_UIDragHandle == ::UI::HandleType::Translate && !m_UIDragStarted && glm::length(delta) < UI_TRANSLATE_DEADZONE)
            return;
        m_UIDragStarted = true;

        ::UI::TransformElement(element, m_UIDragHandle, delta, m_UIDragStartWorldPos, m_UIDragStartScale);

        if (auto *scene = m_EditorLayer->m_Scene)
            scene->MarkAsChanged();
        return;
    }

    // release
    if (m_UIDragHandle != ::UI::HandleType::None && m_EditorLayer->m_Input->IsMouseReleased("MouseLeft")) {
        const glm::vec2 parentWorld = element->Parent ? ::UI::GetWorldPosition(element->Parent) : glm::vec2(0.0f);
        const glm::vec2 startPosLocal = m_UIDragStartWorldPos - parentWorld;

        if ((startPosLocal != element->Rect.Position || m_UIDragStartScale != element->Rect.Scale) && m_EditorLayer->m_Scene) {
            m_EditorLayer->m_UndoManager.Execute(std::make_unique<Commands::TransformUIElementCommand>(element, startPosLocal, element->Rect.Position, m_UIDragStartScale, element->Rect.Scale), *m_EditorLayer->m_Context);
        }

        m_UIDragHandle = ::UI::HandleType::None;
        m_UIDragStarted = false;
    }
}

void Editor::States::EditState::UI_DrawGizmoForSelected() const {
    if (!m_EditorLayer->m_Context->uiRenderer || !m_EditorLayer->m_UIPanel.GetSelectedElement()) {
        return;
    }
    if (m_EditorLayer->m_UIPanel.GetSelectedElement()) {
        ::UI::DrawGizmo(m_EditorLayer->m_UIPanel.GetSelectedElement(), m_EditorLayer->m_Context->uiRenderer, m_UIHoveredHandle);
    }
}
