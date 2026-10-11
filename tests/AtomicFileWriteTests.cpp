#include "TemporaryProject.h"
#include "Core/Utils/OSFileUtils.h"
#include <atomic>
#include <barrier>
#include <thread>

class AtomicFileWriteTests : public TemporaryProject {};

TEST_F(AtomicFileWriteTests, ConcurrentWritersLeaveOneCompletePayloadAndNoTemporaryFiles) {
    const auto path = directory / "value.bin";
    const std::string first(4096, 'a');
    const std::string second(8192, 'b');
    std::atomic<int> failures{0};
    std::barrier start{2};
    const auto write = [&](const std::string &value) {
        start.arrive_and_wait();
        for (int i = 0; i < 20; ++i) {
            if (!Core::Utils::OSFile::WriteAtomic(path, value)) {
                ++failures;
            }
        }
    };
    std::jthread a([&] { write(first); });
    std::jthread b([&] { write(second); });
    a.join();
    b.join();
    EXPECT_EQ(failures, 0);
    const auto result = IO::VFS::ReadVirtual("value.bin");
    ASSERT_TRUE(result);
    EXPECT_TRUE(*result == first || *result == second);
    size_t files = 0;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        EXPECT_EQ(entry.path(), path);
        ++files;
    }
    EXPECT_EQ(files, 1u);
}

TEST_F(AtomicFileWriteTests, FailedReplacementPreservesDestinationAndRemovesTemporaryFile) {
    ASSERT_TRUE(std::filesystem::create_directory(directory / "blocked"));
    ASSERT_NO_FATAL_FAILURE(Write("blocked/marker.txt", "preserve this"));
    EXPECT_FALSE(Core::Utils::OSFile::WriteAtomic(directory / "blocked", "replacement"));
    EXPECT_TRUE(std::filesystem::is_directory(directory / "blocked"));
    EXPECT_EQ(IO::VFS::ReadVirtual("blocked/marker.txt"), "preserve this");
    size_t entries = 0;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        EXPECT_EQ(entry.path(), directory / "blocked");
        ++entries;
    }
    EXPECT_EQ(entries, 1u);
}

TEST_F(AtomicFileWriteTests, FailedOpenPreservesBlockingFile) {
    ASSERT_NO_FATAL_FAILURE(Write("blocked", "preserve this"));
    EXPECT_FALSE(Core::Utils::OSFile::WriteAtomic(directory / "blocked/value.json", "replacement"));
    EXPECT_EQ(IO::VFS::ReadVirtual("blocked"), "preserve this");
}
