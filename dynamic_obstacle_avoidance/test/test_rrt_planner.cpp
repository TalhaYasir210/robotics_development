#include <gtest/gtest.h>
#include "dynamic_obstacle_avoidance/rrt_planner.hpp"
#include <memory>
#include <cmath>

class RRTPlannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner = std::make_shared<RRTPlanner>();
    }
    std::shared_ptr<RRTPlanner> planner;
};

// ==============================================================================
// 1. Coordinate & Distance Utilities Tests
// ==============================================================================

TEST_F(RRTPlannerTest, WorldToGrid_PositiveExact) {
    EXPECT_EQ(planner->worldToGrid(1.0, 0.0, 0.1), 10);
}

TEST_F(RRTPlannerTest, GridToWorld_PositiveIndex) {
    EXPECT_NEAR(planner->gridToWorld(10, 0.0, 0.1), 1.0, 0.0001);
}

TEST_F(RRTPlannerTest, CalculateDistance_SamePoint) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(0.0, 0.0, 0.0, 0.0), 0.0);
}

TEST_F(RRTPlannerTest, CalculateDistance_StraightLine) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(0.0, 0.0, 3.0, 0.0), 3.0);
}

TEST_F(RRTPlannerTest, CalculateDistance_Diagonal) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(0.0, 0.0, 3.0, 4.0), 5.0);
}

// ==============================================================================
// 2. Collision Checking Phase Tests
// ==============================================================================

TEST_F(RRTPlannerTest, IsWithinBounds_Center) {
    EXPECT_TRUE(planner->isWithinBounds(5, 5, 10, 10));
}

TEST_F(RRTPlannerTest, IsWithinBounds_OutOfBounds) {
    EXPECT_FALSE(planner->isWithinBounds(-1, 5, 10, 10));
    EXPECT_FALSE(planner->isWithinBounds(10, 5, 10, 10));
}

TEST_F(RRTPlannerTest, IsObstacle_FreeSpace) {
    std::vector<std::vector<int>> grid = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };
    EXPECT_FALSE(planner->isObstacle(grid, 1, 1));
}

TEST_F(RRTPlannerTest, IsObstacle_Hit) {
    std::vector<std::vector<int>> grid = {
        {0, 0, 0},
        {0, 1, 0},
        {0, 0, 0}
    };
    EXPECT_TRUE(planner->isObstacle(grid, 1, 1));
}

TEST_F(RRTPlannerTest, IsPathCollisionFree_ClearPath) {
    std::vector<std::vector<int>> grid = {
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };
    // From (0.0, 0.0) to (0.3, 0.2) in an empty 4x3 grid with resolution 0.1
    EXPECT_TRUE(planner->isPathCollisionFree(0.0, 0.0, 0.3, 0.2, grid, 0.0, 0.0, 0.1));
}

TEST_F(RRTPlannerTest, IsPathCollisionFree_BlockedPath) {
    std::vector<std::vector<int>> grid = {
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0}
    };
    // Path goes right through the obstacle wall in the middle
    EXPECT_FALSE(planner->isPathCollisionFree(0.0, 0.1, 0.3, 0.1, grid, 0.0, 0.0, 0.1));
}

// ==============================================================================
// 3. Core RRT Mechanics Tests
// ==============================================================================

TEST_F(RRTPlannerTest, GetRandomPoint_WithinLimits) {
    // We cannot easily test randomness without mocking, but we can test bounds
    auto pt = planner->getRandomPoint(5.0, 5.0, 0.0, 10.0, 0.0, 10.0);
    EXPECT_GE(pt.x, 0.0);
    EXPECT_LE(pt.x, 10.0);
    EXPECT_GE(pt.y, 0.0);
    EXPECT_LE(pt.y, 10.0);
}

TEST_F(RRTPlannerTest, GetNearestNode_FindsClosest) {
    TreeNode* n1 = new TreeNode(0.0, 0.0, nullptr);
    TreeNode* n2 = new TreeNode(2.0, 2.0, nullptr);
    TreeNode* n3 = new TreeNode(10.0, 10.0, nullptr);
    
    std::vector<TreeNode*> tree = {n1, n2, n3};
    
    TreeNode* nearest = planner->getNearestNode(tree, 1.0, 1.0);
    // (1,1) is closer to (0,0) (dist 1.414) than (2,2) (dist 1.414). Wait, it's equidistant!
    // Let's use a clear winner: (1.5, 1.5) is closer to (2,2)
    TreeNode* clear_nearest = planner->getNearestNode(tree, 1.6, 1.6);
    EXPECT_EQ(clear_nearest, n2);
    
    delete n1; delete n2; delete n3;
}

TEST_F(RRTPlannerTest, Steer_TowardsTarget) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    double target_x = 10.0;
    double target_y = 0.0;
    double step_size = 2.0;
    
    auto new_node = planner->steer(start, target_x, target_y, step_size);
    EXPECT_FLOAT_EQ(new_node.x, 2.0);
    EXPECT_FLOAT_EQ(new_node.y, 0.0);
    
    delete start;
}

TEST_F(RRTPlannerTest, IsGoalReached_WithinTolerance) {
    EXPECT_TRUE(planner->isGoalReached(4.9, 4.9, 5.0, 5.0, 0.2));
}

TEST_F(RRTPlannerTest, IsGoalReached_OutsideTolerance) {
    EXPECT_FALSE(planner->isGoalReached(4.5, 4.5, 5.0, 5.0, 0.2));
}

// ==============================================================================
// 4. Path Output Phase Tests
// ==============================================================================

TEST_F(RRTPlannerTest, ExtractPath_ReversesCorrectly) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    TreeNode* mid = new TreeNode(1.0, 1.0, start);
    TreeNode* goal = new TreeNode(2.0, 2.0, mid);
    
    auto path = planner->extractPath(goal);
    
    EXPECT_EQ(path.size(), 3);
    if(path.size() == 3) {
        EXPECT_FLOAT_EQ(path[0].x, 0.0);
        EXPECT_FLOAT_EQ(path[2].x, 2.0);
    }
    
    delete start; delete mid; delete goal;
}

// ==============================================================================
// 5. Main Interface Tests
// ==============================================================================

TEST_F(RRTPlannerTest, FindPath_SuccessEmptyMap) {
    std::vector<int8_t> flat_map(100, 0); // 10x10 free map
    auto path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9);
    
    // RRT is random, but it should definitely find a path on an empty map
    EXPECT_GT(path.size(), 0);
}

TEST_F(RRTPlannerTest, FindPath_StartInObstacle) {
    std::vector<int8_t> flat_map(100, 100); // 10x10 totally blocked map
    auto path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9);
    
    EXPECT_EQ(path.size(), 0); // Should fail instantly
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
