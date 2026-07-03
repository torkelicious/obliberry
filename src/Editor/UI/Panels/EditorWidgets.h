#pragma once

#include "Core/EngineContext.h"
#include "ECS/Registry.h"
#include "Scenes/SceneManager.h"
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/TransformComponent.h"

namespace Editor::UI {
    // Helper — marks the current scene as having unsaved changes
    inline void MarkSceneChanged(const Core::EngineContext *ctx) {
        if (ctx && ctx->sceneManager) {
            if (auto *scene = ctx->sceneManager->GetCurrentScene()) {
                scene->MarkAsChanged();
            }
        }
    }

    enum class FieldType : uint8_t { Float, Int, Bool, Vec3, Color3 };

    struct ComponentField {
        const char *Name;
        FieldType Type;
        size_t Offset;
    };

    struct IComponentWidget {
        virtual ~IComponentWidget() = default;

        [[nodiscard]] virtual const char *GetName() const = 0;

        virtual void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) = 0;
    };

    template<typename T>
    class AutoComponentWidget : public IComponentWidget {
    protected:
        const char *m_Name;
        std::vector<ComponentField> m_Fields;

    public:
        explicit AutoComponentWidget(const char *name) : m_Name(name) {
        }

        [[nodiscard]] const char *GetName() const override { return m_Name; }

        void Draw(const ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override {
            if (!entity.HasComponent<T>())
                return;
            if (ImGui::CollapsingHeader(m_Name)) {
                T *component = entity.GetComponent<T>();
                const auto byte_ptr = reinterpret_cast<uint8_t *>(component);

                for (const auto &[Name, Type, Offset]: m_Fields) {
                    void *fieldAddress = byte_ptr + Offset;
                    switch (Type) {
                        case FieldType::Float:
                            ImGui::DragFloat(Name, static_cast<float *>(fieldAddress), 0.1f);
                            break;
                        case FieldType::Int:
                            ImGui::DragInt(Name, static_cast<int *>(fieldAddress));
                            break;
                        case FieldType::Vec3:
                            ImGui::DragFloat3(Name, static_cast<float *>(fieldAddress), 0.1f);
                            break;
                        case FieldType::Color3:
                            ImGui::ColorEdit3(Name, static_cast<float *>(fieldAddress), ImGuiColorEditFlags_NoInputs);
                            break;
                        case FieldType::Bool:
                            ImGui::Checkbox(Name, static_cast<bool *>(fieldAddress));
                            break;
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        MarkSceneChanged(engineContext);
                    }
                }
                DrawExtras(entity, component, engineContext);

                ImGui::Separator();
                const float buttonWidth = ImGui::CalcTextSize((std::string("Remove ") + m_Name).c_str()).x +
                                          ImGui::GetStyle()
                                          .FramePadding.x * 2;
                const float availWidth = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
                if (ImGui::Button((std::string("Remove ##") + m_Name).c_str(), ImVec2(buttonWidth, 0))) {
                    entity.RemoveComponent<T>();
                    MarkSceneChanged(engineContext);
                }
            }
        }

    protected:
        virtual void DrawExtras(ECS::Entity entity, T *component, Core::EngineContext *engineContext) {
        }
    };

    template<typename T>
    class TagWidget : public IComponentWidget {
    protected:
        const char *m_Name;

    public:
        explicit TagWidget(const char *name) : m_Name(name) {
        }

        [[nodiscard]] const char *GetName() const override { return m_Name; }

        void Draw(const ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override {
            if (!entity.HasComponent<T>())
                return;
            if (ImGui::CollapsingHeader(m_Name)) {
                ImGui::TextDisabled("Tag Component (No Data)");
                if (ImGui::Button((std::string("Remove ##") + m_Name).c_str())) {
                    entity.RemoveComponent<T>();
                    MarkSceneChanged(engineContext);
                }
            }
        }
    };

    // Widget Declarations
    struct PointLightWidget : public AutoComponentWidget<ECS::Components::PointLightComponent> {
        PointLightWidget();
    };

    struct TransformWidget : public AutoComponentWidget<ECS::Components::TransformComponent> {
        TransformWidget();

        void DrawExtras(ECS::Entity entity, ECS::Components::TransformComponent *component,
                        Core::EngineContext *engineContext) override;
    };

    struct MovementWidget : public AutoComponentWidget<ECS::Components::MovementComponent> {
        MovementWidget();

        void DrawExtras(ECS::Entity entity, ECS::Components::MovementComponent *component,
                        Core::EngineContext *engineContext) override;
    };

    struct MeshWidget : public IComponentWidget {
        int m_SelectedMesh = 0;

        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override;
    };

    struct MaterialWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override;
    };

    struct DirectionalTextureWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override;
    };

    struct MapWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override;
    };

    struct MapStateWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override;
    };

    struct ScriptWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override;
    };

    struct CustomDataWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity, Core::EngineContext *engineContext = nullptr) override;
    };
} // namespace Editor::UI
