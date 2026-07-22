#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Core {
    class ResourceManager;
}

namespace UI {
    class UISystem;
}

namespace IO::UISerializer {

    void Serialize(nlohmann::json &out, UI::UISystem &uiSystem, Core::ResourceManager &resources);

    bool Deserialize(const nlohmann::json &uiJson, UI::UISystem &uiSystem, Core::ResourceManager &resources);

} // namespace IO::UISerializer
