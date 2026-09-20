#pragma once
#include <nlohmann/json_fwd.hpp>

namespace IO::AssetCatalog {
    nlohmann::json Serialize();
    bool Load();
    bool Save();
} // namespace IO::AssetCatalog
