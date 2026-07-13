#include <string>
#include <fstream>
#include <sstream>
#include "Container.h"
#include "Core/LoggerService.h"

static int g_failures = 0;

#define CHECK(cond)                                                                                                                                                                                                        \
    do {                                                                                                                                                                                                                   \
        if (!(cond)) {                                                                                                                                                                                                     \
            LOG_ERROR("ContainerTest", std::string("FAILED: ") + #cond + " (line " + std::to_string(__LINE__) + ")");                                                                                                      \
            ++g_failures;                                                                                                                                                                                                  \
        }                                                                                                                                                                                                                  \
    } while (0)


#pragma push_macro("LOG_WHO")
#define LOG_WHO "ContainerTest"

int main() {
    // [1] Basic multi-entry roundtrip
    {
        IO::ContainerWriter writer;
        writer.add_script("scripts/a.obsl", "println \"hello from a\";");
        writer.add_script("scripts/b.obsl", "using \"scripts/a.obsl\";\nprintln \"hello from b\";");
        writer.write("test1.obpak");

        IO::ContainerReader reader;
        CHECK(reader.open("test1.obpak"));

        auto a = reader.read("scripts/a.obsl");
        CHECK(a && *a == "println \"hello from a\";");

        auto b = reader.read("scripts/b.obsl");
        CHECK(b && *b == "using \"scripts/a.obsl\";\nprintln \"hello from b\";");

        auto missing = reader.read("scripts/nonexistent.obsl");
        CHECK(!missing.has_value());

        LOG_INFO(LOG_WHO, "[1] Basic multi-entry round-trip passed");
    }

    // [2] Large compressible payload roundtrip
    {
        std::string big_source = "fn noop() { return 0; }\n";
        for (int i = 0; i < 500; ++i) {
            big_source += "fn noop() { return 0; }\n";
        }

        IO::ContainerWriter writer;
        writer.add_script("scripts/big.obsl", big_source);
        writer.write("test2.obpak");

        IO::ContainerReader reader;
        CHECK(reader.open("test2.obpak"));

        auto result = reader.read("scripts/big.obsl");
        CHECK(result && *result == big_source);

        LOG_INFO(LOG_WHO, "[2] Large compressible payload round-trip passed");
    }

    // [3] Tiny payload round-trip (raw fallback path)
    {
        std::string tiny_source = "x;";

        IO::ContainerWriter writer;
        writer.add_script("scripts/tiny.obsl", tiny_source);
        writer.write("test3.obpak");

        IO::ContainerReader reader;
        CHECK(reader.open("test3.obpak"));

        auto result = reader.read("scripts/tiny.obsl");
        CHECK(result && *result == tiny_source);

        LOG_INFO(LOG_WHO, "[3] Tiny payload round-trip (raw fallback path) passed");
    }

    // [4] Missing container file handled cleanly
    {
        IO::ContainerReader reader;
        CHECK(!reader.open("does_not_exist.obpak"));

        LOG_INFO(LOG_WHO, "[4] Missing container file handled cleanly passed");
    }

    // [5] Empty container handled cleanly
    {
        IO::ContainerWriter writer;
        writer.write("test5.obpak");

        IO::ContainerReader reader;
        CHECK(reader.open("test5.obpak"));

        auto result = reader.read("anything");
        CHECK(!result.has_value());

        LOG_INFO(LOG_WHO, "[5] Empty container handled cleanly passed");
    }

    // [6] External file payload round-trip
    {
        std::ifstream file("test.obsl", std::ios::in | std::ios::binary);
        if (!file) {
            LOG_WARN(LOG_WHO, "Could not open 'test.obsl'. Skipping Test 6");
        } else {
            std::ostringstream ss;
            ss << file.rdbuf();
            std::string file_source = ss.str();

            IO::ContainerWriter writer;
            writer.add_script("scripts/test.obsl", file_source);
            writer.write("test6.obpak");

            IO::ContainerReader reader;
            CHECK(reader.open("test6.obpak"));

            auto result = reader.read("scripts/test.obsl");
            CHECK(result && *result == file_source);

            LOG_INFO(LOG_WHO, "[6] External file payload round-trip passed");
        }
    }

    if (g_failures == 0) {
        LOG_INFO(LOG_WHO, "\nAll tests passed successfully");
        return 0;
    }

    LOG_ERROR(LOG_WHO, std::to_string(g_failures) + " test(s) failed");
    return 1;
}
#pragma pop_macro("LOG_WHO")
