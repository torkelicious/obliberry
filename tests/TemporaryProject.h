#pragma once

#include "Core/Utils/UUID.h"
#include "IO/AssetCatalog.h"
#include "IO/Loaders/SceneAssetLoader.h"
#include "IO/VFS/VFS.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

class TemporaryProject : public testing::Test {
protected:
    std::filesystem::path directory;
    bool ownsDirectory = false;

    void SetUp() override {
        IO::AssetCatalog::Close();
        IO::VFS::UnmountProject();
        directory = std::filesystem::temp_directory_path() / ("obliberry-test-" + Core::Utils::UUID::UUIDGenerator::Generate());
        ASSERT_TRUE(std::filesystem::create_directory(directory));
        ownsDirectory = true;
        IO::VFS::MountProject(directory / "project.json");
    }

    void TearDown() override {
        IO::SceneAssetLoader::UnloadStale();
        IO::AssetCatalog::Close();
        IO::VFS::UnmountProject();
        if (ownsDirectory) {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    void Write(const std::string &path, const std::string &contents) {
        const auto destination = directory / path;
        std::filesystem::create_directories(destination.parent_path());
        std::ofstream file(destination, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        file.close();
        ASSERT_FALSE(file.fail());
    }

    void WriteJson(const std::string &path, const nlohmann::json &document) { Write(path, document.dump()); }
};
