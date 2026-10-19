#include "TemporaryProject.h"
#include "ECS/Systems/ScriptSystem.h"
#include "IO/Package/Container.h"
#include "Scripting/EngineLib/ScriptProperties.h"
#include <ObSL/ASTSerializer.h>
#include <deque>
#include <memory>
#include <string>
#include <vector>

class ScriptModuleTests : public TemporaryProject {
protected:
    ObSL::ScriptRuntime runtime;
    std::deque<std::string> sources;
    std::vector<std::vector<std::unique_ptr<ObSL::Stmt>>> programs;

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TemporaryProject::SetUp());
        RestartRuntime();
    }
    void TearDown() override {
        runtime.shutdown();
        TemporaryProject::TearDown();
    }
    void RestartRuntime() {
        runtime.shutdown();
        runtime.init("assets/scripts", 1);
        ECS::Systems::ScriptSystem::SetupScriptRuntime(runtime);
    }
    ObSL::Interpreter &Interpreter() { return runtime.get_worker(0)->GetInterpreter(); }
    void Run(const std::string &source) {
        sources.push_back(source);
        ObSL::Lexer lexer(sources.back());
        ObSL::Parser parser(lexer.tokenize());
        programs.push_back(parser.parse());
        Interpreter().execute_block(programs.back(), Interpreter().get_global_environment());
    }
};

TEST_F(ScriptModuleTests, LooseImportsUseMountedProjectRatherThanWorkingDirectory) {
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/helper.obsl", "fn Value() { return 42; }"));
    ASSERT_NO_THROW(Run("using \"helper.obsl\"; var answer = helper.Value();"));
    EXPECT_EQ(Interpreter().get_global_environment()->get("answer"), ObSL::Value{42.0});
}

TEST_F(ScriptModuleTests, CompressedAndMappedPackagedImportsExecuteTheirAst) {
    for (const bool compressed : {false, true}) {
        SCOPED_TRACE(compressed);
        RestartRuntime();
        const std::string source = "fn Value() { return \"imported packaged string\"; }";
        ObSL::Lexer lexer(source);
        ObSL::Parser parser(lexer.tokenize());
        const auto statements = parser.parse();
        ObSL::ASTSerializer serializer;
        IO::ContainerWriter writer;
        writer.add_compiled_script("assets/scripts/helper.obsl", serializer.finalize(statements), compressed);
        writer.write(directory / "data.obpak");
        IO::VFS::MountPackage(directory / "data.obpak");
        ASSERT_NO_THROW(Run("using \"helper.obsl\"; var answer = helper.Value();"));
        EXPECT_EQ(Interpreter().get_global_environment()->get("answer"), ObSL::Value{std::string{"imported packaged string"}});
        IO::VFS::UnmountProject();
    }
}

TEST_F(ScriptModuleTests, MissingImportFailsSafelyAndDoesNotPoisonSubsequentImports) {
    EXPECT_THROW(Run("using \"missing.obsl\";"), ObSL::RuntimeError);
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/missing.obsl", "fn Value() { return 7; }"));
    ASSERT_NO_THROW(Run("using \"missing.obsl\"; var recovered = missing.Value();"));
    EXPECT_EQ(Interpreter().get_global_environment()->get("recovered"), ObSL::Value{7.0});
}

TEST_F(ScriptModuleTests, ReinitializedRuntimeCanImportFromVfsAgain) {
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/helper.obsl", "fn Value() { return 42; }"));
    ASSERT_NO_THROW(Run("using \"helper.obsl\";"));
    RestartRuntime();
    ASSERT_NO_THROW(Run("using \"helper.obsl\"; var answer = helper.Value();"));
    EXPECT_EQ(Interpreter().get_global_environment()->get("answer"), ObSL::Value{42.0});
}

TEST_F(ScriptModuleTests, ScanScriptPropertiesFindsTypedDeclarations) {
    const std::string source = "var speed = Prop(\"Speed\", 2.5);\n"
                               "var enabled = Prop(\"Enabled\", true);\n"
                               "var title = Prop(\"Title\", \"Hello\");\n"
                               "var target = PropEntity(\"Target\");\n"
                               "var panel = PropUI(\"Panel\");\n"
                               "// Prop(\"Commented\", 1);\n"
                               "var ignored = NotProp(\"Nope\", 1);\n";

    const auto decls = Scripting::ScanScriptProperties(source);
    ASSERT_EQ(decls.size(), 5u);

    EXPECT_EQ(decls[0].varName, "speed");
    EXPECT_EQ(decls[0].name, "Speed");
    EXPECT_EQ(decls[0].type, Scripting::ScriptPropertyType::Number);
    EXPECT_EQ(decls[0].defaultValue, nlohmann::json(2.5));

    EXPECT_EQ(decls[1].name, "Enabled");
    EXPECT_EQ(decls[1].type, Scripting::ScriptPropertyType::Bool);
    EXPECT_EQ(decls[1].defaultValue, nlohmann::json(true));

    EXPECT_EQ(decls[2].name, "Title");
    EXPECT_EQ(decls[2].type, Scripting::ScriptPropertyType::String);
    EXPECT_EQ(decls[2].defaultValue, nlohmann::json("Hello"));

    EXPECT_EQ(decls[3].name, "Target");
    EXPECT_EQ(decls[3].type, Scripting::ScriptPropertyType::Entity);

    EXPECT_EQ(decls[4].name, "Panel");
    EXPECT_EQ(decls[4].type, Scripting::ScriptPropertyType::UIElement);
}

TEST_F(ScriptModuleTests, PropertyScopesNestAndRestorePreviousScope) {
    EXPECT_EQ(Scripting::CurrentInitProps(), nullptr);

    Scripting::PropertyMap outer;
    outer["Speed"] = 1.0;
    Scripting::PropertyMap inner;
    inner["Speed"] = 2.0;

    Scripting::ScriptPropertyScope outerScope(&outer);
    EXPECT_EQ(Scripting::CurrentInitProps(), &outer);
    {
        Scripting::ScriptPropertyScope innerScope(&inner);
        EXPECT_EQ(Scripting::CurrentInitProps(), &inner);
    }
    EXPECT_EQ(Scripting::CurrentInitProps(), &outer);
}
