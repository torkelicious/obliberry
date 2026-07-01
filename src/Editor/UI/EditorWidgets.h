#pragma once

#include "ECS/Entity.h"
#include "ECS/Registry.h"
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
    enum class FieldType : uint8_t { Float, Int, Bool, Vec3, Color3 };

    struct ComponentField {
        const char *Name;
        FieldType Type;
        size_t Offset;
    };

    struct IComponentWidget {
        virtual ~IComponentWidget() = default;

        [[nodiscard]] virtual const char *GetName() const = 0;

        virtual void Draw(ECS::Entity entity) = 0;
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

        void Draw(const ECS::Entity entity) override {
            if (!entity.HasComponent<T>())
                return;
            if (ImGui::CollapsingHeader(m_Name, ImGuiTreeNodeFlags_DefaultOpen)) {
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
                            ImGui::ColorEdit3(Name, static_cast<float *>(fieldAddress));
                            break;
                        case FieldType::Bool:
                            ImGui::Checkbox(Name, static_cast<bool *>(fieldAddress));
                            break;
                    }
                }
                DrawExtras(entity, component);
            }
        }

    protected:
        virtual void DrawExtras(ECS::Entity entity, T *component) {
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

        void Draw(const ECS::Entity entity) override {
            if (!entity.HasComponent<T>())
                return;
            if (ImGui::CollapsingHeader(m_Name, ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextDisabled("Tag Component (No Data)");
                if (ImGui::Button((std::string("Remove ") + m_Name).c_str())) {
                    entity.RemoveComponent<T>();
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

        void DrawExtras(ECS::Entity entity, ECS::Components::TransformComponent *component) override;
    };

    struct MovementWidget : public AutoComponentWidget<ECS::Components::MovementComponent> {
        MovementWidget();

        void DrawExtras(ECS::Entity entity, ECS::Components::MovementComponent *component) override;
    };

    struct MeshWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity) override;
    };

    struct MaterialWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity) override;
    };

    struct DirectionalTextureWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity) override;
    };

    struct MapWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity) override;
    };

    struct MapStateWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity) override;
    };

    struct ScriptWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity) override;
    };

    struct CustomDataWidget : public IComponentWidget {
        [[nodiscard]] const char *GetName() const override;

        void Draw(ECS::Entity entity) override;
    };
} // namespace Editor::UI
