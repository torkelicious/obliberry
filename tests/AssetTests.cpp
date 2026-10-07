#include "TemporaryProject.h"
#include "Core/ResourceManager.h"
#include "IO/AssetCatalogFile.h"
#include "IO/Loaders/SceneAssetLoader.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Texture/Texture.h"

class AssetTests : public TemporaryProject {
protected:
    Core::ResourceManager &resources = Core::ResourceManager::GetInstance();

    void LoadCatalog(const nlohmann::json &assets) {
        auto document = IO::CatalogFile::Empty();
        for (const auto &[type, entries] : assets.items()) {
            document["assets"][type] = entries;
        }
        ASSERT_NO_FATAL_FAILURE(WriteJson("assets.json", document));
        ASSERT_TRUE(IO::AssetCatalog::Load());
    }
};

TEST_F(AssetTests, CatalogSaveReloadPreservesIdsNamesAndPaths) {
    const nlohmann::json texture{{"id", "test-texture"}, {"name", "Test Texture"}, {"path", "assets/texture.png"}};
    ASSERT_NO_FATAL_FAILURE(LoadCatalog({{"textures", nlohmann::json::array({texture})}}));
    ASSERT_TRUE(IO::AssetCatalog::Save());
    IO::AssetCatalog::Close();
    ASSERT_TRUE(IO::AssetCatalog::Load());
    const auto *actual = IO::AssetCatalog::Find("textures", "test-texture");
    ASSERT_NE(actual, nullptr);
    EXPECT_EQ(*actual, texture);
}

TEST_F(AssetTests, MissingAssetOrDependencyFailsWithoutRetainingPartialAssets) {
    ASSERT_NO_FATAL_FAILURE(LoadCatalog({{"materials", nlohmann::json::array({{{"id", "test-material"}, {"texture", "missing-texture"}}})}}));
    for (const char *id : {"missing-material", "test-material"}) {
        SCOPED_TRACE(id);
        IO::SceneAssetLoader::SceneAssetScope scope;
        EXPECT_FALSE(IO::SceneAssetLoader::Acquire(IO::SceneAssetLoader::AssetKind::Material, id, scope));
        EXPECT_EQ(resources.Get<Rendering::Material>(id), nullptr);
    }
}

TEST_F(AssetTests, SharedDependencyIsReleasedOnlyAfterTheLastScope) {
    ASSERT_NO_FATAL_FAILURE(Write("assets/shared.ppm", std::string{"P6\n1 1\n255\n"} + std::string{"\xff\x00\x00", 3}));
    ASSERT_NO_FATAL_FAILURE(LoadCatalog({{"textures", nlohmann::json::array({{{"id", "test-shared-texture"}, {"path", "assets/shared.ppm"}}, {{"id", "test-unused-texture"}, {"path", "assets/unused.ppm"}}})},
            {"materials", nlohmann::json::array({{{"id", "test-material-a"}, {"shader", ""}, {"texture", "test-shared-texture"}}, {{"id", "test-material-b"}, {"shader", ""}, {"texture", "test-shared-texture"}}})}}));
    IO::SceneAssetLoader::SceneAssetScope a;
    IO::SceneAssetLoader::SceneAssetScope b;
    ASSERT_TRUE(IO::SceneAssetLoader::Acquire(IO::SceneAssetLoader::AssetKind::Material, "test-material-a", a));
    ASSERT_TRUE(IO::SceneAssetLoader::Acquire(IO::SceneAssetLoader::AssetKind::Material, "test-material-b", b));
    const auto texture = resources.Get<Rendering::Texture>("test-shared-texture");
    const auto second = resources.Get<Rendering::Material>("test-material-b");
    ASSERT_NE(texture, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->texture, texture);
    EXPECT_EQ(resources.Get<Rendering::Texture>("test-unused-texture"), nullptr);

    a = IO::SceneAssetLoader::SceneAssetScope{};
    IO::SceneAssetLoader::UnloadStale();
    EXPECT_EQ(resources.Get<Rendering::Material>("test-material-a"), nullptr);
    EXPECT_EQ(resources.Get<Rendering::Material>("test-material-b"), second);
    EXPECT_EQ(resources.Get<Rendering::Texture>("test-shared-texture"), texture);

    b = IO::SceneAssetLoader::SceneAssetScope{};
    IO::SceneAssetLoader::UnloadStale();
    EXPECT_EQ(resources.Get<Rendering::Material>("test-material-b"), nullptr);
    EXPECT_EQ(resources.Get<Rendering::Texture>("test-shared-texture"), nullptr);
}
