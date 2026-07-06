#include "ObpakTools.h"

#include <filesystem>
#include <iostream>

#include "AssetPacking.h"
#include "DependencyGraph.h"
#include "IO/VFS.h"
#include "IO/Package/Container.h"

namespace IO::Package::Tools {
    void PackageCurrentProject(const std::string &output_dir) {
        std::filesystem::path project_dir = VFS::GetProjectRoot();

        std::filesystem::path out_file = "game.obpak";
        const std::string BINARY_NAME = "[obliberry]";


        if (project_dir.empty()) {
            std::cerr << "No project directory specified.\n";
            return;
        }
        if (!std::filesystem::exists(project_dir) || !std::filesystem::is_directory(project_dir)) {
            std::cerr << "Provided path is not a valid directory: " + project_dir.string() << "\n";
            return;
        }
        out_file = output_dir / out_file;

        std::cout << "Packing project: " + project_dir.string() << "\n";
        std::cout << "Output file: " + out_file.string() << "\n";

        std::filesystem::path script_root = project_dir / "assets" / "scripts";
        if (!std::filesystem::exists(script_root)) {
            std::cout << "Expected scripts folder not found: " + script_root.string() << "\n";
            return;
        }

        ContainerWriter writer;
        DependencyGraph dep_graph;
        PackOptions opts{true, true, false, BINARY_NAME};


        int success_count = 0, fail_count = 0;

        for (const auto &entry: std::filesystem::recursive_directory_iterator(project_dir)) {
            if (entry.is_directory()) continue;
            try {
                if (pack_one_file(entry.path(), project_dir, script_root, writer, dep_graph, opts))
                    ++success_count;
            } catch (const std::exception &e) {
                std::cerr << (entry.path().string() + " - " + e.what()) << "\n";
                ++fail_count;
            }
        }

        if (!dep_graph.validate(BINARY_NAME)) {
            std::cerr << "Dependency validation warnings reported (packing will continue).\n";
        }

        if (success_count > 0) {
            try {
                writer.write(out_file);
                std::cout << "Wrote " + out_file.string();
                std::cout << "Packed " + std::to_string(success_count) + "/" +
                        std::to_string(success_count + fail_count) + " files.\n";
            } catch (const std::exception &e) {
                std::cerr << std::string("Could not write package - ") + e.what() << "\n";
                return;
            }
        } else {
            std::cerr << "No files were successfully packed.\n";
            return;
        }
    }
}
