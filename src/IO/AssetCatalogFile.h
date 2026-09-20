#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif

namespace IO::CatalogFile {
    using json = nlohmann::json;
    namespace fs = std::filesystem;
    inline constexpr std::array<std::string_view, 6> Types = {"textures", "shaders", "meshes", "materials", "fonts", "animation_sets"};

    inline json Empty() {
        json result = {{"version", 1}, {"assets", json::object()}};
        for (auto type : Types)
            result["assets"][std::string(type)] = json::array();
        return result;
    }

    inline void Merge(json &destination, const json &incoming, const std::string &source) {
        if (!incoming.is_object())
            throw std::runtime_error(source + ": assets must be an object");
        for (const auto &[type, entries] : incoming.items()) {
            if (std::find(Types.begin(), Types.end(), type) == Types.end())
                throw std::runtime_error(source + ": unsupported asset type '" + type + "'");
            if (!entries.is_array())
                throw std::runtime_error(source + ": " + type + " must be an array");
            auto &output = destination.at(type);
            for (const auto &asset : entries) {
                if (!asset.is_object() || !asset.contains("id") || !asset.at("id").is_string() || asset.at("id").get_ref<const std::string &>().empty())
                    throw std::runtime_error(source + ": invalid " + type + " asset ID");
                const auto &id = asset.at("id").get_ref<const std::string &>();
                const auto found = std::find_if(output.begin(), output.end(), [&](const json &other) { return other.at("id") == id; });
                if (found == output.end())
                    output.push_back(asset);
                else if (*found != asset)
                    throw std::runtime_error(source + ": conflicting " + type + " asset '" + id + "'");
            }
            std::sort(output.begin(), output.end(), [](const json &a, const json &b) { return a.at("id").get_ref<const std::string &>() < b.at("id").get_ref<const std::string &>(); });
        }
    }

    inline json Normalize(const json &document, const std::string &source) {
        if (!document.is_object() || !document.contains("version") || !document.at("version").is_number_integer() || document.at("version") != 1 || !document.contains("assets"))
            throw std::runtime_error(source + ": invalid or unsupported asset catalog");
        json result = document; // Preserve any additional catalog metadata.
        result["assets"] = Empty().at("assets");
        Merge(result["assets"], document.at("assets"), source);
        return result;
    }

    inline json Read(const fs::path &path) {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Could not open " + path.string());
        json document;
        file >> document;
        if (file.bad())
            throw std::runtime_error("Could not read " + path.string());
        return document;
    }

    inline fs::path CreateUniqueDirectory(const fs::path &parent, const std::string &prefix) {
        static std::atomic<unsigned long long> counter{0};
        const auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto candidate = parent / (prefix + std::to_string(stamp) + "-" + std::to_string(counter.fetch_add(1)));
            std::error_code error;
            if (fs::create_directory(candidate, error))
                return candidate;
            if (error && error != std::errc::file_exists)
                throw fs::filesystem_error("Create directory", candidate, error);
        }
        throw std::runtime_error("Could not allocate temporary directory in " + parent.string());
    }

    inline void WriteAtomic(const fs::path &target, const json &document) {
        const auto parent = target.has_parent_path() ? target.parent_path() : fs::path(".");
        const auto tempDir = CreateUniqueDirectory(parent, ".asset-write-");
        struct Cleanup {
            fs::path directory;
            ~Cleanup() {
                std::error_code ignored;
                fs::remove_all(directory, ignored);
            }
        } cleanup{tempDir};
        const auto temp = tempDir / "document.json";
        {
            std::ofstream file;
            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.open(temp, std::ios::binary | std::ios::trunc);
            file << document.dump(4) << '\n';
            file.close();
        }
#ifdef _WIN32
        if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            const DWORD error = GetLastError();
            throw std::system_error(static_cast<int>(error), std::system_category(), "Replace " + target.string());
        }
#else
        fs::rename(temp, target);
#endif
    }
} // namespace IO::CatalogFile
