/**
 * @file test_elevation_grid_map.cpp
 * @brief Unit tests for the ElevationGridMap class.
 *
 * Verifies grid dimensions, bounds checking, coordinate transformations,
 * flat ground cost calculation, obstacle thresholding, intermediate cost
 * normalization, and the high-speed buffer reset method.
 */

#include <gtest/gtest.h>
#include <limits>
#include "traversability_mapping/elevation_grid_map.hpp"

using namespace traversability_mapping;

class ElevationGridMapTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // 4m lateral (-2 to +2), 4m forward (0 to 4), 0.05m resolution
    map_.initialize(4.0f, 4.0f, 0.05f, 0.04f, 0.12f, -0.15f, 1.50f);
  }

  ElevationGridMap map_;
};

TEST_F(ElevationGridMapTest, InitializationDimensions)
{
  EXPECT_TRUE(map_.isInitialized());
  EXPECT_EQ(map_.getCols(), 80);  // 4.0 / 0.05
  EXPECT_EQ(map_.getRows(), 80);  // 4.0 / 0.05
  EXPECT_EQ(map_.getTotalCells(), 6400u);
  EXPECT_EQ(map_.getCostMap().size(), 6400u);
  EXPECT_EQ(map_.getMinZ().size(), 6400u);
  EXPECT_EQ(map_.getMaxZ().size(), 6400u);
  EXPECT_EQ(map_.getCellCounts().size(), 6400u);

  // All cells initially unobserved (-1)
  for (size_t i = 0; i < map_.getTotalCells(); ++i) {
    EXPECT_EQ(map_.getCostMap()[i], -1);
    EXPECT_EQ(map_.getCellCounts()[i], 0);
  }
}

TEST_F(ElevationGridMapTest, BoundsChecking)
{
  // Valid points inside forward (0 to 4m) and lateral (-2 to +2m) bounds
  EXPECT_TRUE(map_.addPoint(1.0f, 0.0f, 0.0f));
  EXPECT_TRUE(map_.addPoint(0.1f, -1.8f, 0.0f));
  EXPECT_TRUE(map_.addPoint(3.9f, 1.9f, 0.0f));

  // Behind the robot (negative X)
  EXPECT_FALSE(map_.addPoint(-0.1f, 0.0f, 0.0f));

  // Beyond forward length (X >= 4.0m)
  EXPECT_FALSE(map_.addPoint(4.1f, 0.0f, 0.0f));

  // Beyond lateral bounds (|Y| >= 2.0m)
  EXPECT_FALSE(map_.addPoint(1.0f, -2.1f, 0.0f));
  EXPECT_FALSE(map_.addPoint(1.0f, 2.1f, 0.0f));

  // Vertical cutoff filtering (Z < -0.15m or Z > 1.50m)
  EXPECT_FALSE(map_.addPoint(1.0f, 0.0f, -0.20f));
  EXPECT_FALSE(map_.addPoint(1.0f, 0.0f, 1.60f));
}

TEST_F(ElevationGridMapTest, FlatGroundZeroCost)
{
  // Add multiple points with delta Z = 0.02m <= safe_step (0.04m)
  map_.addPoint(1.0f, 0.0f, 0.00f);
  map_.addPoint(1.0f, 0.0f, 0.02f);
  map_.addPoint(1.0f, 0.0f, 0.01f);

  map_.computeTraversability();

  int row = 0, col = 0;
  ASSERT_TRUE(map_.toGrid(1.0f, 0.0f, row, col));
  EXPECT_EQ(map_.getCostAt(row, col), 0);
}

TEST_F(ElevationGridMapTest, ObstacleFullCost)
{
  // Add points with delta Z = 0.20m >= obstacle_threshold (0.12m)
  map_.addPoint(2.0f, 0.5f, 0.00f);
  map_.addPoint(2.0f, 0.5f, 0.20f);

  map_.computeTraversability();

  int row = 0, col = 0;
  ASSERT_TRUE(map_.toGrid(2.0f, 0.5f, row, col));
  EXPECT_EQ(map_.getCostAt(row, col), 100);
}

TEST_F(ElevationGridMapTest, IntermediateCostNormalization)
{
  // Delta Z = 0.08m (halfway between safe_step 0.04m and obstacle_step 0.12m)
  // Expected cost: ((0.08 - 0.04) / (0.12 - 0.04)) * 99 = 0.5 * 99 = ~50
  map_.addPoint(1.5f, -0.5f, 0.00f);
  map_.addPoint(1.5f, -0.5f, 0.08f);

  map_.computeTraversability();

  int row = 0, col = 0;
  ASSERT_TRUE(map_.toGrid(1.5f, -0.5f, row, col));
  const int8_t cost = map_.getCostAt(row, col);
  EXPECT_GE(cost, 45);
  EXPECT_LE(cost, 55);
}

TEST_F(ElevationGridMapTest, FastResetMethod)
{
  map_.addPoint(1.0f, 0.0f, 0.0f);
  map_.addPoint(1.0f, 0.0f, 0.25f);
  map_.computeTraversability();

  int row = 0, col = 0;
  ASSERT_TRUE(map_.toGrid(1.0f, 0.0f, row, col));
  EXPECT_EQ(map_.getCostAt(row, col), 100);

  // In-place buffer reset
  map_.reset();

  EXPECT_EQ(map_.getCostAt(row, col), -1);
  EXPECT_EQ(map_.getCountAt(row, col), 0);
  EXPECT_EQ(map_.getMinZAt(row, col), std::numeric_limits<float>::infinity());
  EXPECT_EQ(map_.getMaxZAt(row, col), -std::numeric_limits<float>::infinity());
}

TEST_F(ElevationGridMapTest, CoordinateConversions)
{
  int row = 0, col = 0;
  // Cell at forward x=2.0m, lateral y=0.0m
  EXPECT_TRUE(map_.toGrid(2.0f, 0.0f, row, col));
  EXPECT_EQ(row, 40);  // 2.0 / 0.05
  EXPECT_EQ(col, 40);  // (0.0 + 2.0) / 0.05

  float wx = 0.0f, wy = 0.0f;
  EXPECT_TRUE(map_.toWorld(row, col, wx, wy));
  EXPECT_NEAR(wx, 2.025f, 0.05f);  // Cell center
  EXPECT_NEAR(wy, 0.025f, 0.05f);
}
