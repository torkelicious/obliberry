#include <algorithm>
#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include "Map/Hex.h"
#include "Math/HexMath.h"

TEST(HexMathTests, WorldCentersRoundtrip) {
    for (const float size : {0.25f, 0.5f, 2.0f}) {
        for (int q = -10; q <= 10; ++q) {
            for (int r = -10; r <= 10; ++r) {
                SCOPED_TRACE(testing::Message() << "size: " << size << ", q: " << q << ", r: " << r);

                const Map::HexCoords expected{q, r};
                const auto world = Math::HexMath::HexToWorld(expected, size);

                EXPECT_EQ(Math::HexMath::PixelToHex(world, size), expected);
            }
        }
    }
}

TEST(HexMathTests, CubeCoordinatesRoundtrip) {
    for (int q = -10; q <= 10; ++q) {
        for (int r = -10; r <= 10; ++r) {
            SCOPED_TRACE(testing::Message() << "q: " << q << ", r: " << r);

            const Map::HexCoords expected{q, r};
            const auto cube = Math::HexMath::OddRToCube(expected);

            EXPECT_EQ(Math::HexMath::CubeToOddR(cube), expected);
            EXPECT_EQ(cube.x + cube.y + cube.z, 0);
        }
    }
}

TEST(HexMathTests, NeighborsMatchEvenOddAndNegativeRows) {
    struct Case {
        Map::HexCoords origin;
        std::vector<Map::HexCoords> expected;
    };

    const std::vector<Case> cases{
            {{0, 0}, {{1, 0}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}}}, {{0, 1}, {{1, 1}, {1, 2}, {0, 2}, {-1, 1}, {0, 0}, {1, 0}}}, {{0, -1}, {{1, -1}, {1, 0}, {0, 0}, {-1, -1}, {0, -2}, {1, -2}}}};

    for (const auto &test : cases) {
        SCOPED_TRACE(test.origin.r);

        const auto neighbors = Math::HexMath::GetNeighbors(test.origin);

        std::vector<Map::HexCoords> actual(neighbors.begin(), neighbors.end());
        auto expected = test.expected;

        std::sort(actual.begin(), actual.end());
        std::sort(expected.begin(), expected.end());

        EXPECT_EQ(actual, expected);
    }
}

TEST(HexMathTests, DistanceMatchesKnownCases) {
    struct Case {
        Map::HexCoords first;
        Map::HexCoords second;
        int expected;
    };

    const std::vector<Case> cases{{{0, 0}, {0, 0}, 0}, {{0, 0}, {4, 0}, 4}, {{0, 0}, {0, 2}, 2}, {{-2, -2}, {2, 2}, 6}};

    for (std::size_t i = 0; i < cases.size(); ++i) {
        SCOPED_TRACE(i);
        const auto &test = cases[i];

        EXPECT_EQ(Math::HexMath::Distance(test.first, test.second), test.expected);
        EXPECT_EQ(Math::HexMath::Distance(test.second, test.first), test.expected);
    }
}

class PathfindingTests : public testing::Test {
protected:
    Map::HexGrid grid;
    std::vector<Map::HexCoords> path;

    void FillArea(int minQ, int maxQ, int minR, int maxR) {
        for (int q = minQ; q <= maxQ; ++q) {
            for (int r = minR; r <= maxR; ++r) {
                grid.EmplaceTile({q, r}, 0);
            }
        }
    }

    void SetWalkable(const Map::HexCoords &position, bool walkable) { grid.EmplaceTile(position, 0, walkable); }

    void FindPath(Map::HexCoords start, Map::HexCoords goal) { grid.FindPath(start, goal, path); }

    void ExpectValidPath(Map::HexCoords start, Map::HexCoords goal) {
        ASSERT_FALSE(path.empty());

        EXPECT_EQ(path.front(), start);
        EXPECT_EQ(path.back(), goal);

        for (std::size_t i = 0; i < path.size(); ++i) {
            SCOPED_TRACE(i);

            const auto *tile = grid.Get(path[i]);
            ASSERT_NE(tile, nullptr);
            EXPECT_TRUE(tile->walkable);

            if (i > 0) {
                EXPECT_EQ(Math::HexMath::Distance(path[i - 1], path[i]), 1);
            }
        }
    }
};

TEST_F(PathfindingTests, UnobstructedRouteHasShortestLength) {
    FillArea(0, 4, -2, 2);

    FindPath({0, 0}, {4, 0});

    ExpectValidPath({0, 0}, {4, 0});
    EXPECT_EQ(path.size(), 5u);
}

TEST_F(PathfindingTests, RouteAvoidsBlockedTile) {
    FillArea(0, 4, -2, 2);

    const Map::HexCoords blocked{2, 0};
    SetWalkable(blocked, false);

    FindPath({0, 0}, {4, 0});

    ExpectValidPath({0, 0}, {4, 0});
    EXPECT_EQ(path.size(), 6u);
    EXPECT_EQ(std::find(path.begin(), path.end(), blocked), path.end());
}

TEST_F(PathfindingTests, UnreachableDestinationReturnsEmptyPath) {
    FillArea(0, 4, 0, 0);
    SetWalkable({2, 0}, false);

    FindPath({0, 0}, {4, 0});

    EXPECT_TRUE(path.empty());
}

TEST_F(PathfindingTests, IdenticalEndpointsReturnOneTile) {
    FillArea(0, 2, 0, 0);

    const Map::HexCoords position{1, 0};
    FindPath(position, position);

    const std::vector<Map::HexCoords> expected{position};
    EXPECT_EQ(path, expected);
}

TEST_F(PathfindingTests, MissingEndpointsReturnEmptyPath) {
    FillArea(0, 2, 0, 0);

    FindPath({-1, 0}, {2, 0});
    EXPECT_TRUE(path.empty());

    FindPath({0, 0}, {3, 0});
    EXPECT_TRUE(path.empty());
}

TEST_F(PathfindingTests, BlockedEndpointsReturnEmptyPath) {
    FillArea(0, 2, 0, 0);

    SetWalkable({0, 0}, false);
    FindPath({0, 0}, {2, 0});
    EXPECT_TRUE(path.empty());

    SetWalkable({0, 0}, true);
    SetWalkable({2, 0}, false);

    FindPath({0, 0}, {2, 0});
    EXPECT_TRUE(path.empty());
}

TEST_F(PathfindingTests, RepeatedSearchesHaveIndependentResults) {
    FillArea(0, 3, 0, 0);

    FindPath({0, 0}, {3, 0});
    ExpectValidPath({0, 0}, {3, 0});
    EXPECT_EQ(path.size(), 4u);

    SetWalkable({2, 0}, false);

    FindPath({0, 0}, {3, 0});
    EXPECT_TRUE(path.empty());

    SetWalkable({2, 0}, true);

    FindPath({3, 0}, {0, 0});
    ExpectValidPath({3, 0}, {0, 0});
    EXPECT_EQ(path.size(), 4u);
}
