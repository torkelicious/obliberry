#include <ObSL/Interpreter.h>
#include <ObSL/Lexer.h>
#include <ObSL/Parser.h>
#include <gtest/gtest.h>
#include <deque>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

class ObSLTests : public testing::Test {
protected:
    std::deque<std::string> sources;
    std::vector<std::vector<std::unique_ptr<ObSL::Stmt>>> programs;
    std::ostringstream output;
    std::istringstream input;
    ObSL::Interpreter interpreter{".", output, input};

    void Run(const std::string &source) {
        sources.push_back(source);
        ObSL::Lexer lexer{sources.back()};
        ObSL::Parser parser{lexer.tokenize()};
        programs.push_back(parser.parse());
        interpreter.execute_block(programs.back(), interpreter.get_global_environment());
    }

    ObSL::Value Get(const std::string &name) { return interpreter.get_global_environment()->get(name); }

    void ExpectNumber(const std::string &name, const double expected) {
        const auto value = Get(name);
        ASSERT_TRUE(std::holds_alternative<double>(value)) << name;
        EXPECT_DOUBLE_EQ(std::get<double>(value), expected) << name;
    }
};

TEST_F(ObSLTests, LiteralsEscapesCommentsAndOperators) {
    ASSERT_NO_THROW(Run(R"obsl(
        // Comments must not consume the next line.
        var number = 12.5;
        var text = "a\n\t\"b\\c";
        var yes = true;
        var nothing = null;
        var result = ((5 & 3) | (2 << 2)) ^ 1;
    )obsl"));
    ExpectNumber("number", 12.5);
    ExpectNumber("result", 8);
    ASSERT_TRUE(std::holds_alternative<std::string>(Get("text")));
    EXPECT_EQ(std::get<std::string>(Get("text")), "a\n\t\"b\\c");
    ASSERT_TRUE(std::holds_alternative<bool>(Get("yes")));
    EXPECT_TRUE(std::get<bool>(Get("yes")));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(Get("nothing")));
}

TEST_F(ObSLTests, PrecedenceParenthesesAndAssociativity) {
    ASSERT_NO_THROW(Run("var a = 2 + 3 * 4; var b = (2 + 3) * 4; "
                        "var c = 20 - 5 - 3; var d = -2 * (3 + 1);"));
    ExpectNumber("a", 14);
    ExpectNumber("b", 20);
    ExpectNumber("c", 12);
    ExpectNumber("d", -8);
}

TEST_F(ObSLTests, MalformedSyntaxReportsErrors) {
    for (const std::string_view source : {"var x = ;", "var x = (1 + 2;", "fn broken( {", "if (true) {", "\"unfinished"}) {
        SCOPED_TRACE(source);
        EXPECT_THROW(Run(std::string{source}), ObSL::RuntimeError);
    }
    ASSERT_NO_THROW(Run("var recovered = 42;"));
    ExpectNumber("recovered", 42);
}

TEST_F(ObSLTests, ScopesFunctionsReturnsAndClosures) {
    ASSERT_NO_THROW(Run(R"obsl(
        var outer = 10;
        { var outer = 99; }
        fn add(a, b) { return a + b; }
        fn makeAdder(base) {
            fn inner(value) { return base + value; }
            return inner;
        }
        var sum = add(2, 3);
        var closure = makeAdder(7);
        var captured = closure(4);
    )obsl"));
    ExpectNumber("outer", 10);
    ExpectNumber("sum", 5);
    ExpectNumber("captured", 11);
    ASSERT_NO_THROW(Run("var second = closure(8);"));
    ExpectNumber("second", 15);
}

TEST_F(ObSLTests, ConditionalsLoopsAndShortCircuiting) {
    ASSERT_NO_THROW(Run(R"obsl(
        var sum = 0;
        for (var i = 1; i <= 4; i++) { sum += i; }
        var count = 0;
        while (count < 3) { count++; }
        var branch = 0;
        if (sum == 10) { branch = 1; } else { branch = 2; }
        var calls = 0;
        fn bump() { calls++; return true; }
        var a = false and bump();
        var b = true or bump();
        var c = true and bump();
    )obsl"));
    ExpectNumber("sum", 10);
    ExpectNumber("count", 3);
    ExpectNumber("branch", 1);
    ExpectNumber("calls", 1);
}

TEST_F(ObSLTests, ArraysSupportValidReadAndWriteIndices) {
    ASSERT_NO_THROW(Run("var array = [10, 20, 30]; array[1] = 25; "
                        "var first = array[0]; var middle = array[1]; var last = array[2];"));
    ExpectNumber("first", 10);
    ExpectNumber("middle", 25);
    ExpectNumber("last", 30);
}

TEST_F(ObSLTests, InvalidIndicesAndRuntimeTypesReportErrorsAndRecover) {
    ASSERT_NO_THROW(Run("var array = [10, 20];"));
    for (const char *source : {"var bad = array[2];", "array[-3] = 1;", "var bad = array[\"x\"];", "var bad = true - 1;", "var bad = unknownVariable;", "var bad = 3();"}) {
        SCOPED_TRACE(source);
        EXPECT_THROW(Run(source), ObSL::RuntimeError);
    }
    ASSERT_NO_THROW(Run("var recovered = array[0] + array[1];"));
    ExpectNumber("recovered", 30);
}
