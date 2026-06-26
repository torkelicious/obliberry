#pragma once

#include "ECS/Entity.h"
#include <cstddef>
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/TransformComponent.h"

enum class FieldType : uint8_t { Float, Int, Bool, Vec3, Color3 };

struct ComponentField {
    const char* Name;
    FieldType Type;
    size_t Offset;
};

struct IComponentWidget {
    virtual ~IComponentWidget() = default;
    virtual const char* GetName() const = 0;
    virtual void Draw(Entity entity) = 0;
};

// widget base
template <typename T> class AutoComponentWidget : public IComponentWidget {
protected:
    const char* m_Name;
    std::vector<ComponentField> m_Fields;

public:
    AutoComponentWidget(const char* name) : m_Name(name) {}

    const char* GetName() const override { return m_Name; }

    virtual void Draw(Entity entity) override {
        if (!entity.HasComponent<T>()) {
            return;
        }

        if (ImGui::CollapsingHeader(m_Name, ImGuiTreeNodeFlags_DefaultOpen)) {
            T* component = entity.GetComponent<T>();
            uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(component);

            for (const auto& field : m_Fields) {
                void* fieldAddress = byte_ptr + field.Offset;

                switch (field.Type) {
                case FieldType::Float:
                    ImGui::DragFloat(field.Name, static_cast<float*>(fieldAddress), 0.1f);
                    break;
                case FieldType::Int:
                    ImGui::DragInt(field.Name, static_cast<int*>(fieldAddress));
                    break;
                case FieldType::Vec3:
                    ImGui::DragFloat3(field.Name, static_cast<float*>(fieldAddress), 0.1f);
                    break;
                case FieldType::Color3:
                    ImGui::ColorEdit3(field.Name, static_cast<float*>(fieldAddress));
                    break;
                case FieldType::Bool:
                    ImGui::Checkbox(field.Name, static_cast<bool*>(fieldAddress));
                    break;
                }
            }
            DrawExtras(entity, component);
        }
    }

protected:
    virtual void DrawExtras(Entity entity, T* component) {}
};

// tags
template <typename T> class TagWidget : public IComponentWidget {
protected:
    const char* m_Name;

public:
    TagWidget(const char* name) : m_Name(name) {}

    const char* GetName() const override { return m_Name; }

    virtual void Draw(Entity entity) override {
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

// WIDGET IMPLEMENTATIONS

struct PointLightWidget : public AutoComponentWidget<PointLightComponent> {
    PointLightWidget() : AutoComponentWidget("Point Light") {
        m_Fields.push_back({"Color", FieldType::Color3, offsetof(PointLightComponent, color)});
        m_Fields.push_back({"Radius", FieldType::Float, offsetof(PointLightComponent, radius)});
        m_Fields.push_back({"Intensity", FieldType::Float, offsetof(PointLightComponent, intensity)});
    };
};

struct TransformWidget : public AutoComponentWidget<TransformComponent> {
    TransformWidget() : AutoComponentWidget("Transform") {
        m_Fields.push_back({"Position", FieldType::Vec3, offsetof(TransformComponent, transform.position)});
        m_Fields.push_back({"Rotation", FieldType::Vec3, offsetof(TransformComponent, transform.rotation)});
        m_Fields.push_back({"Scale", FieldType::Vec3, offsetof(TransformComponent, transform.scale)});
    };

    void DrawExtras(Entity entity, TransformComponent* component) override {
        ImGui::Spacing();
        bool hasBillboard = entity.HasComponent<BillboardTagComponent>();
        bool useBillboard = hasBillboard;

        if (ImGui::Checkbox("Use Billboard", &useBillboard)) {
            if (useBillboard && !hasBillboard) {
                entity.AddComponent<BillboardTagComponent>();
            } else if (!useBillboard && hasBillboard) {
                entity.RemoveComponent<BillboardTagComponent>();
            }
        }
    }
};

struct MovementWidget : public AutoComponentWidget<MovementComponent> {
    MovementWidget() : AutoComponentWidget("Movement") {
        m_Fields.push_back({"Time Per Step", FieldType::Float, offsetof(MovementComponent, timePerStep)});
        m_Fields.push_back({"Step Timer", FieldType::Float, offsetof(MovementComponent, stepTimer)});
        m_Fields.push_back({"Idle Timer", FieldType::Float, offsetof(MovementComponent, idleTimer)});
        m_Fields.push_back({"Is Moving", FieldType::Bool, offsetof(MovementComponent, isMoving)});
    };

    void DrawExtras(Entity entity, MovementComponent* component) override {
        ImGui::Text("Path Nodes: %zu", component->currentPath.size());
        ImGui::Text("Current Path Index: %zu", component->currentPathIndex);
    }
};

struct MeshWidget : public IComponentWidget {
    const char* GetName() const override { return "Mesh"; }
    void Draw(Entity entity) override {
        if (!entity.HasComponent<MeshComponent>())
            return;
        if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* comp = entity.GetComponent<MeshComponent>();
            ImGui::Text("Mesh Status: %s", comp->mesh ? "Loaded" : "Empty");
        }
    }
    // TODO: intergrate with meshfactory / assetloader etc
};

struct MaterialWidget : public IComponentWidget {
    const char* GetName() const override { return "Material"; }
    void Draw(Entity entity) override {
        if (!entity.HasComponent<MaterialComponent>())
            return;
        if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* comp = entity.GetComponent<MaterialComponent>();
            ImGui::Text("Material Status: %s", comp->material ? "Assigned" : "Empty");
        }
    }
    // TODO: intergrate properly
};

struct DirectionalTextureWidget : public IComponentWidget {
    const char* GetName() const override { return "Directional Texture"; }
    void Draw(Entity entity) override {
        if (!entity.HasComponent<DirectionalTextureComponent>())
            return;
        if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* comp = entity.GetComponent<DirectionalTextureComponent>();
            ImGui::SliderInt("Facing Index", &comp->index, 0, 5);

            for (int i = 0; i < 6; i++) {
                ImGui::BulletText("Direction %d: %s", i, comp->textures[i] ? "Loaded" : "Empty");
            }
        }
    }
};

struct MapWidget : public IComponentWidget {
    const char* GetName() const override { return "Map"; }
    void Draw(Entity entity) override {
        if (!entity.HasComponent<MapComponent>())
            return;
        if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* comp = entity.GetComponent<MapComponent>();

            char buffer[256];
            strncpy(buffer, comp->mapFilePath.c_str(), sizeof(buffer));
            if (ImGui::InputText("File Path", buffer, sizeof(buffer))) {
                comp->mapFilePath = buffer;
            }

            ImGui::Checkbox("Needs Mesh Update", &comp->needsMeshUpdate);
            ImGui::Text("Render Visibles: %zu types", comp->visibles.size());
            ImGui::Text("Hex Mesh: %s", comp->hexMesh ? "Loaded" : "Missing");
        }
    }
};

struct MapStateWidget : public IComponentWidget {
    const char* GetName() const override { return "Map State"; }
    void Draw(Entity entity) override {
        if (!entity.HasComponent<MapStateComponent>())
            return;
        if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* comp = entity.GetComponent<MapStateComponent>();

            ImGui::Checkbox("Has Selection", &comp->hasSelection);
            if (comp->hasSelection) {
                ImGui::Text("Selected Hex: [%d, %d]", comp->selectedHex.q, comp->selectedHex.r);
            }

            ImGui::Checkbox("Has Path To", &comp->hasPathTo);
            if (comp->hasPathTo) {
                ImGui::Text("Path Target: [%d, %d]", comp->pathTo.q, comp->pathTo.r);
            }
        }
    }
};

struct ScriptWidget : public IComponentWidget {
    const char* GetName() const override { return "Scripts"; }
    void Draw(Entity entity) override {
        if (!entity.HasComponent<ScriptComponent>())
            return;
        if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* comp = entity.GetComponent<ScriptComponent>();

            ImGui::Text("Attached Scripts: %zu", comp->scriptPaths.size());
            ImGui::Separator();
            for (size_t i = 0; i < comp->scriptPaths.size(); i++) {
                ImGui::BulletText("%s", comp->scriptPaths[i].c_str());
                ImGui::Indent();
                ImGui::TextDisabled("Initialized: %s", comp->isInitialized[i] ? "Yes" : "No");
                ImGui::Unindent();
            }
        }
    }
};

struct CustomDataWidget : public IComponentWidget {
    const char* GetName() const override { return "ObSL Custom Data"; }
    void Draw(Entity entity) override {
        if (!entity.HasComponent<CustomDataComponent>())
            return;
        if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* comp = entity.GetComponent<CustomDataComponent>();

            if (comp->script_components.empty()) {
                ImGui::TextDisabled("No script variables defined.");
                return;
            }

            for (auto& [varName, value] : comp->script_components) {
                ImGui::BulletText("%s", varName.c_str());
            }
        }
    }
};
