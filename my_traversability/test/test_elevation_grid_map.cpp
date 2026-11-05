#include <gtest/gtest.h>
#include "my_traversability/elevation_grid_map.hpp"

namespace my_traversability
{

class ElevationGridMapTest : public ::testing::Test
{
protected:
  // Map parameters: 4m x 4m, resolution = 0.05m (80 x 80 = 6,400 cells)
  // Safe step: <= 0.06m (6cm), Lethal obstacle: >= 0.12m (12cm)
  // Z bounds: -0.15m to 1.05m
  ElevationGridMap map_{4.0f, 4.0f, 0.05f, 0.06f, 0.12f, -0.15f, 1.05f};
};

// 1. Verify Discrete Grid Geometry & Initial Unobserved States
TEST_F(ElevationGridMapTest, GridDimensionsAndInitialState)
{
  EXPECT_EQ(map_.getRows(), 80);
  EXPECT_EQ(map_.getCols(), 80);
  EXPECT_EQ(map_.getTotalCells(), 6400U);
  EXPECT_TRUE(map_.isInitialized());

  map_.computeTraversability();
  const auto & costs = map_.getCostMap();
  ASSERT_EQ(costs.size(), 6400U);

  // Every unobserved cell must remain -1 (Nav2 unknown space)
  for (size_t i = 0; i < costs.size(); ++i) {
    EXPECT_EQ(costs[i], -1);
  }
}

// 2. Verify Coordinate Boundary and Height Threshold Rejections
TEST_F(ElevationGridMapTest, RejectsOutOfBoundsPoints)
{
  // Behind the robot chassis (negative X)
  EXPECT_FALSE(map_.addPoint(-0.1f, 0.0f, 0.0f));

  // Beyond forward grid length (> 4.0m)
  EXPECT_FALSE(map_.addPoint(4.05f, 0.0f, 0.0f));

  // Beyond lateral bounds (<-2.0m or >+2.0m along Y)
  EXPECT_FALSE(map_.addPoint(2.0f, -2.05f, 0.0f));
  EXPECT_FALSE(map_.addPoint(2.0f, 2.05f, 0.0f));

  // Below wheel contact plane (< -0.15m) or above clearance (> 1.05m)
  EXPECT_FALSE(map_.addPoint(2.0f, 0.0f, -0.20f));
  EXPECT_FALSE(map_.addPoint(2.0f, 0.0f, 1.10f));

  // Valid point well inside bounds
  EXPECT_TRUE(map_.addPoint(2.0f, 0.0f, 0.0f));
}

// 3. Verify Flat Floor Evaluation (Cost 0)
TEST_F(ElevationGridMapTest, ClassifiesFlatFloorAsFree)
{
  // Two points in the same cell with 2cm step (< 6cm safe threshold)
  EXPECT_TRUE(map_.addPoint(2.0f, 0.0f, 0.01f));
  EXPECT_TRUE(map_.addPoint(2.0f, 0.0f, 0.03f));

  map_.computeTraversability();

  int row = 0;
  int col = 0;
  ASSERT_TRUE(map_.toGridCoordinates(2.0f, 0.0f, row, col));
  const size_t idx = map_.toIndex(row, col);

  EXPECT_EQ(map_.getCostMap()[idx], 0);
}

// 4. Verify Lethal Obstacle Evaluation (Cost 100)
TEST_F(ElevationGridMapTest, ClassifiesTallStepAsLethalObstacle)
{
  // Two points with 15cm step (> 12cm obstacle threshold)
  EXPECT_TRUE(map_.addPoint(2.5f, 0.5f, 0.00f));
  EXPECT_TRUE(map_.addPoint(2.5f, 0.5f, 0.15f));

  map_.computeTraversability();

  int row = 0;
  int col = 0;
  ASSERT_TRUE(map_.toGridCoordinates(2.5f, 0.5f, row, col));
  const size_t idx = map_.toIndex(row, col);

  EXPECT_EQ(map_.getCostMap()[idx], 100);
}

// 5. Verify Rough Ground / Slope Linear Interpolation (Cost 1 to 99)
TEST_F(ElevationGridMapTest, InterpolatesRoughGroundCost)
{
  // Midpoint height step: 9cm (delta_z = 0.09m)
  // Normalized: (0.09 - 0.06) / (0.12 - 0.06) = 0.5
  // Expected cost: 1 + round(0.5 * 98) = 50
  EXPECT_TRUE(map_.addPoint(1.5f, -0.5f, 0.00f));
  EXPECT_TRUE(map_.addPoint(1.5f, -0.5f, 0.09f));

  map_.computeTraversability();

  int row = 0;
  int col = 0;
  ASSERT_TRUE(map_.toGridCoordinates(1.5f, -0.5f, row, col));
  const size_t idx = map_.toIndex(row, col);

  const int8_t cost = map_.getCostMap()[idx];
  EXPECT_GE(cost, 1);
  EXPECT_LE(cost, 99);
  EXPECT_NEAR(cost, 50, 2);
}

// 6. Verify Reset Restores Default State In-Place Without Reallocating
TEST_F(ElevationGridMapTest, ResetClearsGridBuffers)
{
  EXPECT_TRUE(map_.addPoint(1.0f, 0.0f, 0.20f));
  map_.computeTraversability();

  map_.reset();

  // Buffer capacities and sizes must remain identical
  EXPECT_EQ(map_.getTotalCells(), 6400U);
  EXPECT_EQ(map_.getCostMap().size(), 6400U);

  // All cells must be reset to unobserved (-1) and hit counts back to 0
  for (size_t i = 0; i < map_.getTotalCells(); ++i) {
    EXPECT_EQ(map_.getCostMap()[i], -1);
    EXPECT_EQ(map_.getCellCounts()[i], 0U);
  }
}

}  // namespace my_traversability