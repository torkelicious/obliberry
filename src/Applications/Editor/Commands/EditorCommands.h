#pragma once
#include "ICommand.h"
#include "Applications/Editor/EditorContext.h"
#include "Config/ProjectConfig.h"
#include "Platform/Window/Window.h"
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <cstring>
#include <vector>
#include <cstddef>
#include <string>
#include <utility>
#include <stdexcept>
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include "ECS/Components/ScriptComponent.h"
#include "Map/Hex.h"
#include "Map/HexCoords.h"
#include "Rendering/PostProcessing/PostProcessing.h"
#include "Scenes/SceneManager.h"
#include "UI/UIElement.h"
#include "UI/Rendering/UISystem.h"
#include "Rendering/Types/Texture/Texture.h"
#include "UI/Text/Font.h"

namespace Editor::Commands {

    // Entity IDs change when deletion is undone; commands retain the UUID instead.
    class EntityCommand : public ICommand {
    public:
        explicit EntityCommand(std::string targetUUID) : m_EntityUUID(std::move(targetUUID)) {}
        [[nodiscard]] bool Succeeded() const noexcept override { return m_Succeeded; }

    protected:
        [[nodiscard]] ECS::Entity ResolveEntity(Core::EngineContext &ctx);
        void Complete(Core::EngineContext &ctx);
        std::string m_EntityUUID;
        bool m_Succeeded = false;
    };

    struct EntitySubtreeSnapshot;

    // hack but whatever
    static void RefreshWindowTitle(const Core::EngineContext &ctx) {
        if (!ctx.window || !ctx.projectConfig)
            return;
        std::string title = "Obliberry: " + ctx.projectConfig->Title;
        if (ctx.sceneManager)
            if (auto *scene = ctx.sceneManager->GetCurrentScene())
                title += " - Scene - " + scene->GetProperties().ScenePath;
        ctx.window->SetWindowTitle(title);
    }

    // = = = = = //
    // TRANSFORM //
    // = = = = = //

    //
    // Move
    //

    class TranslateEntityCommand final : public EntityCommand {
    public:
        // i could make this a unified transform command
        // but transforms are much larger than just the vec3's they hold sooo...
        // optimization ig :DDDD
        TranslateEntityCommand(std::string targetUUID, glm::vec3 oldPos, glm::vec3 newPos);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        glm::vec3 m_OldPos;
        glm::vec3 m_NewPos;
    };

    //
    // Rotate
    //

    class RotateEntityCommand final : public EntityCommand {
    public:
        RotateEntityCommand(std::string targetUUID, glm::vec3 oldRot, glm::vec3 newRot);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        glm::vec3 m_OldRot;
        glm::vec3 m_NewRot;
    };

    //
    // Scale
    //

    class ScaleEntityCommand final : public EntityCommand {
    public:
        ScaleEntityCommand(std::string targetUUID, glm::vec3 oldScale, glm::vec3 newScale);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        glm::vec3 m_OldScale;
        glm::vec3 m_NewScale;
    };

    // = = = = = //
    // Entity   //
    // = = = = = //

    // Rename Entity
    class SetNameCommand final : public EntityCommand {
    public:
        SetNameCommand(std::string targetUUID, std::string oldName, std::string newName);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        std::string m_OldName;
        std::string m_NewName;
    };

    // = = = =  //
    // Registry //
    // = = = =  //
    /* ( or scene in general ) */

    // Remove Component
    template <typename T> class RemoveComponentCommand final : public EntityCommand {
    public:
        RemoveComponentCommand(std::string targetUUID, const T &componentData) : EntityCommand(std::move(targetUUID)), m_OldData(componentData) {}
        void Execute(Core::EngineContext &ctx) override {
            const auto entity = ResolveEntity(ctx);
            if (!entity || !entity.template HasComponent<T>()) {
                return;
            }
            entity.template RemoveComponent<T>();
            Complete(ctx);
        }
        void Undo(Core::EngineContext &ctx) override {
            auto entity = ResolveEntity(ctx);
            if (!entity || entity.template HasComponent<T>()) {
                return;
            }
            entity.template AddComponent<T>(m_OldData);
            Complete(ctx);
        }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Remove Component"; }

    private:
        T m_OldData;
    };

    // Add Component
    template <typename T> class AddComponentCommand final : public EntityCommand {
    public:
        AddComponentCommand(std::string targetUUID, const T &componentData) : EntityCommand(std::move(targetUUID)), m_Data(componentData) {}
        void Execute(Core::EngineContext &ctx) override {
            auto entity = ResolveEntity(ctx);
            if (!entity || entity.template HasComponent<T>()) {
                return;
            }
            entity.template AddComponent<T>(m_Data);
            Complete(ctx);
        }
        void Undo(Core::EngineContext &ctx) override {
            const auto entity = ResolveEntity(ctx);
            if (!entity || !entity.template HasComponent<T>()) {
                return;
            }
            entity.template RemoveComponent<T>();
            Complete(ctx);
        }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Add Component"; }

    private:
        T m_Data;
    };


    // SCRIPT COMPONENT HAS ITS OWN HANDLER:
    class RemoveScriptCommand : public EntityCommand {
    public:
        RemoveScriptCommand(std::string targetUUID, int index);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        int m_Index;
        bool m_ComponentRemoved = false;
        // per-entry data
        std::string m_SavedPath;
        std::filesystem::path m_SavedResolvedPath;
        std::string m_SavedSourceCode;
        std::filesystem::file_time_type m_SavedLastModified;
    };


    class AddScriptCommand : public EntityCommand {
    public:
        AddScriptCommand(std::string targetUUID, const std::string &script_path);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        std::string m_PendingPath;
        std::size_t m_Index = 0;
        bool m_Captured = false;
        bool m_ComponentCreated = false;
    };

    // Scene Config
    class UpdateScenePropertiesCommand : public ICommand {
    public:
        UpdateScenePropertiesCommand(const Scenes::SceneProperties &oldCfg, const Scenes::SceneProperties &newCfg);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        Scenes::SceneProperties m_OldData;
        Scenes::SceneProperties m_NewData;
    };


    // TODO:
    //  Entity Deletion
    //  veri hard because ecs purges dead entities -.-
    //  Maybe use some sort of "shadow delete" idk
    class DeleteEntityCommand final : public EntityCommand {
    public:
        explicit DeleteEntityCommand(std::string targetUUID);
        ~DeleteEntityCommand() override;
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override { return "Delete entity"; }

    private:
        std::unique_ptr<EntitySubtreeSnapshot> m_Snapshot;
    };

    // = = = = //
    // Project //
    // = = = = //

    //
    // Project config
    //
    class ProjectConfigUpdateCommand : public ICommand {
    public:
        ProjectConfigUpdateCommand(const Config::ProjectConfig &oldCfg, const Config::ProjectConfig &newCfg);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        Config::ProjectConfig m_OldData;
        Config::ProjectConfig m_NewData;
    };

    // = = //
    // Map //
    // = = //

    // Paint / Erase
    class MapChangeTileCommand : public EntityCommand {
    public:
        using TileState = std::pair<uint8_t, bool>; // {type, walkable}
        using StateMap = std::unordered_map<Map::HexCoords, std::optional<TileState>, Map::HexCoordsHash>;

        MapChangeTileCommand(StateMap oldState, StateMap newState, std::string targetUUID);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        void ApplyStates(Core::EngineContext &ctx, const StateMap &states);
        StateMap m_OldState;
        StateMap m_NewState;
    };

    // = = = = = //
    // Graphics  //
    // = = = = = //

    // Cfg
    class GraphicsConfigUpdateCommand : public ICommand {
    public:
        GraphicsConfigUpdateCommand(const Config::GraphicsConfig &oldCfg, const Config::GraphicsConfig &newCfg);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        Config::GraphicsConfig m_OldData;
        Config::GraphicsConfig m_NewData;
    };

    // = = = = = //
    //   UI    //
    // = = = = = //

    struct UIElementSnapshot {
        enum Type : uint8_t { RECT, TEXT, BUTTON, IMAGE };
        Type type = RECT;
        std::string name;
        glm::vec2 position{0.0f};
        glm::vec2 scale{0.0f};
        uint8_t flags = ::UI::VISIBLE | ::UI::ENABLED;
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        // UIText / UIButton
        std::string text;
        std::shared_ptr<::UI::Font> font;
        // UIButton
        glm::vec4 bgColor{0.2f, 0.2f, 0.2f, 1.0f};
        glm::vec4 hoverBgColor{0.1f, 0.1f, 0.1f, 1.0f};
        std::shared_ptr<Rendering::Texture> bgTexture;
        // UIImage
        std::shared_ptr<Rendering::Texture> image;
    };

    std::unique_ptr<::UI::UIElement> CreateUIElementFromSnapshot(const UIElementSnapshot &snap);

    UIElementSnapshot SnapshotUIElement(const ::UI::UIElement *element);

    class AddUIElementCommand final : public ICommand {
    public:
        AddUIElementCommand(::UI::UISystem *sys, ::UI::UIElement *parent, UIElementSnapshot snapshot);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override { return "Add UI Element"; }
        [[nodiscard]] ::UI::UIElement *GetCreated() const { return m_Created; }

    private:
        ::UI::UISystem *m_UISystem;
        ::UI::UIElement *m_Parent;
        UIElementSnapshot m_Snapshot;
        ::UI::UIElement *m_Created = nullptr;
    };

    class RemoveUIElementCommand final : public ICommand {
    public:
        RemoveUIElementCommand(::UI::UISystem *sys, ::UI::UIElement *parent, const ::UI::UIElement *child);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override { return "Remove UI Element"; }

    private:
        ::UI::UISystem *m_UISystem;
        ::UI::UIElement *m_Parent;
        UIElementSnapshot m_Snapshot;
        ::UI::UIElement *m_Restored = nullptr;
        ::UI::UIElement *m_RemovedChild = nullptr;
    };

    class SetUIElementNameCommand final : public ICommand {
    public:
        SetUIElementNameCommand(::UI::UIElement *target, std::string oldName, std::string newName) : m_Target(target), m_OldName(std::move(oldName)), m_NewName(std::move(newName)) {}
        void Execute(Core::EngineContext &ctx) override { m_Target->Name = m_NewName; }
        void Undo(Core::EngineContext &ctx) override { m_Target->Name = m_OldName; }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Rename UI Element"; }

    private:
        ::UI::UIElement *m_Target;
        std::string m_OldName;
        std::string m_NewName;
    };

    class SetUIElementPositionCommand final : public ICommand {
    public:
        SetUIElementPositionCommand(::UI::UIElement *target, const glm::vec2 oldPos, const glm::vec2 newPos) : m_Target(target), m_OldPos(oldPos), m_NewPos(newPos) {}
        void Execute(Core::EngineContext &ctx) override { m_Target->Rect.Position = m_NewPos; }
        void Undo(Core::EngineContext &ctx) override { m_Target->Rect.Position = m_OldPos; }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Move UI Element"; }

    private:
        ::UI::UIElement *m_Target;
        glm::vec2 m_OldPos;
        glm::vec2 m_NewPos;
    };

    class SetUIElementScaleCommand final : public ICommand {
    public:
        SetUIElementScaleCommand(::UI::UIElement *target, const glm::vec2 oldScale, const glm::vec2 newScale) : m_Target(target), m_OldScale(oldScale), m_NewScale(newScale) {}
        void Execute(Core::EngineContext &ctx) override { m_Target->Rect.Scale = m_NewScale; }
        void Undo(Core::EngineContext &ctx) override { m_Target->Rect.Scale = m_OldScale; }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Resize UI Element"; }

    private:
        ::UI::UIElement *m_Target;
        glm::vec2 m_OldScale;
        glm::vec2 m_NewScale;
    };

    class TransformUIElementCommand final : public ICommand {
    public:
        TransformUIElementCommand(::UI::UIElement *target, const glm::vec2 oldPos, const glm::vec2 newPos, const glm::vec2 oldScale, const glm::vec2 newScale)
            : m_Target(target), m_OldPos(oldPos), m_NewPos(newPos), m_OldScale(oldScale), m_NewScale(newScale) {}
        void Execute(Core::EngineContext &ctx) override {
            m_Target->Rect.Position = m_NewPos;
            m_Target->Rect.Scale = m_NewScale;
        }
        void Undo(Core::EngineContext &ctx) override {
            m_Target->Rect.Position = m_OldPos;
            m_Target->Rect.Scale = m_OldScale;
        }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Transform UI Element"; }

    private:
        ::UI::UIElement *m_Target;
        glm::vec2 m_OldPos;
        glm::vec2 m_NewPos;
        glm::vec2 m_OldScale;
        glm::vec2 m_NewScale;
    };

    // = = = = = = = = = = = = = = //
    // Generic Component Field Edit //
    // = = = = = = = = = = = = = = //

    // A type-erased command that stores old/new bytes at a given offset inside a component.
    // Used by AutoComponentWidget to make generic field edits (DragFloat, DragInt, etc.) undoable.
    template <typename T> class ModifyComponentFieldCommand final : public EntityCommand {
    public:
        ModifyComponentFieldCommand(std::string targetUUID, const size_t offset, const size_t fieldSize, const void *oldData, const void *newData, std::string fieldName)
            : EntityCommand(std::move(targetUUID)), m_Offset(offset), m_FieldSize(fieldSize), m_FieldName(std::move(fieldName)) {
            if (!oldData || !newData || offset > sizeof(T) || fieldSize > sizeof(T) - offset) {
                throw std::invalid_argument("Invalid component field range");
            }
            const auto *src = static_cast<const uint8_t *>(oldData);
            m_OldData.assign(src, src + fieldSize);
            src = static_cast<const uint8_t *>(newData);
            m_NewData.assign(src, src + fieldSize);
        }

        void Execute(Core::EngineContext &ctx) override { Apply(ctx, m_NewData); }

        void Undo(Core::EngineContext &ctx) override { Apply(ctx, m_OldData); }

        [[nodiscard]] std::string_view Name() const noexcept override { return m_FieldName; }

    private:
        void Apply(Core::EngineContext &ctx, const std::vector<uint8_t> &data) {
            const auto entity = ResolveEntity(ctx);
            if (!entity) {
                return;
            }
            if (auto *comp = entity.template GetComponent<T>()) {
                std::memcpy(reinterpret_cast<uint8_t *>(comp) + m_Offset, data.data(), m_FieldSize);
                Complete(ctx);
            }
        }
        size_t m_Offset;
        size_t m_FieldSize;
        std::string m_FieldName;
        std::vector<uint8_t> m_OldData;
        std::vector<uint8_t> m_NewData;
    };

    // = = = //
    // Theme //
    // = = = //

    class ThemeUpdateCommand : public ICommand {
    public:
        ThemeUpdateCommand(const UI::Theme::Theme &oldCfg, const UI::Theme::Theme &newCfg, const UI::Theme::FontSet &oldSet, const UI::Theme::FontSet &newSet, EditorContext &eCtx);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        EditorContext *m_EditorCtx;
        UI::Theme::Theme m_OldData;
        UI::Theme::Theme m_NewData;

        UI::Theme::FontSet m_OldSet;
        UI::Theme::FontSet m_NewSet;
    };

    // = = = = = = = = = = = //
    // Post Processor editor //
    // = = = = = = = = = = = //
    class PostProcUpdateCommand : public ICommand {
    public:
        PostProcUpdateCommand(const std::vector<Rendering::PostProcessing::PostEffect> &oldFx, const std::vector<Rendering::PostProcessing::PostEffect> &newFx);

        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        std::string_view Name() const noexcept override { return "Update post processing"; }

    private:
        std::vector<Rendering::PostProcessing::PostEffect> m_OldData;
        std::vector<Rendering::PostProcessing::PostEffect> m_NewData;
    };

    // = = = = = = //
    //  Clipboard  //
    // = = = = = = //

    class PasteEntityCommand final : public EntityCommand {
    public:
        explicit PasteEntityCommand(nlohmann::json data, std::string parentUUID = {});
        ~PasteEntityCommand() override;

        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override { return "Paste entity"; }

        [[nodiscard]] ECS::EntityID GetCreated() const { return m_Created; }
        [[nodiscard]] const std::string &GetCreatedUUID() const { return m_EntityUUID; }

    private:
        nlohmann::json m_Data;
        std::string m_ParentUUID;
        ECS::EntityID m_Created = ECS::INVALID_ENTITY_ID;
        std::unique_ptr<EntitySubtreeSnapshot> m_Snapshot;
    };

    class PasteUIElementCommand final : public ICommand {
    public:
        PasteUIElementCommand(::UI::UIElement *parent, nlohmann::json data) : m_Parent(parent), m_Data(std::move(data)) {}

        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override { return "Paste UI element"; }

        [[nodiscard]] ::UI::UIElement *GetCreated() const { return m_Created; }

    private:
        ::UI::UIElement *m_Parent;
        nlohmann::json m_Data;
        ::UI::UIElement *m_Created = nullptr;
    };

} // namespace Editor::Commands
