#include "AssetCatalog.h"

#include "AssetCatalogFile.h"
#include "IO/Loaders/SceneAssetLoader.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

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

        bool Commmit(json document) {
            if (!IsLoaded() || VFS::IsPackaged()) {
                return false;
            }
            try {
                document = CatalogFile::Normalize(document, "asset catalog update");
                CatalogFile::WriteAtomic(VFS::Resolve("assets.json"), document);
                s_Document = std::move(document);
                return true;

            } catch (std::exception &e) {
                LOG_ERROR("AssetCatalog", std::string("Update Failed: ") + e.what());
            }
            return false;
        }

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

    bool Save() { return Commmit(s_Document); }

    void Close() {
        SceneAssetLoader::UnloadStale();
        s_Document = json();
        s_ProjectRoot.clear();
        s_Loaded = false;
    }

    bool Upsert(const std::string_view type, const nlohmann::json &def) {
        if (!IsLoaded() || VFS::IsPackaged() || !def.is_object()) {
            return false;
        }

        const auto idVal = def.find("id");
        if (idVal == def.end() || !idVal->is_string() || idVal->get_ref<const std::string &>().empty()) {
            return false;
        }

        json updated = s_Document;
        const auto entries = updated["assets"].find(std::string(type));

        if (entries == updated["assets"].end() || !entries->is_array()) {
            return false;
        }

        const std::string &id = idVal->get_ref<const std::string &>();
        const auto existing = std::find_if(entries->begin(), entries->end(), [&](const json &asset) { return asset.is_object() && asset.value("id", std::string{}) == id; });

        const bool replaceExisting = existing != entries->end();
        if (!replaceExisting) {
            entries->push_back(def);
        } else {
            *existing = def;
        }

        if (!Commmit(std::move(updated))) {
            return false;
        }

        if (replaceExisting) {
            SceneAssetLoader::MarkStale(type, id);
        }

        return true;
    }

    bool Remove(const std::string_view type, const std::string_view id) {
        if (!IsLoaded() || VFS::IsPackaged() || id.empty()) {
            return false;
        }

        json updated = s_Document;
        const auto entries = updated["assets"].find(std::string(type));

        if (entries == updated["assets"].end() || !entries->is_array()) {
            return false;
        }

        const auto firstRemoved = std::remove_if(entries->begin(), entries->end(), [&](const json &asset) { return asset.is_object() && asset.value("id", std::string{}) == id; });

        if (firstRemoved == entries->end()) {
            return false;
        }

        entries->erase(firstRemoved, entries->end());

        if (!Commmit(std::move(updated))) {
            return false;
        }

        SceneAssetLoader::MarkStale(type, id);
        return true;
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
