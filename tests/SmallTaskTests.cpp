#include "Platform/Threading/SmallTask.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <string>

using Platform::Threading::SmallTask;

TEST(SmallTaskTests, SmallAlignedCapturesStayInline) {
    SmallTask task([value = int{42}] {});
    EXPECT_FALSE(task.UsesHeap());

    EXPECT_TRUE(static_cast<bool>(task));
}

TEST(SmallTaskTests, EightByteAlignedCaptureStaysInline) {
    SmallTask task([value = double{1.5}] {});
    EXPECT_FALSE(task.UsesHeap());
}

TEST(SmallTaskTests, OverAlignedCaptureFallsBackToHeap) {
    struct alignas(32) OverAligned {
        char data[8];
    };
    SmallTask task([value = OverAligned{}] {});
    EXPECT_TRUE(task.UsesHeap());
    task();
}

TEST(SmallTaskTests, OversizedCaptureFallsBackToHeap) {
    struct Big {
        char data[64];
        bool operator==(const Big &) const = default;
    };
    static_assert(std::is_trivially_move_constructible_v<Big>);
    static_assert(alignof(Big) <= 8);
    SmallTask task([value = Big{}] {});
    EXPECT_TRUE(task.UsesHeap());
    task();
}

TEST(SmallTaskTests, InlineTaskInvokesAndMoves) {
    int invoked = 0;
    {
        SmallTask task([&invoked] { invoked++; });
        task();
        EXPECT_EQ(invoked, 1);

        SmallTask moved(std::move(task));
        EXPECT_TRUE(static_cast<bool>(moved));
        EXPECT_FALSE(static_cast<bool>(task));
        moved();
        EXPECT_EQ(invoked, 2);

        SmallTask assigned;
        assigned = std::move(moved);
        EXPECT_TRUE(static_cast<bool>(assigned));
        assigned();
        EXPECT_EQ(invoked, 3);
    }
}

TEST(SmallTaskTests, HeapTaskInvokesAndMovesExactlyOnce) {
    int invoked = 0;
    {
        SmallTask task([&invoked, name = std::string("heap")] { invoked += static_cast<int>(name.size()); });
        EXPECT_TRUE(task.UsesHeap());
        task();
        EXPECT_EQ(invoked, 4);

        SmallTask moved(std::move(task));
        moved();
        EXPECT_EQ(invoked, 8);
    }
}

TEST(SmallTaskTests, EmptyTaskIsInert) {
    SmallTask task;
    EXPECT_FALSE(static_cast<bool>(task));
    task();
    SmallTask moved(std::move(task));
    EXPECT_FALSE(static_cast<bool>(moved));
}
