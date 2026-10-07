#pragma once

#include "IO/AssetDependencies.h"
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "nlohmann/json_fwd.hpp"

namespace ObSL {
    struct Stmt;
}

namespace IO::Package::Tools {
    struct ScriptAssetAnalysis {
        AssetDependencies::RequiredAssets required;

        std::set<std::string> files;
        std::set<std::string> scenes;
        // types requiring all of their assets
        std::set<std::string> dynamicAssets;

        bool includeAllCatalogAssets = true;

        [[nodiscard]] ScriptAssetAnalysis AnalyzeScriptAssets(const std::vector<std::unique_ptr<ObSL::Stmt>> &statements, const nlohmann::json &catalogAssets);
    };

} // namespace IO::Package::Tools
