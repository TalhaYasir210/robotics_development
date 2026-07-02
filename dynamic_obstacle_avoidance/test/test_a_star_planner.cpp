#include <gtest/gtest.h>
#include "dynamic_obstacle_avoidance/a_star_planner.hpp"
#include <memory>

class AStarPlannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner = std::make_shared<AStarPlanner>();
    }
    std::shared_ptr<AStarPlanner> planner;
};

// ==============================================================================
// 1. worldToGrid() Tests
// ==============================================================================

// Category 1: Simple Cases (Origin is 0.0)
TEST_F(AStarPlannerTest, WorldToGrid_ExactMatch) {
    EXPECT_EQ(planner->worldToGrid(1.0, 0.0, 0.1), 10);
}

TEST_F(AStarPlannerTest, WorldToGrid_RoundDown) {
    EXPECT_EQ(planner->worldToGrid(1.04, 0.0, 0.1), 10);
}

TEST_F(AStarPlannerTest, WorldToGrid_RoundUp) {
    EXPECT_EQ(planner->worldToGrid(1.06, 0.0, 0.1), 11);
}

TEST_F(AStarPlannerTest, WorldToGrid_ZeroCoord) {
    EXPECT_EQ(planner->worldToGrid(0.0, 0.0, 0.1), 0);
}

// Category 2: Negative World Coordinates
TEST_F(AStarPlannerTest, WorldToGrid_NegativeExact) {
    EXPECT_EQ(planner->worldToGrid(-1.0, 0.0, 0.1), -10);
}

TEST_F(AStarPlannerTest, WorldToGrid_NegativeRoundDown) {
    EXPECT_EQ(planner->worldToGrid(-1.04, 0.0, 0.1), -10);
}

TEST_F(AStarPlannerTest, WorldToGrid_NegativeRoundUp) {
    EXPECT_EQ(planner->worldToGrid(-1.06, 0.0, 0.1), -11);
}

// Category 3: Real ROS Maps (Origin is Negative)
TEST_F(AStarPlannerTest, WorldToGrid_NegativeOrigin_PositiveWorld) {
    EXPECT_EQ(planner->worldToGrid(1.0, -2.0, 0.1), 30);
}

TEST_F(AStarPlannerTest, WorldToGrid_NegativeOrigin_NegativeWorld) {
    EXPECT_EQ(planner->worldToGrid(-1.0, -2.0, 0.1), 10);
}

// Category 4: Different Resolutions
TEST_F(AStarPlannerTest, WorldToGrid_HighResolution) {
    EXPECT_EQ(planner->worldToGrid(0.005, 0.0, 0.001), 5);
}

TEST_F(AStarPlannerTest, WorldToGrid_LowResolution) {
    EXPECT_EQ(planner->worldToGrid(15.0, 0.0, 5.0), 3);
}

// ==============================================================================
// 2. gridToWorld() Tests
// ==============================================================================

// Category 1: Simple Cases (Origin is 0.0)
TEST_F(AStarPlannerTest, GridToWorld_PositiveIndex) {
    EXPECT_NEAR(planner->gridToWorld(10, 0.0, 0.1), 1.0, 0.0001);
}

TEST_F(AStarPlannerTest, GridToWorld_ZeroIndex) {
    EXPECT_NEAR(planner->gridToWorld(0, 0.0, 0.1), 0.0, 0.0001);
}

// Category 2: Math with Negative Numbers
TEST_F(AStarPlannerTest, GridToWorld_NegativeIndex) {
    EXPECT_NEAR(planner->gridToWorld(-10, 0.0, 0.1), -1.0, 0.0001);
}

// Category 3: Real ROS Maps (Origin is Negative)
TEST_F(AStarPlannerTest, GridToWorld_NegativeOrigin_PositiveWorld) {
    EXPECT_NEAR(planner->gridToWorld(30, -2.0, 0.1), 1.0, 0.0001);
}

TEST_F(AStarPlannerTest, GridToWorld_NegativeOrigin_NegativeWorld) {
    EXPECT_NEAR(planner->gridToWorld(10, -2.0, 0.1), -1.0, 0.0001);
}

// Category 4: Different Resolutions
TEST_F(AStarPlannerTest, GridToWorld_HighResolution) {
    EXPECT_NEAR(planner->gridToWorld(5, 0.0, 0.001), 0.005, 0.0001);
}

TEST_F(AStarPlannerTest, GridToWorld_LowResolution) {
    EXPECT_NEAR(planner->gridToWorld(3, 0.0, 5.0), 15.0, 0.0001);
}

// ==============================================================================
// 3. isWithinBounds() Tests
// ==============================================================================

// Category 1: Inside the Map (Should be TRUE)
TEST_F(AStarPlannerTest, IsWithinBounds_Center) {
    EXPECT_TRUE(planner->isWithinBounds(5, 5, 10, 10));
}

TEST_F(AStarPlannerTest, IsWithinBounds_BottomLeftEdge) {
    EXPECT_TRUE(planner->isWithinBounds(0, 0, 10, 10));
}

TEST_F(AStarPlannerTest, IsWithinBounds_TopRightEdge) {
    EXPECT_TRUE(planner->isWithinBounds(9, 9, 10, 10));
}

// Category 2: Outside the Map - Negative Numbers (Should be FALSE)
TEST_F(AStarPlannerTest, IsWithinBounds_NegativeX) {
    EXPECT_FALSE(planner->isWithinBounds(-1, 5, 10, 10));
}

TEST_F(AStarPlannerTest, IsWithinBounds_NegativeY) {
    EXPECT_FALSE(planner->isWithinBounds(5, -1, 10, 10));
}

// Category 3: Outside the Map - Too Big (Should be FALSE)
TEST_F(AStarPlannerTest, IsWithinBounds_TooBigX) {
    EXPECT_FALSE(planner->isWithinBounds(10, 5, 10, 10));
}

TEST_F(AStarPlannerTest, IsWithinBounds_TooBigY) {
    EXPECT_FALSE(planner->isWithinBounds(5, 10, 10, 10));
}

TEST_F(AStarPlannerTest, IsWithinBounds_WayTooBig) {
    EXPECT_FALSE(planner->isWithinBounds(50, 50, 10, 10));
}

// ==============================================================================
// 4. isObstacle() Tests
// ==============================================================================

TEST_F(AStarPlannerTest, IsObstacle_FreeSpace) {
    std::vector<std::vector<int>> dummy_grid = {
        {0, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    EXPECT_FALSE(planner->isObstacle(dummy_grid, 0, 0));
}

TEST_F(AStarPlannerTest, IsObstacle_WallCenter) {
    std::vector<std::vector<int>> dummy_grid = {
        {0, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    EXPECT_TRUE(planner->isObstacle(dummy_grid, 1, 1));
}

TEST_F(AStarPlannerTest, IsObstacle_WallCorner) {
    std::vector<std::vector<int>> dummy_grid = {
        {0, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    EXPECT_TRUE(planner->isObstacle(dummy_grid, 2, 2));
}

// ==============================================================================
// 5. buildGridFromMap() Tests
// ==============================================================================

TEST_F(AStarPlannerTest, BuildGrid_AllFreeSpace) {
    std::vector<int8_t> flat_map = {0, 0, 0, 0};
    auto grid = planner->buildGridFromMap(flat_map, 2, 2);

    EXPECT_EQ(grid[0][0], 0);
    EXPECT_EQ(grid[0][1], 0);
    EXPECT_EQ(grid[1][0], 0);
    EXPECT_EQ(grid[1][1], 0);
}

TEST_F(AStarPlannerTest, BuildGrid_AllObstacles) {
    std::vector<int8_t> flat_map = {100, 100, 100, 100};
    auto grid = planner->buildGridFromMap(flat_map, 2, 2);

    EXPECT_EQ(grid[0][0], 1);
    EXPECT_EQ(grid[0][1], 1);
    EXPECT_EQ(grid[1][0], 1);
    EXPECT_EQ(grid[1][1], 1);
}

TEST_F(AStarPlannerTest, BuildGrid_MixedMap_WithThresholds) {
    std::vector<int8_t> flat_map = {0, 100, 50, -1};
    auto grid = planner->buildGridFromMap(flat_map, 2, 2);

    EXPECT_EQ(grid[0][0], 0); // 0 -> 0
    EXPECT_EQ(grid[0][1], 1); // 100 -> 1
    EXPECT_EQ(grid[1][0], 1); // 50 -> 1
    EXPECT_EQ(grid[1][1], 0); // -1 -> 0
}

// ==============================================================================
// 6. calculateMoveCost() Tests
// ==============================================================================

// Category 1: Straight Moves (Should cost 1.0)
TEST_F(AStarPlannerTest, MoveCost_StraightUp) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(0, 1), 1.0f);
}

TEST_F(AStarPlannerTest, MoveCost_StraightDown) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(0, -1), 1.0f);
}

TEST_F(AStarPlannerTest, MoveCost_StraightRight) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(1, 0), 1.0f);
}

TEST_F(AStarPlannerTest, MoveCost_StraightLeft) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(-1, 0), 1.0f);
}

// Category 2: Diagonal Moves (Should cost 1.414)
TEST_F(AStarPlannerTest, MoveCost_DiagonalTopRight) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(1, 1), 1.414f);
}

TEST_F(AStarPlannerTest, MoveCost_DiagonalBottomLeft) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(-1, -1), 1.414f);
}

TEST_F(AStarPlannerTest, MoveCost_DiagonalTopLeft) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(-1, 1), 1.414f);
}

TEST_F(AStarPlannerTest, MoveCost_DiagonalBottomRight) {
    EXPECT_FLOAT_EQ(planner->calculateMoveCost(1, -1), 1.414f);
}

// ==============================================================================
// 7. calculateHeuristic() Tests
// ==============================================================================

TEST_F(AStarPlannerTest, Heuristic_SamePoint) {
    EXPECT_FLOAT_EQ(planner->calculateHeuristic(0, 0, 0, 0), 0.0f);
}

TEST_F(AStarPlannerTest, Heuristic_HorizontalLine) {
    EXPECT_FLOAT_EQ(planner->calculateHeuristic(0, 0, 3, 0), 3.0f);
}

TEST_F(AStarPlannerTest, Heuristic_VerticalLine) {
    EXPECT_FLOAT_EQ(planner->calculateHeuristic(0, 0, 0, 4), 4.0f);
}

TEST_F(AStarPlannerTest, Heuristic_PerfectDiagonal) {
    // 3-4-5 Triangle
    EXPECT_FLOAT_EQ(planner->calculateHeuristic(0, 0, 3, 4), 5.0f);
}

TEST_F(AStarPlannerTest, Heuristic_NegativeCoordinates) {
    // 3-4-5 Triangle shifted to negative coordinates (-1 to 2 is 3, -1 to 3 is 4)
    EXPECT_FLOAT_EQ(planner->calculateHeuristic(-1, -1, 2, 3), 5.0f);
}

// ==============================================================================
// 8. isCornerCutting() Tests
// ==============================================================================

TEST_F(AStarPlannerTest, CornerCut_StraightMove) {
    std::vector<std::vector<int>> grid = {
        {0, 1, 0},
        {1, 0, 0},
        {0, 0, 0}
    };
    // Move straight down (dx=1, dy=0)
    EXPECT_FALSE(planner->isCornerCutting(grid, 1, 1, 1, 0));
}

TEST_F(AStarPlannerTest, CornerCut_DiagonalSafe) {
    std::vector<std::vector<int>> grid = {
        {0, 1, 0},
        {1, 0, 0},
        {0, 0, 0}
    };
    // Move diagonally down-right (dx=1, dy=1)
    EXPECT_FALSE(planner->isCornerCutting(grid, 1, 1, 1, 1));
}

TEST_F(AStarPlannerTest, CornerCut_DiagonalOneWall) {
    std::vector<std::vector<int>> grid = {
        {0, 1, 0},
        {1, 0, 0},
        {0, 0, 0}
    };
    // Move diagonally up-right (dx=-1, dy=1)
    EXPECT_TRUE(planner->isCornerCutting(grid, 1, 1, -1, 1));
}

TEST_F(AStarPlannerTest, CornerCut_DiagonalTwoWalls) {
    std::vector<std::vector<int>> grid = {
        {0, 1, 0},
        {1, 0, 0},
        {0, 0, 0}
    };
    // Move diagonally up-left (dx=-1, dy=-1)
    EXPECT_TRUE(planner->isCornerCutting(grid, 1, 1, -1, -1));
}

// ==============================================================================
// 9. getValidNeighbors() Tests
// ==============================================================================

TEST_F(AStarPlannerTest, GetValidNeighbors_Gauntlet) {
    std::vector<std::vector<int>> grid = {
        {0, 1, 0},
        {1, 0, 0},
        {0, 0, 0}
    };
    
    std::vector<std::vector<bool>> closed_list = {
        {false, false, false},
        {false, false, false},
        {false, true,  false} // (2,1) is in the closed list
    };

    GridNode start(1, 1);
    start.g_cost = 0.0f;
    GridNode goal(2, 2);
    
    std::vector<GridNode*> neighbors = planner->getValidNeighbors(&start, grid, closed_list, goal);

    // Out of 8 directions, exactly 2 should survive:
    // Right (1,2), Down-Right (2,2)
    EXPECT_EQ(neighbors.size(), 2);

    // Verify Right neighbor (1,2)
    bool found_right = false;
    for (auto* n : neighbors) {
        if (n->x == 1 && n->y == 2) {
            found_right = true;
            EXPECT_FLOAT_EQ(n->g_cost, 1.0f); // Straight move from start (1,1)
            EXPECT_FLOAT_EQ(n->h_cost, 1.0f); // Distance from (1,2) to (2,2) is 1.0
        }
        delete n; // Clean up memory
    }
    EXPECT_TRUE(found_right);
}

// ==============================================================================
// 10. computePath() Tests
// ==============================================================================

TEST_F(AStarPlannerTest, ComputePath_PathFound) {
    std::vector<std::vector<int>> grid = {
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0}
    };
    
    GridNode start(0, 0);
    GridNode goal(3, 3);
    
    std::vector<GridNode*> path = planner->computePath(grid, start, goal);
    
    EXPECT_GT(path.size(), 0);
    if (!path.empty()) {
        // Path is built backwards: from Goal to Start
        EXPECT_EQ(path.front()->x, 3);
        EXPECT_EQ(path.front()->y, 3);
        EXPECT_EQ(path.back()->x, 0);
        EXPECT_EQ(path.back()->y, 0);
    }
    
    for (auto* n : path) {
        delete n;
    }
}

TEST_F(AStarPlannerTest, ComputePath_NoPathPossible) {
    std::vector<std::vector<int>> grid = {
        {0, 0, 0, 0},
        {0, 1, 1, 1},
        {0, 1, 0, 1}, // Goal trapped at (2,2)
        {0, 1, 1, 1}
    };
    
    GridNode start(0, 0);
    GridNode goal(2, 2);
    
    std::vector<GridNode*> path = planner->computePath(grid, start, goal);
    
    EXPECT_EQ(path.size(), 0);
    
    for (auto* n : path) {
        delete n;
    }
}

// ==============================================================================
// 11. convertGridPathToWorldPath() Tests
// ==============================================================================

TEST_F(AStarPlannerTest, ConvertGridPath_ReversalAndScaling) {
    // Create a fake backwards path (like the engine outputs)
    GridNode goal(2, 2);
    GridNode middle(1, 1);
    GridNode start(0, 0);
    
    std::vector<GridNode*> grid_path = {&goal, &middle, &start};
    
    // Convert to world path (origin 0.0, resolution 0.1)
    std::vector<Point2D> world_path = planner->convertGridPathToWorldPath(grid_path, 0.0, 0.0, 0.1);
    
    // Check that it reversed the list AND scaled it properly
    EXPECT_EQ(world_path.size(), 3);
    
    if (world_path.size() == 3) {
        // First point should be Start (0,0) -> (0.0, 0.0)
        EXPECT_FLOAT_EQ(world_path[0].x, 0.0);
        EXPECT_FLOAT_EQ(world_path[0].y, 0.0);
        
        // Middle point (1,1) -> (0.1, 0.1)
        EXPECT_FLOAT_EQ(world_path[1].x, 0.1);
        EXPECT_FLOAT_EQ(world_path[1].y, 0.1);
        
        // Last point should be Goal (2,2) -> (0.2, 0.2)
        EXPECT_FLOAT_EQ(world_path[2].x, 0.2);
        EXPECT_FLOAT_EQ(world_path[2].y, 0.2);
    }
}

// ==============================================================================
// 12. findPath() Tests (The Ultimate Boss)
// ==============================================================================

TEST_F(AStarPlannerTest, FindPath_Success) {
    // 3x3 map, wall in the center
    std::vector<int8_t> flat_map = {
        0, 0, 0,
        0, 100, 0,
        0, 0, 0
    };
    
    // Find path from (0.0, 0.0) to (0.2, 0.2)
    std::vector<Point2D> path = planner->findPath(flat_map, 3, 3, 0.1, 0.0, 0.0, 0.0, 0.0, 0.2, 0.2);
    
    EXPECT_GT(path.size(), 0);
}

TEST_F(AStarPlannerTest, FindPath_Fail_OutOfBounds) {
    std::vector<int8_t> flat_map = {
        0, 0, 0,
        0, 100, 0,
        0, 0, 0
    };
    
    // Find path from (0.0, 0.0) to (5.0, 5.0) which is off the 0.3x0.3m map
    std::vector<Point2D> path = planner->findPath(flat_map, 3, 3, 0.1, 0.0, 0.0, 0.0, 0.0, 5.0, 5.0);
    
    EXPECT_EQ(path.size(), 0);
}

TEST_F(AStarPlannerTest, FindPath_Fail_StartInWall) {
    std::vector<int8_t> flat_map = {
        0, 0, 0,
        0, 100, 0,
        0, 0, 0
    };
    
    // Find path from (0.1, 0.1) [which is the wall] to (0.2, 0.2)
    std::vector<Point2D> path = planner->findPath(flat_map, 3, 3, 0.1, 0.0, 0.0, 0.1, 0.1, 0.2, 0.2);
    
    EXPECT_EQ(path.size(), 0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
