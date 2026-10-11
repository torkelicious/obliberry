#include "TemporaryProject.h"
#include "ECS/Systems/ScriptSystem.h"
#include "IO/Package/Container.h"
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
