#include "TemporaryProject.h"
#include "IO/Package/Container.h"
#include "IO/Package/Tools/DependencyGraph.h"
#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <vector>

class VFSPackageTests : public TemporaryProject {};

TEST_F(VFSPackageTests, MountedFilesPreserveBinaryContentsAndRejectInvalidPaths) {
    const std::string contents{"abc\0def", 7};
    ASSERT_NO_FATAL_FAILURE(Write("assets/data.bin", contents));
    EXPECT_EQ(IO::VFS::ReadVirtual("assets/data.bin"), contents);
    EXPECT_FALSE(IO::VFS::ReadVirtual("assets/missing.bin"));
    EXPECT_FALSE(IO::VFS::ReadVirtual("../outside.bin"));
    EXPECT_FALSE(IO::VFS::ReadVirtual(directory / "assets/data.bin"));
    EXPECT_FALSE(IO::VFS::ReadVirtualView("assets/data.bin"));
}

TEST_F(VFSPackageTests, CompressedAndUncompressedPackagesPreserveContents) {
    const std::string binary{"a\0b\xff", 4};
    const std::string repeated(4096, 'x');
    IO::ContainerWriter writer;
    writer.add_raw_data("assets/raw.bin", {binary.begin(), binary.end()}, IO::Package::EntryType::RawBinary, false);
    writer.add_raw_data("assets/compressed.bin", {repeated.begin(), repeated.end()}, IO::Package::EntryType::RawBinary, true);
    writer.add_binary_json("project.json", nlohmann::json::to_msgpack({{"title", "Packaged"}}), true);
    writer.write(directory / "data.obpak");

    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "data.obpak"));
    EXPECT_EQ(reader.read("assets/raw.bin"), binary);
    EXPECT_EQ(reader.read_view("assets/raw.bin"), std::string_view{binary});
    EXPECT_EQ(reader.read("assets/compressed.bin"), repeated);
    EXPECT_FALSE(reader.read_view("assets/compressed.bin"));
    EXPECT_FALSE(reader.read("missing"));

    IO::VFS::MountPackage(directory / "data.obpak");
    ASSERT_TRUE(IO::VFS::IsPackaged());
    EXPECT_EQ(IO::VFS::ReadVirtual("assets/raw.bin"), binary);
    EXPECT_EQ(IO::VFS::ReadVirtual("assets/compressed.bin"), repeated);
    EXPECT_EQ(IO::VFS::ReadVirtualJson("project.json"), nlohmann::json({{"title", "Packaged"}}));
}

TEST_F(VFSPackageTests, ReturningFromPackageToProjectResetsMountState) {
    IO::ContainerWriter writer;
    writer.add_raw_data("value.txt", {'p','a','c','k'}, IO::Package::EntryType::RawBinary, false);
    writer.write(directory / "data.obpak");
    ASSERT_NO_FATAL_FAILURE(Write("value.txt", "disk"));

    IO::VFS::MountPackage(directory / "data.obpak");
    ASSERT_TRUE(IO::VFS::IsPackaged());
    EXPECT_EQ(IO::VFS::ReadVirtual("value.txt"), "pack");
    IO::VFS::MountProject(directory / "project.json");
    EXPECT_FALSE(IO::VFS::IsPackaged());
    EXPECT_EQ(IO::VFS::ReadVirtual("value.txt"), "disk");
    IO::VFS::UnmountProject();
    EXPECT_FALSE(IO::VFS::IsPackaged());
    EXPECT_FALSE(IO::VFS::IsProjectLoaded());
}

TEST_F(VFSPackageTests, InvalidHeadersAndTruncatedPackagesFailSafely) {
    IO::ContainerWriter writer;
    writer.add_raw_data("value.txt", {'a','b','c'}, IO::Package::EntryType::RawBinary, false);
    writer.write(directory / "valid.obpak");
    std::ifstream file(directory / "valid.obpak", std::ios::binary);
    const std::string original{std::istreambuf_iterator<char>{file}, {}};
    ASSERT_GT(original.size(), sizeof(IO::Package::FileHeader));

    for (const auto length : {std::size_t{0}, std::size_t{4}, sizeof(IO::Package::FileHeader) - 1}) {
        SCOPED_TRACE(length);
        ASSERT_NO_FATAL_FAILURE(Write("broken.obpak", original.substr(0, length)));
        IO::ContainerReader reader;
        EXPECT_FALSE(reader.open(directory / "broken.obpak"));
    }
    for (const bool corruptMagic : {false, true}) {
        auto bytes = original;
        if (corruptMagic) {
            bytes[0] = 'X';
        } else {
            IO::Package::FileHeader header;
            std::memcpy(&header, bytes.data(), sizeof(header));
            header.toc_offset = std::numeric_limits<std::uint64_t>::max();
            std::memcpy(bytes.data(), &header, sizeof(header));
        }
        ASSERT_NO_FATAL_FAILURE(Write("broken.obpak", bytes));
        IO::ContainerReader reader;
        EXPECT_FALSE(reader.open(directory / "broken.obpak"));
    }
    ASSERT_NO_FATAL_FAILURE(Write("broken.obpak", original.substr(0, original.size() - 1)));
    IO::ContainerReader reader;
    if (reader.open(directory / "broken.obpak")) {
        EXPECT_FALSE(reader.read("value.txt"));
    }
}

TEST_F(VFSPackageTests, ReopeningReaderDoesNotExposeOldPackageEntries) {
    IO::ContainerWriter first;
    first.add_raw_data("first", {'a'}, IO::Package::EntryType::RawBinary, false);
    first.write(directory / "first.obpak");
    IO::ContainerWriter second;
    second.add_raw_data("second", {'b'}, IO::Package::EntryType::RawBinary, false);
    second.write(directory / "second.obpak");
    IO::ContainerReader reader;
    ASSERT_TRUE(reader.open(directory / "first.obpak"));
    ASSERT_TRUE(reader.open(directory / "second.obpak"));
    EXPECT_FALSE(reader.read("first"));
    EXPECT_EQ(reader.read("second"), "b");
    EXPECT_FALSE(reader.open(directory / "missing.obpak"));
    EXPECT_FALSE(reader.read("second"));
}

TEST_F(VFSPackageTests, ScriptDependencyGraphRejectsMissingImports) {
    IO::Package::Tools::DependencyGraph complete;
    complete.add_script("main.obsl", {"helper.obsl"});
    complete.add_script("helper.obsl", {});
    EXPECT_TRUE(complete.validate("test"));
    IO::Package::Tools::DependencyGraph incomplete;
    incomplete.add_script("main.obsl", {"missing.obsl"});
    EXPECT_FALSE(incomplete.validate("test"));
}
