#include "ObpakTools.h"
#include <filesystem>
#include "Logger/LoggerService.h"
#include <iostream>
#include "AssetPacking.h"
#include "DependencyGraph.h"
#include <string>
#include <cctype>
#include "Core/Project.h"
#include "IO/VFS/VFS.h"
#include "IO/Package/Container.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ObpakTools"

namespace IO::Package::Tools {
    std::string SanitizeExecutableName(const std::string &input) {
        std::string out;
        bool lastWasUnderscore = false;
        for (const unsigned char c : input) {
            if (std::isalnum(c) || c == '-' || c == '_') {
                out += c;
                lastWasUnderscore = false;
            } else if (!lastWasUnderscore) {
                out += '_';
                lastWasUnderscore = true;
            }
        }
        if (out.empty())
            out = "game";
        return out;
    }


    void PackageCurrentProject(const std::string &output_dir) {
        std::filesystem::path project_dir = VFS::GetProjectRoot();

        std::filesystem::path out_file = "data.obpak";
        const std::string BINARY_NAME = "obliberry exporter";


        if (project_dir.empty()) {
            LOG_ERROR(LOG_WHO, "No project directory specified");
            return;
        }
        if (!std::filesystem::exists(project_dir) || !std::filesystem::is_directory(project_dir)) {
            LOG_ERROR(LOG_WHO, "Provided path is not a valid directory: " + project_dir.string());
            return;
        }
        out_file = std::filesystem::path(output_dir) / out_file;

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

        for (const auto &entry : std::filesystem::recursive_directory_iterator(project_dir)) {
            if (entry.is_directory())
                continue;
            try {
                if (pack_one_file(entry.path(), project_dir, script_root, writer, dep_graph, opts))
                    ++success_count;
            } catch (const std::exception &e) {
                LOG_ERROR(LOG_WHO, entry.path().string() + " - " + e.what());
                ++fail_count;
            }
        }

        if (!dep_graph.validate(BINARY_NAME)) {
            LOG_WARN(LOG_WHO, "Dependency validation warnings reported (packing will continue)");
        }

        if (success_count > 0) {
            try {
                writer.write(out_file);
                std::cout << "Wrote " + out_file.string() << "\n";
                std::cout << "Packed " + std::to_string(success_count) + "/" + std::to_string(success_count + fail_count) + " files.\n";
            } catch (const std::exception &e) {
                LOG_ERROR(LOG_WHO, std::string("Could not write package - ") + e.what());
            }
        } else {
            LOG_ERROR(LOG_WHO, "No valid assets found to pack");
        }
    }

    void ExportGame(const std::string &output_dir) {
        std::cout << "Exporting game to: " << output_dir << "\n";
        PackageCurrentProject(output_dir);

        const std::string clean_project_name = SanitizeExecutableName(Core::Project::GetActive()->GetConfig().Title);
#ifdef _WIN32
        const std::string runtime_name = "obliberry_runtime.exe";
        const std::string export_name = clean_project_name + ".exe";
#else
        const std::string runtime_name = "obliberry_runtime";
        const std::string &export_name = clean_project_name;
#endif
        // Locate the runtime
        const std::filesystem::path runtime_src = std::filesystem::current_path() / "internal" / runtime_name;
        const std::filesystem::path dest_exe = std::filesystem::path(output_dir) / export_name;

        try {
            if (std::filesystem::exists(runtime_src)) {
                std::filesystem::copy_file(runtime_src, dest_exe, std::filesystem::copy_options::overwrite_existing);
                std::cout << "[Export] Successfully copied runtime binary to " << dest_exe.string() << "\n";
            } else {
                LOG_ERROR(LOG_WHO, "Error: Could not find runtime binary at " + runtime_src.string());
                LOG_ERROR(LOG_WHO, "Ensure obliberry_runtime is built and located in the 'internal' folder next to the editor");
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Exception while copying runtime: " + std::string(e.what()));
        }
    }
} // namespace IO::Package::Tools
#pragma pop_macro("LOG_WHO")
