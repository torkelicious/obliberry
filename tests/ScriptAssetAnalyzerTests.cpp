#include "IO/AssetCatalogFile.h"
#include "IO/Package/Tools/ScriptAssetAnalyzer.h"
#include <ObSL/Lexer.h>
#include <ObSL/Parser.h>
#include <ObSL/Parser/ast.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>

namespace {
    IO::Package::Tools::ScriptAssetAnalysis Analyze(const std::string &source) {
        auto catalog = IO::CatalogFile::Empty();
        catalog["assets"]["textures"] = {{{"id", "used"}}, {{"id", "unused"}}};
        catalog["assets"]["fonts"] = {{{"id", "font"}}};
        ObSL::Lexer lexer(source);
        ObSL::Parser parser(lexer.tokenize());
        const auto statements = parser.parse();
        return IO::Package::Tools::AnalyzeScriptAssets(statements, catalog["assets"]);
    }
} // namespace

TEST(ScriptAssetAnalyzerTests, LiteralAndConcatenatedPathsRemainPrecise) {
    const auto result = Analyze(R"(
        PlayMusic(("assets/audio/" + "battle.ogg"), 1.0);
        PlaySound2D("assets/audio/hit.wav", 1.0);
        LoadScene("assets/scenes/next.json");
        Instantiate("assets/prefabs/enemy.json");
        sprite.SetTexture("used");
    )");
    EXPECT_EQ(result.files, (std::set<std::string>{"assets/audio/battle.ogg", "assets/audio/hit.wav"}));
    EXPECT_EQ(result.scenes, (std::set<std::string>{"assets/scenes/next.json"}));
    EXPECT_EQ(result.prefabs, (std::set<std::string>{"assets/prefabs/enemy.json"}));
    EXPECT_EQ(result.required.textures, (std::set<std::string>{"used"}));
    EXPECT_TRUE(result.unresolvedFileCalls.empty());
    EXPECT_FALSE(result.includeAllCatalogAssets);
}

TEST(ScriptAssetAnalyzerTests, VariableFilePathsRequestConservativeFallback) {
    const auto result = Analyze(R"(
        var music = "assets/audio/battle.ogg";
        PlayMusic(music, 1.0);
        PlaySound2D(sounds[0], 1.0);
        LoadScene(nextScene);
        Instantiate(prefab);
    )");
    EXPECT_EQ(result.unresolvedFileCalls, (std::set<std::string>{"PlayMusic", "PlaySound2D", "LoadScene", "Instantiate"}));
}

TEST(ScriptAssetAnalyzerTests, OrdinaryArrayReadsWritesAndMethodsKeepFiltering) {
    const auto result = Analyze(R"(
        var numbers = [10, 20, 30];
        print numbers[0];
        numbers[1] = numbers[2];
        numbers.push(40);
        numbers.pop();
        numbers.clear();
    )");
    EXPECT_FALSE(result.includeAllCatalogAssets);
    EXPECT_TRUE(result.dynamicAssets.empty());
    EXPECT_TRUE(result.unrecognizedCalls.empty());
    EXPECT_TRUE(result.unresolvedFileCalls.empty());
}

TEST(ScriptAssetAnalyzerTests, IndexedAssetArgumentsRetainOnlyTheirAssetTypes) {
    const auto result = Analyze("sprites[0].SetTexture(textures[index]); labels[0].SetFont(fonts[index]);");
    EXPECT_FALSE(result.includeAllCatalogAssets);
    EXPECT_EQ(result.dynamicAssets, (std::set<std::string>{"textures", "fonts"}));
}

TEST(ScriptAssetAnalyzerTests, KnownUiAndComponentMethodsKeepFiltering) {
    const char *methods[] = {"SetColor", "GetColor", "SetTextColor", "GetTextColor", "SetBackgroundColor", "GetBackgroundColor", "GetFont", "GetSize", "GetText", "SetVisible", "IsVisible", "IsEnabled", "IsFocused",
            "IsHeld", "IsHovered", "WasClicked", "SetSize", "SetText", "AddComponent", "RemoveComponent", "AddCustomComponent", "GetCustomComponent", "TryMoveTo", "SetIsMoving", "SetTimePerStep", "GetHasSelection",
            "GetSelectedHex", "GetPathToHex", "SetIndex", "SetIntensity", "SetRadius", "SetEmitRate", "SetActive", "GetActive", "GetAliveCount", "GetIsTrigger", "SetIsTrigger", "GetLayer", "SetLayer", "GetMask",
            "SetMask", "GetFrame", "GetColumns", "GetRows", "GetTexture", "SetFrame", "SetGrid", "Play", "Restart", "Pause", "Resume", "Stop", "IsPlaying", "GetClip"};
    for (const auto *name : methods) {
        SCOPED_TRACE(name);
        const auto result = Analyze(std::string("object.") + name + "();");
        EXPECT_FALSE(result.includeAllCatalogAssets);
        EXPECT_TRUE(result.unrecognizedCalls.empty());
    }
}

TEST(ScriptAssetAnalyzerTests, AliasesAndUnknownCallsStayConservative) {
    const auto result = Analyze(R"(
        var play = PlayMusic;
        play("assets/audio/battle.ogg", 1.0);
        var setTexture = sprite.SetTexture;
        setTexture(key);
        callbacks[index]();
    )");
    EXPECT_TRUE(result.includeAllCatalogAssets);
    EXPECT_TRUE(result.unresolvedFileCalls.contains("PlayMusic"));
    EXPECT_TRUE(result.unrecognizedCalls.contains("<indirect call>"));
}

TEST(ScriptAssetAnalyzerTests, StandardMathStringAndTimingCallsKeepFiltering) {
    const auto result = Analyze(R"(
        var index = floor(random() * 3);
        var label = to_string(numbers[index]);
        if (contains(to_lower(label), "x")) { print len(label); }
        SetTimeout(1, callback);
    )");
    EXPECT_FALSE(result.includeAllCatalogAssets);
    EXPECT_TRUE(result.unrecognizedCalls.empty());
}
