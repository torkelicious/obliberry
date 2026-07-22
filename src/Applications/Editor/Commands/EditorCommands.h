#pragma once
#include "ICommand.h"
#include "Config/ProjectConfig.h"
#include "Platform/Window/Window.h"
#include <glm/glm.hpp>
#include <optional>
#include <cstring>
#include <vector>
#include "ECS/Types.h"
#include "ECS/Components/ScriptComponent.h"
#include "Map/Hex.h"
#include "Map/HexCoords.h"
#include "Scenes/SceneManager.h"
#include "UI/UIElement.h"
#include "UI/Rendering/UISystem.h"
#include "UI/Elements/UIImage.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIButton.h"
#include "UI/Elements/UIRect.h"
#include "Rendering/Texture.h"
#include "UI/Text/Font.h"

namespace Editor::Commands {

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

    class TranslateEntityCommand final : public ICommand {
    public:
        // i could make this a unified transform command
        // but transforms are much larger than just the vec3's they hold sooo...
        // optimization ig :DDDD
        TranslateEntityCommand(ECS::EntityID target, glm::vec3 oldPos, glm::vec3 newPos);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldPos;
        glm::vec3 m_NewPos;
    };

    //
    // Rotate
    //

    class RotateEntityCommand final : public ICommand {
    public:
        RotateEntityCommand(ECS::EntityID target, glm::vec3 oldRot, glm::vec3 newRot);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldRot;
        glm::vec3 m_NewRot;
    };

    //
    // Scale
    //

    class ScaleEntityCommand final : public ICommand {
    public:
        ScaleEntityCommand(ECS::EntityID target, glm::vec3 oldScale, glm::vec3 newScale);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldScale;
        glm::vec3 m_NewScale;
    };

    // = = = = = //
    // Entity   //
    // = = = = = //

    // Rename Entity
    class SetNameCommand final : public ICommand {
    public:
        SetNameCommand(ECS::EntityID target, std::string oldName, std::string newName);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        std::string m_OldName;
        std::string m_NewName;
    };

    // = = = =  //
    // Registry //
    // = = = =  //
    /* ( or scene in general ) */

    // Remove Component
    template <typename T> class RemoveComponentCommand final : public ICommand {
    public:
        RemoveComponentCommand(const ECS::EntityID target, const T &componentData) : m_EntityID(target), m_OldData(componentData) {}
        void Execute(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().RemoveComponent<T>(m_EntityID); }
        void Undo(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().AddComponent<T>(m_EntityID, m_OldData); }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Remove Component"; }

    private:
        ECS::EntityID m_EntityID;
        T m_OldData;
    };

    // Add Component
    template <typename T> class AddComponentCommand final : public ICommand {
    public:
        AddComponentCommand(const ECS::EntityID target, const T &componentData) : m_EntityID(target), m_Data(componentData) {}
        void Execute(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().AddComponent<T>(m_EntityID, m_Data); }
        void Undo(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().RemoveComponent<T>(m_EntityID); }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Add Component"; }

    private:
        ECS::EntityID m_EntityID;
        T m_Data;
    };


    // SCRIPT COMPONENT HAS ITS OWN HANDLER:
    class RemoveScriptCommand : public ICommand {
    public:
        RemoveScriptCommand(ECS::EntityID target, ECS::Components::ScriptComponent &component, int index);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        ECS::Components::ScriptComponent *m_scriptComp = nullptr;
        int m_Index;
        bool m_ComponentRemoved = false;
        // per-entry data
        std::string m_SavedPath;
        std::vector<std::shared_ptr<ObSL::Environment>> m_SavedInstanceEnvs;
        bool m_IsInitialized = false;
        std::string m_SavedSourceCode;
        std::filesystem::file_time_type m_SavedLastModified;
    };


    class AddScriptCommand : public ICommand {
    public:
        AddScriptCommand(ECS::EntityID target, const std::string &script_path);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        ECS::Components::ScriptComponent *m_Comp = nullptr;
        std::string m_PendingPath;
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
    class MapChangeTileCommand : public ICommand {
    public:
        using TileState = std::pair<uint8_t, bool>; // {type, walkable}
        using StateMap = std::unordered_map<Map::HexCoords, std::optional<TileState>, Map::HexCoordsHash>;

        MapChangeTileCommand(StateMap oldState, StateMap newState, Map::HexGrid *grid, bool *meshDirty = nullptr);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        void ApplyStates(const StateMap &states) const;
        StateMap m_OldState;
        StateMap m_NewState;
        Map::HexGrid *m_Grid;
        bool *m_MeshDirty;
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
        // UIImage
        std::shared_ptr<::Rendering::Texture> image;
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
        RemoveUIElementCommand(::UI::UISystem *sys, ::UI::UIElement *parent, ::UI::UIElement *child);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override { return "Remove UI Element"; }

    private:
        ::UI::UISystem *m_UISystem;
        ::UI::UIElement *m_Parent;
        UIElementSnapshot m_Snapshot;
        ::UI::UIElement *m_Restored = nullptr;
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

    // = = = = = = = = = = = = = = //
    // Generic Component Field Edit //
    // = = = = = = = = = = = = = = //

    // A type-erased command that stores old/new bytes at a given offset inside a component.
    // Used by AutoComponentWidget to make generic field edits (DragFloat, DragInt, etc.) undoable.
    template <typename T> class ModifyComponentFieldCommand final : public ICommand {
    public:
        ModifyComponentFieldCommand(ECS::EntityID target, size_t offset, size_t fieldSize, const void *oldData, const void *newData, std::string fieldName)
            : m_EntityID(target), m_Offset(offset), m_FieldSize(fieldSize), m_FieldName(std::move(fieldName)) {
            const auto *src = static_cast<const uint8_t *>(oldData);
            m_OldData.assign(src, src + fieldSize);
            src = static_cast<const uint8_t *>(newData);
            m_NewData.assign(src, src + fieldSize);
        }

        void Execute(Core::EngineContext &ctx) override {
            auto *comp = ctx.sceneManager->GetCurrentScene()->GetRegistry().GetComponent<T>(m_EntityID);
            if (comp)
                std::memcpy(reinterpret_cast<uint8_t *>(comp) + m_Offset, m_NewData.data(), m_FieldSize);
        }

        void Undo(Core::EngineContext &ctx) override {
            auto *comp = ctx.sceneManager->GetCurrentScene()->GetRegistry().GetComponent<T>(m_EntityID);
            if (comp)
                std::memcpy(reinterpret_cast<uint8_t *>(comp) + m_Offset, m_OldData.data(), m_FieldSize);
        }

        [[nodiscard]] std::string_view Name() const noexcept override { return m_FieldName; }

    private:
        ECS::EntityID m_EntityID;
        size_t m_Offset;
        size_t m_FieldSize;
        std::string m_FieldName;
        std::vector<uint8_t> m_OldData;
        std::vector<uint8_t> m_NewData;
    };

} // namespace Editor::Commands
