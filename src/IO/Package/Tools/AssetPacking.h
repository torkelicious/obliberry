#pragma once
#include <filesystem>
#include <string>
#include "IO/Package/Container.h"
#include "DependencyGraph.h"

namespace IO::Package::Tools {
    struct PackOptions {
        bool global_compress = true;
        bool verbose = false;
        bool quiet = false;
        std::string binary_name;
    };

    bool pack_one_file(const std::filesystem::path &filepath, const std::filesystem::path &project_dir,
                       IO::ContainerWriter &writer, DependencyGraph &dep_graph, const PackOptions &opts);
}
