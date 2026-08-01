#pragma once
#include <filesystem>
#include <string>
#include "IO/Package/Container.h"
#include "DependencyGraph.h"

namespace IO::Package::Tools {
    class IgnoreRules;

    struct PackOptions {
        bool global_compress = true;
        bool verbose = false;
        bool quiet = false;
        std::string binary_name;
        const IgnoreRules *ignore = nullptr; // optional .pakignore rules
    };

    bool pack_one_file(const std::filesystem::path &filepath, const std::filesystem::path &project_dir, const std::filesystem::path &script_root, ContainerWriter &writer, DependencyGraph &dep_graph,
                       const PackOptions &opts);
} // namespace IO::Package::Tools
