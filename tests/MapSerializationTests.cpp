#include "TemporaryProject.h"
#include "IO/MapSerialization.h"

class MapSerializationTests : public TemporaryProject {};

TEST_F(MapSerializationTests, RoundTripPreservesNegativeCoordinatesAndTileValues) {
    Map::HexGrid original;
    original.EmplaceTile({-3, 5}, 7, false);
    original.EmplaceTile({2, -4}, 3, true);
    ASSERT_TRUE(IO::MapIO::Serialize("map.obmap", original));
    Map::HexGrid restored;
    ASSERT_TRUE(IO::MapIO::Deserialize("map.obmap", restored));
    ASSERT_EQ(restored.tiles.size(), 2u);
    EXPECT_EQ(restored.tiles.at({-3, 5}).type, 7);
    EXPECT_FALSE(restored.tiles.at({-3, 5}).walkable);
    EXPECT_EQ(restored.tiles.at({2, -4}).type, 3);
    EXPECT_TRUE(restored.tiles.at({2, -4}).walkable);
}

TEST_F(MapSerializationTests, MissingAndTruncatedFilesPreserveExistingGrid) {
    Map::HexGrid grid;
    grid.EmplaceTile({0, 0}, 2, true);
    ASSERT_NO_FATAL_FAILURE(Write("truncated.obmap", "OBLI"));
    for (const auto *path : {"missing.obmap", "truncated.obmap"}) {
        EXPECT_FALSE(IO::MapIO::Deserialize(path, grid));
        ASSERT_EQ(grid.tiles.size(), 1u);
        EXPECT_EQ(grid.tiles.at({0, 0}).type, 2);
    }
}

TEST_F(MapSerializationTests, BlockedDestinationReportsWriteFailure) {
    ASSERT_NO_FATAL_FAILURE(Write("blocked", "file"));
    Map::HexGrid grid;
    grid.EmplaceTile({0, 0}, 2, true);
    EXPECT_FALSE(IO::MapIO::Serialize("blocked/map.obmap", grid));
    EXPECT_EQ(IO::VFS::ReadVirtual("blocked"), "file");
}
