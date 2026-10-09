#include "TemporaryProject.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/MaterialComponent.h"
#include "IO/AssetCatalogFile.h"
#include "IO/Loaders/PrefabManager.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Texture/Texture.h"

class PrefabAssetScopeTests : public TemporaryProject {
protected:
    Core::ResourceManager &resources = Core::ResourceManager::GetInstance();
    Core::EngineContext context{};

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TemporaryProject::SetUp());
        IO::PrefabManager::ClearCache();
        context.resources = &resources;
        IO::EntityFactory::RegisterDeserializers();
        auto catalog = IO::CatalogFile::Empty();
        catalog["assets"]["textures"] = {{{"id", "spawn-texture"}, {"path", "assets/shared.ppm"}}};
        catalog["assets"]["materials"] = {{{"id", "spawn-material-a"}, {"texture", "spawn-texture"}, {"shader", ""}}, {{"id", "spawn-material-b"}, {"texture", "spawn-texture"}, {"shader", ""}}};
        ASSERT_NO_FATAL_FAILURE(WriteJson("assets.json", catalog));
        ASSERT_NO_FATAL_FAILURE(Write("assets/shared.ppm", std::string{"P6\n1 1\n255\n"} + std::string{"\xff\x00\x00", 3}));
        for (const char *suffix : {"a", "b"}) {
            ASSERT_NO_FATAL_FAILURE(WriteJson(std::string("assets/prefabs/") + suffix + ".json", {{"components", {{"MaterialComponent", {{"material_id", std::string("spawn-material-") + suffix}}}}}}));
        }
        ASSERT_TRUE(IO::AssetCatalog::Load());
    }

    void TearDown() override {
        IO::PrefabManager::ClearCache();
        TemporaryProject::TearDown();
    }
};

TEST_F(PrefabAssetScopeTests, RepeatedSpawnDestroyRetainsOnlyDistinctAssets) {
    {
        Scenes::Scene scene(&context, {});
        for (int i = 0; i < 10000; ++i) {
            const char *path = i % 2 == 0 ? "assets/prefabs/a.json" : "assets/prefabs/b.json";
            const auto id = IO::PrefabManager::Instantiate(scene, path);
            ASSERT_TRUE(scene.GetRegistry().IsValid(id));
            const auto *component = scene.GetRegistry().GetComponent<ECS::Components::MaterialComponent>(id);
            ASSERT_NE(component, nullptr);
            ASSERT_NE(component->material, nullptr);
            EXPECT_EQ(component->material->texture, resources.Get<Rendering::Texture>("spawn-texture"));
            scene.GetRegistry().DestroyEntity(id);
            ASSERT_EQ(scene.GetRetainedAssetCount(), i == 0 ? 2 : 3);
        }
        EXPECT_TRUE(scene.GetRegistry().GetLivingEntities().empty());
        EXPECT_NE(resources.Get<Rendering::Texture>("spawn-texture"), nullptr);
    }
    EXPECT_EQ(resources.Get<Rendering::Material>("spawn-material-a"), nullptr);
    EXPECT_EQ(resources.Get<Rendering::Material>("spawn-material-b"), nullptr);
    EXPECT_EQ(resources.Get<Rendering::Texture>("spawn-texture"), nullptr);
}

TEST_F(PrefabAssetScopeTests, SharedAssetsSurviveUntilTheLastSceneAndCachedPrefabsReload) {
    {
        Scenes::Scene first(&context, {});
        const auto a = IO::PrefabManager::Instantiate(first, "assets/prefabs/a.json");
        ASSERT_TRUE(first.GetRegistry().IsValid(a));
        const auto texture = resources.Get<Rendering::Texture>("spawn-texture");
        ASSERT_NE(texture, nullptr);
        {
            Scenes::Scene second(&context, {});
            const auto b = IO::PrefabManager::Instantiate(second, "assets/prefabs/b.json");
            ASSERT_TRUE(second.GetRegistry().IsValid(b));
            EXPECT_EQ(resources.Get<Rendering::Texture>("spawn-texture"), texture);
        }
        EXPECT_EQ(resources.Get<Rendering::Texture>("spawn-texture"), texture);
        EXPECT_EQ(resources.Get<Rendering::Material>("spawn-material-b"), nullptr);
    }
    EXPECT_EQ(resources.Get<Rendering::Texture>("spawn-texture"), nullptr);
    {
        Scenes::Scene next(&context, {});
        const auto id = IO::PrefabManager::Instantiate(next, "assets/prefabs/a.json");
        ASSERT_TRUE(next.GetRegistry().IsValid(id));
        EXPECT_NE(resources.Get<Rendering::Texture>("spawn-texture"), nullptr);
        EXPECT_EQ(next.GetRetainedAssetCount(), 2);
    }
    EXPECT_EQ(resources.Get<Rendering::Texture>("spawn-texture"), nullptr);
}

TEST_F(PrefabAssetScopeTests, EmptyAndMovedScopesMergeSafely) {
    using namespace IO::SceneAssetLoader;
    SceneAssetScope owner;
    owner.Merge(SceneAssetScope{});
    EXPECT_EQ(owner.GetAssetCount(), 0);
    SceneAssetScope incoming;
    ASSERT_TRUE(Acquire(AssetKind::Material, "spawn-material-a", incoming));
    owner.Merge(std::move(incoming));
    EXPECT_EQ(incoming.GetAssetCount(), 0);
    owner.Merge(std::move(incoming));
    EXPECT_EQ(owner.GetAssetCount(), 2);
    owner = SceneAssetScope{};
    EXPECT_EQ(resources.Get<Rendering::Texture>("spawn-texture"), nullptr);
}
