#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string_view>

namespace IO::AssetCatalog {

    bool Load();

    bool Save();

    void Close();

    bool Upsert(std::string_view type, const nlohmann::json &def);
    bool Remove(std::string_view type, std::string_view id);

    [[nodiscard]] bool IsLoaded();

    [[nodiscard]] const nlohmann::json *GetAssets();

    [[nodiscard]] const nlohmann::json *Find(std::string_view type, std::string_view id);
} // namespace IO::AssetCatalog
