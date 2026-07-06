#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "Container.h"

static int g_failures = 0;

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "FAILED: " << #cond << " (line " << __LINE__ << ")\n";                                        \
            ++g_failures;                                                                                              \
        }                                                                                                              \
    } while (0)

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

        std::cout << "[1] Basic multi-entry round-trip passed.\n";
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

        std::cout << "[2] Large compressible payload round-trip passed.\n";
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

        std::cout << "[3] Tiny payload round-trip (raw fallback path) passed.\n";
    }

    // [4] Missing container file handled cleanly
    {
        IO::ContainerReader reader;
        CHECK(!reader.open("does_not_exist.obpak"));

        std::cout << "[4] Missing container file handled cleanly passed.\n";
    }

    // [5] Empty container handled cleanly
    {
        IO::ContainerWriter writer;
        writer.write("test5.obpak");

        IO::ContainerReader reader;
        CHECK(reader.open("test5.obpak"));

        auto result = reader.read("anything");
        CHECK(!result.has_value());

        std::cout << "[5] Empty container handled cleanly passed.\n";
    }

    // [6] External file payload round-trip
    {
        std::ifstream file("test.obsl", std::ios::in | std::ios::binary);
        if (!file) {
            std::cerr << "Warning: Could not open 'test.obsl'. Skipping Test 6.\n";
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

            std::cout << "[6] External file payload round-trip passed.\n";
        }
    }

    if (g_failures == 0) {
        std::cout << "\nAll tests passed successfully.\n";
        return 0;
    }

    std::cerr << "\n" << g_failures << " test(s) failed.\n";
    return 1;
}
