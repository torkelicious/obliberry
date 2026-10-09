#include "TemporaryProject.h"
#include "IO/AssetCatalogFile.h"
#include "IO/Package/Container.h"
#include "IO/Package/Tools/ObpakTools.h"

class ExportDependencyTests : public TemporaryProject {
protected:
    void Prepare(const std::string &script) {
        auto catalog = IO::CatalogFile::Empty();
        catalog["assets"]["textures"] = {{{"id", "used"}, {"path", "assets/textures/used.ppm"}}, {{"id", "unused"}, {"path", "assets/textures/unused.ppm"}}};
        catalog["assets"]["materials"] = {{{"id", "enemy-material"}, {"texture", "used"}, {"shader", ""}}};
        ASSERT_NO_FATAL_FAILURE(WriteJson("assets.json", catalog));
        ASSERT_NO_FATAL_FAILURE(WriteJson("project.json", {{"start_scene", "assets/scenes/main.json"}}));
        ASSERT_NO_FATAL_FAILURE(WriteJson("assets/scenes/main.json", {{"entities", nlohmann::json::array({{{"components", {{"ScriptComponent", {{"scriptPaths", {"assets/scripts/main.obsl"}}}}}}}})}}));
        ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/main.obsl", script));
        ASSERT_NO_FATAL_FAILURE(Write("assets/textures/used.ppm", std::string{"P6\n1 1\n255\n"} + std::string{"\xff\x00\x00", 3}));
        ASSERT_NO_FATAL_FAILURE(Write("assets/textures/unused.ppm", std::string{"P6\n1 1\n255\n"} + std::string{"\x00\xff\x00", 3}));
        ASSERT_NO_FATAL_FAILURE(Write("assets/audio/battle.ogg", "audio fixture"));
        ASSERT_NO_FATAL_FAILURE(WriteJson("custom/enemy.json", {{"components", {{"MaterialComponent", {{"material_id", "enemy-material"}}}}}}));
        ASSERT_NO_FATAL_FAILURE(WriteJson("custom/next.json", {{"entities", nlohmann::json::array()}}));
        ASSERT_TRUE(IO::AssetCatalog::Load());
    }

    bool Package() { return IO::Package::Tools::PackageCurrentProject((directory / "export").string()); }

    nlohmann::json ReadCatalog(IO::ContainerReader &reader) {
        const auto bytes = reader.read("assets.json");
        if (!bytes) {
            ADD_FAILURE() << "Missing packaged asset catalog";
            return nlohmann::json::object();
        }
        return nlohmann::json::from_msgpack(bytes->begin(), bytes->end());
    }
};

TEST_F(ExportDependencyTests, UiAndIndexingDoNotPackUnusedCatalogAssets) {
    ASSERT_NO_FATAL_FAILURE(Prepare(R"(
        var numbers = [10, 20, 30];
        print numbers[0];
        ui.SetColor(1, 1, 1, 1);
        ui.GetColor();
        ui.SetBackgroundColor(0, 0, 0, 1);
        sprite.SetTexture("used");
    )"));
    ASSERT_TRUE(Package());
    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "export/data.obpak"));
    EXPECT_TRUE(reader.read("assets/textures/used.ppm"));
    EXPECT_FALSE(reader.read("assets/textures/unused.ppm"));
    EXPECT_FALSE(reader.read("assets/audio/battle.ogg"));
    EXPECT_EQ(ReadCatalog(reader)["assets"]["textures"].size(), 1);
}

TEST_F(ExportDependencyTests, LiteralPrefabIncludesTransitiveAssetsWithoutUnusedFiles) {
    ASSERT_NO_FATAL_FAILURE(Prepare("Instantiate(\"custom/enemy.json\");"));
    ASSERT_TRUE(Package());
    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "export/data.obpak"));
    EXPECT_TRUE(reader.read("custom/enemy.json"));
    EXPECT_TRUE(reader.read("assets/textures/used.ppm"));
    EXPECT_FALSE(reader.read("assets/textures/unused.ppm"));
    const auto catalog = ReadCatalog(reader);
    EXPECT_EQ(catalog["assets"]["materials"].size(), 1);
    EXPECT_EQ(catalog["assets"]["textures"].size(), 1);
}

TEST_F(ExportDependencyTests, VariablePathsExportProjectFilesAndTheFullCatalog) {
    ASSERT_NO_FATAL_FAILURE(Prepare(R"(
        var music = "assets/audio/battle.ogg";
        PlayMusic(music, 1.0);
        PlaySound2D(music, 1.0);
        var scene = "custom/next.json";
        LoadScene(scene);
        var prefab = "custom/enemy.json";
        Instantiate(prefab);
    )"));
    ASSERT_TRUE(Package());
    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "export/data.obpak"));
    EXPECT_TRUE(reader.read("assets/audio/battle.ogg"));
    EXPECT_TRUE(reader.read("custom/next.json"));
    EXPECT_TRUE(reader.read("custom/enemy.json"));
    EXPECT_TRUE(reader.read("assets/textures/unused.ppm"));
    EXPECT_EQ(ReadCatalog(reader)["assets"]["textures"].size(), 2);
}

TEST_F(ExportDependencyTests, RuntimePathsAliasesAndImportedScriptsUseFallback) {
    ASSERT_NO_FATAL_FAILURE(Prepare(R"(
        var play = PlayMusic;
        play(paths[index], 1.0);
    )"));
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/helper.obsl", "using \"module.obsl\";"));
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/module.obsl", "fn value() { return 1; }"));
    ASSERT_TRUE(Package());
    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "export/data.obpak"));
    EXPECT_TRUE(reader.read("assets/scripts/helper.obsl"));
    EXPECT_TRUE(reader.read("assets/scripts/module.obsl"));
    EXPECT_TRUE(reader.read("assets/audio/battle.ogg"));
}

TEST_F(ExportDependencyTests, FallbackHonorsIgnoresAndExcludesPreviousExportOutput) {
    ASSERT_NO_FATAL_FAILURE(Prepare("PlayMusic(path, 1.0);"));
    ASSERT_NO_FATAL_FAILURE(Write(".pakignore", "private/\nignored.txt\n"));
    ASSERT_NO_FATAL_FAILURE(Write("private/invalid.json", "not JSON"));
    ASSERT_NO_FATAL_FAILURE(Write("ignored.txt", "ignored"));
    ASSERT_NO_FATAL_FAILURE(Write(".git/config", "metadata"));
    ASSERT_NO_FATAL_FAILURE(Write("export/old.json", "not JSON"));
    ASSERT_NO_FATAL_FAILURE(Write("old.obpak", "old package"));
    ASSERT_TRUE(Package());
    ASSERT_TRUE(Package());
    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "export/data.obpak"));
    for (const char *path : {"private/invalid.json", "ignored.txt", ".git/config", "export/old.json", "export/data.obpak", "old.obpak"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(reader.read(path));
    }
}

TEST_F(ExportDependencyTests, MissingLiteralFileStillFailsExport) {
    ASSERT_NO_FATAL_FAILURE(Prepare("PlayMusic(\"assets/audio/missing.ogg\", 1.0);"));
    EXPECT_FALSE(Package());
    EXPECT_FALSE(std::filesystem::exists(directory / "export/data.obpak"));
}

TEST_F(ExportDependencyTests, RequiredIgnoredFileStillFailsExport) {
    ASSERT_NO_FATAL_FAILURE(Prepare("PlayMusic(\"assets/audio/battle.ogg\", 1.0);"));
    ASSERT_NO_FATAL_FAILURE(Write(".pakignore", "assets/audio/\n"));
    EXPECT_FALSE(Package());
}

TEST_F(ExportDependencyTests, DynamicTextureDoesNotRetainUnrelatedAssetTypes) {
    ASSERT_NO_FATAL_FAILURE(Prepare("sprite.SetTexture(textures[index]);"));
    ASSERT_TRUE(Package());
    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "export/data.obpak"));
    const auto catalog = ReadCatalog(reader);
    EXPECT_EQ(catalog["assets"]["textures"].size(), 2);
    EXPECT_TRUE(catalog["assets"]["materials"].empty());
    EXPECT_FALSE(reader.read("assets/audio/battle.ogg"));
    EXPECT_FALSE(reader.read("custom/enemy.json"));
}
