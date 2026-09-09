#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace Core {
    class ResourceManager;
}

namespace UI {
    class UISystem;
    class UIElement;
} // namespace UI

namespace IO::UISerializer {

    void Serialize(nlohmann::json &out, UI::UISystem &uiSystem, Core::ResourceManager &resources);

    bool Deserialize(const nlohmann::json &uiJson, UI::UISystem &uiSystem, Core::ResourceManager &resources);

    void SerializeElement(nlohmann::json &out, const UI::UIElement *element, Core::ResourceManager &resources);

    std::unique_ptr<UI::UIElement> DeserializeElement(const nlohmann::json &j, Core::ResourceManager &resources);

    // returns the created subtree root
    UI::UIElement *DeserializeElementTree(const nlohmann::json &j, UI::UIElement *parent, UI::UISystem &uiSystem, Core::ResourceManager &resources);

} // namespace IO::UISerializer
