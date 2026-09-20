#include "IO/AssetCatalogFile.h"
#include <iostream>
#include <vector>

namespace {
    namespace fs = std::filesystem;
    namespace Catalog = IO::CatalogFile;
    using json = nlohmann::json;

    struct Scene {
        fs::path path;
        json document;
    };

    void Usage() {
        std::cout << "Usage: ob_asset_migrator <project-directory> [--dry-run] [--strip-scenes]\n"
                     "  Default: merge scene assets into assets.json; keep scene definitions.\n"
                     "  --strip-scenes: also remove scene asset sections after making backups.\n"
                     "  --dry-run: validate and report without writing anything.\n";
    }

    int Run(const std::vector<fs::path> &args) {
        fs::path project;
        bool dryRun = false;
        bool strip = false;
        for (const auto &arg : args) {
            if (arg == "--help" || arg == "-h") {
                Usage();
                return 0;
            }
            if (arg == "--dry-run")
                dryRun = true;
            else if (arg == "--strip-scenes")
                strip = true;
            else if (arg.native().starts_with(fs::path("-").native()))
                throw std::runtime_error("Unknown option: " + arg.string());
            else if (project.empty())
                project = arg;
            else
                throw std::runtime_error("Only one project directory is accepted");
        }
        if (project.empty()) {
            Usage();
            return 2;
        }
        project = fs::canonical(project);
        if (!fs::is_directory(project) || !fs::is_regular_file(project / "project.json"))
            throw std::runtime_error("Expected a project directory containing project.json");

        const auto catalogPath = project / "assets.json";
        const bool hadCatalog = fs::exists(catalogPath);
        const auto originalCatalog = hadCatalog ? Catalog::Read(catalogPath) : json();
        auto catalog = hadCatalog ? Catalog::Normalize(originalCatalog, catalogPath.string()) : Catalog::Empty();
        const auto scenesDir = project / "assets" / "scenes";
        std::vector<fs::path> paths;
        if (fs::exists(scenesDir)) {
            for (const auto &entry : fs::recursive_directory_iterator(scenesDir)) {
                if (entry.is_symlink()) {
                    if (entry.path().extension() == ".json" || entry.is_directory())
                        throw std::runtime_error("Resolve linked scene path before migrating: " + entry.path().string());
                    continue;
                }
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                    paths.push_back(entry.path());
            }
        }
        std::ranges::sort(paths);
        std::vector<Scene> scenes;
        for (const auto &path : paths) {
            auto scene = Catalog::Read(path);
            if (!scene.is_object())
                throw std::runtime_error(path.string() + ": scene must be an object");
            if (!scene.contains("assets"))
                continue;
            Catalog::Merge(catalog["assets"], scene.at("assets"), path.string());
            scenes.push_back({.path = path, .document = std::move(scene)});
        }

        std::size_t count = 0;
        for (const auto &array : catalog.at("assets"))
            count += array.size();
        std::cout << "Checked " << paths.size() << " scenes; " << scenes.size() << " contain asset sections. Catalog: " << count << " assets.\n";
        if (dryRun) {
            std::cout << "Dry run: no files written.\n";
            return 0;
        }

        const bool writeCatalog = !hadCatalog || originalCatalog != catalog;
        if (!writeCatalog && (!strip || scenes.empty())) {
            std::cout << "Already up to date.\n";
            return 0;
        }

        if ((writeCatalog && hadCatalog) || (strip && !scenes.empty())) {
            fs::path backup;
            backup = Catalog::CreateUniqueDirectory(project, "asset-migration-backup-");
            if (hadCatalog)
                fs::copy_file(catalogPath, backup / "assets.json");
            if (strip) {
                for (const auto &scene : scenes) {
                    const auto destination = backup / scene.path.lexically_relative(project);
                    fs::create_directories(destination.parent_path());
                    fs::copy_file(scene.path, destination);
                }
            }
            Catalog::WriteAtomic(backup / "migration-info.json", {{"project", project.generic_string()}, {"had_catalog", hadCatalog}, {"stripped_scenes", strip}, {"scene_count", scenes.size()}});
            std::cout << "Backups: " << backup.string() << std::endl;
        }

        if (writeCatalog)
            Catalog::WriteAtomic(catalogPath, catalog);
        if (strip) {
            for (auto &scene : scenes) {
                scene.document.erase("assets");
                Catalog::WriteAtomic(scene.path, scene.document);
            }
        }
        std::cout << "Migration complete. " << (strip ? "Scene asset sections removed.\n" : "Scene asset sections preserved.\n");
        return 0;
    }

    int Main(const std::vector<fs::path> &args) {
        try {
            return Run(args);
        } catch (const std::exception &e) {
            std::cerr << "Migration failed: " << e.what() << '\n';
            return 1;
        }
    }
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[]) {
    std::vector<std::filesystem::path> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);
    return Main(args);
}
#else
int main(int argc, char *argv[]) {
    std::vector<std::filesystem::path> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);
    return Main(args);
}
#endif
