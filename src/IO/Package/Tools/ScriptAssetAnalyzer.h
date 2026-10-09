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
        std::set<std::string> prefabs;
        std::set<std::string> unresolvedFileCalls;
        // types requiring all of their assets
        std::set<std::string> dynamicAssets;

        std::set<std::string> unrecognizedCalls;

        bool includeAllCatalogAssets = false;
    };

    [[nodiscard]] ScriptAssetAnalysis AnalyzeScriptAssets(const std::vector<std::unique_ptr<ObSL::Stmt>> &statements, const nlohmann::json &catalogAssets);

} // namespace IO::Package::Tools
