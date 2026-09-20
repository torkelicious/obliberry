#include "AssetCatalog.h"

#include "AssetCatalogFile.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

namespace IO::AssetCatalog {

    /*
     * assets.json holds asset definitions for curr project
     * it is parsed once ( per project )
     * all asset references shall be lazy-loaded from it
     */


    using json = nlohmann::json;

    namespace {
        json s_Document;
        std::filesystem::path s_ProjectRoot;
        bool s_Loaded = false;
    } // namespace

    bool Load() {
        Close();

        try {
            const auto document = VFS::ReadVirtualJson("assets.json");

            if (!document) {
                LOG_ERROR("AssetCatalog", "Could not read assets.json");
                return false;
            }

            s_Document = CatalogFile::Normalize(*document, "assets.json");

            s_ProjectRoot = VFS::GetProjectRoot();
            s_Loaded = true;

            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("AssetCatalog", std::string("Load failed: ") + e.what());

            Close();
            return false;
        }
    }

    bool Save() {
        if (!IsLoaded() || VFS::IsPackaged()) {
            return false;
        }

        try {
            CatalogFile::WriteAtomic(VFS::Resolve("assets.json"), s_Document);

            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("AssetCatalog", std::string("Save failed: ") + e.what());

            return false;
        }
    }

    void Close() {
        s_Document = json();
        s_ProjectRoot.clear();
        s_Loaded = false;
    }

    bool IsLoaded() { return s_Loaded && VFS::IsProjectLoaded() && s_ProjectRoot == VFS::GetProjectRoot(); }

    const json *GetAssets() {
        if (!IsLoaded()) {
            return nullptr;
        }

        return &s_Document.at("assets");
    }

    const json *Find(const std::string_view type, const std::string_view id) {
        const json *assets = GetAssets();

        if (!assets) {
            return nullptr;
        }

        const auto typeIt = assets->find(std::string(type));

        if (typeIt == assets->end() || !typeIt->is_array()) {
            return nullptr;
        }

        for (const auto &asset : *typeIt) {
            if (asset.is_object() && asset.value("id", std::string{}) == id) {
                return &asset;
            }
        }

        return nullptr;
    }
} // namespace IO::AssetCatalog
