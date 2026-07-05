#include <gtest/gtest.h>
#include "dynamic_obstacle_avoidance/rrt_planner.hpp"
#include <memory>
#include <cmath>
#include <vector>

class RRTPlannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner = std::make_shared<RRTPlanner>();
    }
    std::shared_ptr<RRTPlanner> planner;
};

// ==============================================================================
// 1. worldToGrid Tests (5 cases)
// =============================================================================
TEST_F(RRTPlannerTest, WorldToGrid_PositiveExact) {
    EXPECT_EQ(planner->worldToGrid(1.0, 0.0, 0.1), 10);
}
TEST_F(RRTPlannerTest, WorldToGrid_PositiveRoundDown) {
    EXPECT_EQ(planner->worldToGrid(1.04, 0.0, 0.1), 10);
}
TEST_F(RRTPlannerTest, WorldToGrid_NegativeExact) {
    EXPECT_EQ(planner->worldToGrid(-1.0, 0.0, 0.1), -10);
}
TEST_F(RRTPlannerTest, WorldToGrid_ZeroOrigin) {
    EXPECT_EQ(planner->worldToGrid(0.0, 0.0, 0.1), 0);
}
TEST_F(RRTPlannerTest, WorldToGrid_NegativeOrigin) {
    EXPECT_EQ(planner->worldToGrid(1.0, -2.0, 0.1), 30);
}

// ==============================================================================
// 2. gridToWorld Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, GridToWorld_PositiveIndex) {
    EXPECT_NEAR(planner->gridToWorld(10, 0.0, 0.1), 1.0, 0.0001);
}
TEST_F(RRTPlannerTest, GridToWorld_ZeroIndex) {
    EXPECT_NEAR(planner->gridToWorld(0, 0.0, 0.1), 0.0, 0.0001);
}
TEST_F(RRTPlannerTest, GridToWorld_NegativeIndex) {
    EXPECT_NEAR(planner->gridToWorld(-10, 0.0, 0.1), -1.0, 0.0001);
}
TEST_F(RRTPlannerTest, GridToWorld_NegativeOrigin) {
    EXPECT_NEAR(planner->gridToWorld(30, -2.0, 0.1), 1.0, 0.0001);
}
TEST_F(RRTPlannerTest, GridToWorld_HighResolution) {
    EXPECT_NEAR(planner->gridToWorld(5, 0.0, 0.001), 0.005, 0.0001);
}

// ==============================================================================
// 3. calculateDistance Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, CalculateDistance_SamePoint) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(0.0, 0.0, 0.0, 0.0), 0.0);
}
TEST_F(RRTPlannerTest, CalculateDistance_HorizontalLine) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(0.0, 0.0, 3.0, 0.0), 3.0);
}
TEST_F(RRTPlannerTest, CalculateDistance_VerticalLine) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(0.0, 0.0, 0.0, 4.0), 4.0);
}
TEST_F(RRTPlannerTest, CalculateDistance_Diagonal) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(0.0, 0.0, 3.0, 4.0), 5.0);
}
TEST_F(RRTPlannerTest, CalculateDistance_NegativeCoordinates) {
    EXPECT_FLOAT_EQ(planner->calculateDistance(-1.0, -1.0, 2.0, 3.0), 5.0);
}

// ==============================================================================
// 4. isWithinBounds Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, IsWithinBounds_Center) {
    EXPECT_TRUE(planner->isWithinBounds(5, 5, 10, 10));
}
TEST_F(RRTPlannerTest, IsWithinBounds_TopLeftEdge) {
    EXPECT_TRUE(planner->isWithinBounds(0, 0, 10, 10));
}
TEST_F(RRTPlannerTest, IsWithinBounds_NegativeX) {
    EXPECT_FALSE(planner->isWithinBounds(-1, 5, 10, 10));
}
TEST_F(RRTPlannerTest, IsWithinBounds_NegativeY) {
    EXPECT_FALSE(planner->isWithinBounds(5, -1, 10, 10));
}
TEST_F(RRTPlannerTest, IsWithinBounds_ExceedsDimensions) {
    EXPECT_FALSE(planner->isWithinBounds(10, 10, 10, 10));
}

// ==============================================================================
// 5. isObstacle Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, IsObstacle_FreeSpace) {
    std::vector<std::vector<int>> grid = {{0, 0}, {0, 0}};
    EXPECT_FALSE(planner->isObstacle(grid, 0, 0));
}
TEST_F(RRTPlannerTest, IsObstacle_HitCenter) {
    std::vector<std::vector<int>> grid = {{0, 0, 0}, {0, 1, 0}, {0, 0, 0}};
    EXPECT_TRUE(planner->isObstacle(grid, 1, 1));
}
TEST_F(RRTPlannerTest, IsObstacle_HitEdge) {
    std::vector<std::vector<int>> grid = {{1, 0}, {0, 0}};
    EXPECT_TRUE(planner->isObstacle(grid, 0, 0));
}
TEST_F(RRTPlannerTest, IsObstacle_MultipleObstacles_Hit) {
    std::vector<std::vector<int>> grid = {{1, 0, 1}, {0, 1, 0}, {1, 0, 1}};
    EXPECT_TRUE(planner->isObstacle(grid, 0, 2));
}
TEST_F(RRTPlannerTest, IsObstacle_MultipleObstacles_Miss) {
    std::vector<std::vector<int>> grid = {{1, 0, 1}, {0, 1, 0}, {1, 0, 1}};
    EXPECT_FALSE(planner->isObstacle(grid, 1, 2));
}

// ==============================================================================
// 6. isPathCollisionFree Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, IsPathCollisionFree_ClearPath) {
    std::vector<std::vector<int>> grid(5, std::vector<int>(5, 0));
    EXPECT_TRUE(planner->isPathCollisionFree(0.0, 0.0, 0.4, 0.4, grid, 0.0, 0.0, 0.1));
}
TEST_F(RRTPlannerTest, IsPathCollisionFree_BlockedMiddle) {
    std::vector<std::vector<int>> grid(5, std::vector<int>(5, 0));
    grid[2][2] = 1;
    EXPECT_FALSE(planner->isPathCollisionFree(0.0, 0.0, 0.4, 0.4, grid, 0.0, 0.0, 0.1));
}
TEST_F(RRTPlannerTest, IsPathCollisionFree_BlockedStart) {
    std::vector<std::vector<int>> grid(5, std::vector<int>(5, 0));
    grid[0][0] = 1;
    EXPECT_FALSE(planner->isPathCollisionFree(0.0, 0.0, 0.4, 0.4, grid, 0.0, 0.0, 0.1));
}
TEST_F(RRTPlannerTest, IsPathCollisionFree_BlockedEnd) {
    std::vector<std::vector<int>> grid(5, std::vector<int>(5, 0));
    grid[4][4] = 1;
    EXPECT_FALSE(planner->isPathCollisionFree(0.0, 0.0, 0.4, 0.4, grid, 0.0, 0.0, 0.1));
}
TEST_F(RRTPlannerTest, IsPathCollisionFree_DiagonalClear) {
    std::vector<std::vector<int>> grid = {{0, 1, 1}, {1, 0, 1}, {1, 1, 0}};
    EXPECT_TRUE(planner->isPathCollisionFree(0.0, 0.0, 0.2, 0.2, grid, 0.0, 0.0, 0.1));
}

// ==============================================================================
// 7. getRandomPoint Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, GetRandomPoint_WithinLimits_Center) {
    auto pt = planner->getRandomPoint(5.0, 5.0, 0.0, 10.0, 0.0, 10.0);
    EXPECT_GE(pt.x, 0.0); EXPECT_LE(pt.x, 10.0);
    EXPECT_GE(pt.y, 0.0); EXPECT_LE(pt.y, 10.0);
}
TEST_F(RRTPlannerTest, GetRandomPoint_WithinLimits_Edges) {
    auto pt = planner->getRandomPoint(0.0, 0.0, 0.0, 10.0, 0.0, 10.0);
    EXPECT_GE(pt.x, 0.0); EXPECT_LE(pt.x, 10.0);
    EXPECT_GE(pt.y, 0.0); EXPECT_LE(pt.y, 10.0);
}
TEST_F(RRTPlannerTest, GetRandomPoint_RespectsMinBounds) {
    auto pt = planner->getRandomPoint(5.0, 5.0, 5.0, 10.0, 5.0, 10.0);
    EXPECT_GE(pt.x, 5.0); EXPECT_GE(pt.y, 5.0);
}
TEST_F(RRTPlannerTest, GetRandomPoint_RespectsMaxBounds) {
    auto pt = planner->getRandomPoint(-5.0, -5.0, -10.0, -5.0, -10.0, -5.0);
    EXPECT_LE(pt.x, -5.0); EXPECT_LE(pt.y, -5.0);
}
TEST_F(RRTPlannerTest, GetRandomPoint_NegativeMapLimits) {
    auto pt = planner->getRandomPoint(-1.0, -1.0, -5.0, 0.0, -5.0, 0.0);
    EXPECT_GE(pt.x, -5.0); EXPECT_LE(pt.x, 0.0);
    EXPECT_GE(pt.y, -5.0); EXPECT_LE(pt.y, 0.0);
}

// ==============================================================================
// 8. getNearestNode Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, GetNearestNode_ExactMatch) {
    TreeNode* n1 = new TreeNode(1.0, 1.0, nullptr);
    std::vector<TreeNode*> tree = {n1};
    EXPECT_EQ(planner->getNearestNode(tree, 1.0, 1.0), n1);
    delete n1;
}
TEST_F(RRTPlannerTest, GetNearestNode_ClearClosest) {
    TreeNode* n1 = new TreeNode(0.0, 0.0, nullptr);
    TreeNode* n2 = new TreeNode(10.0, 10.0, nullptr);
    std::vector<TreeNode*> tree = {n1, n2};
    EXPECT_EQ(planner->getNearestNode(tree, 2.0, 2.0), n1);
    delete n1; delete n2;
}
TEST_F(RRTPlannerTest, GetNearestNode_EquidistantTiebreaker) {
    TreeNode* n1 = new TreeNode(-2.0, 0.0, nullptr);
    TreeNode* n2 = new TreeNode(2.0, 0.0, nullptr);
    std::vector<TreeNode*> tree = {n1, n2};
    // Result can be either n1 or n2, but it shouldn't crash
    TreeNode* res = planner->getNearestNode(tree, 0.0, 0.0);
    EXPECT_TRUE(res == n1 || res == n2);
    delete n1; delete n2;
}
TEST_F(RRTPlannerTest, GetNearestNode_NegativeCoordinates) {
    TreeNode* n1 = new TreeNode(-5.0, -5.0, nullptr);
    TreeNode* n2 = new TreeNode(0.0, 0.0, nullptr);
    std::vector<TreeNode*> tree = {n1, n2};
    EXPECT_EQ(planner->getNearestNode(tree, -4.0, -4.0), n1);
    delete n1; delete n2;
}
TEST_F(RRTPlannerTest, GetNearestNode_LargeDistance) {
    TreeNode* n1 = new TreeNode(0.0, 0.0, nullptr);
    TreeNode* n2 = new TreeNode(1000.0, 1000.0, nullptr);
    std::vector<TreeNode*> tree = {n1, n2};
    EXPECT_EQ(planner->getNearestNode(tree, 900.0, 900.0), n2);
    delete n1; delete n2;
}

// ==============================================================================
// 9. steer Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, Steer_StraightX) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    auto next = planner->steer(start, 5.0, 0.0, 1.0);
    EXPECT_FLOAT_EQ(next.x, 1.0);
    EXPECT_FLOAT_EQ(next.y, 0.0);
    delete start;
}
TEST_F(RRTPlannerTest, Steer_StraightY) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    auto next = planner->steer(start, 0.0, 5.0, 2.0);
    EXPECT_NEAR(next.x, 0.0, 1e-6);
    EXPECT_FLOAT_EQ(next.y, 2.0);
    delete start;
}
TEST_F(RRTPlannerTest, Steer_Diagonal) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    auto next = planner->steer(start, 3.0, 4.0, 2.5); // Move halfway along 3-4-5 triangle
    EXPECT_FLOAT_EQ(next.x, 1.5);
    EXPECT_FLOAT_EQ(next.y, 2.0);
    delete start;
}
TEST_F(RRTPlannerTest, Steer_StepSizeLargerThanDistance) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    auto next = planner->steer(start, 1.0, 0.0, 5.0);
    EXPECT_FLOAT_EQ(next.x, 1.0); // Should stop at target if step_size > distance
    EXPECT_FLOAT_EQ(next.y, 0.0);
    delete start;
}
TEST_F(RRTPlannerTest, Steer_NegativeDirection) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    auto next = planner->steer(start, -10.0, 0.0, 1.0);
    EXPECT_FLOAT_EQ(next.x, -1.0);
    EXPECT_NEAR(next.y, 0.0, 1e-6);
    delete start;
}

// ==============================================================================
// 10. isGoalReached Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, IsGoalReached_ExactMatch) {
    EXPECT_TRUE(planner->isGoalReached(5.0, 5.0, 5.0, 5.0, 0.5));
}
TEST_F(RRTPlannerTest, IsGoalReached_InsideTolerance) {
    EXPECT_TRUE(planner->isGoalReached(4.8, 4.8, 5.0, 5.0, 0.5));
}
TEST_F(RRTPlannerTest, IsGoalReached_OutsideTolerance) {
    EXPECT_FALSE(planner->isGoalReached(4.0, 4.0, 5.0, 5.0, 0.5));
}
TEST_F(RRTPlannerTest, IsGoalReached_OnToleranceBoundary) {
    EXPECT_TRUE(planner->isGoalReached(4.5, 5.0, 5.0, 5.0, 0.5));
}
TEST_F(RRTPlannerTest, IsGoalReached_NegativeCoordinates) {
    EXPECT_TRUE(planner->isGoalReached(-4.9, -4.9, -5.0, -5.0, 0.2));
}

// ==============================================================================
// 11. extractPath Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, ExtractPath_SingleNode) {
    TreeNode* goal = new TreeNode(0.0, 0.0, nullptr);
    auto path = planner->extractPath(goal);
    EXPECT_EQ(path.size(), 1);
    delete goal;
}
TEST_F(RRTPlannerTest, ExtractPath_TwoNodes) {
    TreeNode* start = new TreeNode(0.0, 0.0, nullptr);
    TreeNode* goal = new TreeNode(1.0, 1.0, start);
    auto path = planner->extractPath(goal);
    EXPECT_EQ(path.size(), 2);
    EXPECT_FLOAT_EQ(path[0].x, 0.0);
    delete start; delete goal;
}
TEST_F(RRTPlannerTest, ExtractPath_MultipleNodes) {
    TreeNode* n1 = new TreeNode(0.0, 0.0, nullptr);
    TreeNode* n2 = new TreeNode(1.0, 1.0, n1);
    TreeNode* n3 = new TreeNode(2.0, 2.0, n2);
    TreeNode* n4 = new TreeNode(3.0, 3.0, n3);
    auto path = planner->extractPath(n4);
    EXPECT_EQ(path.size(), 4);
    delete n1; delete n2; delete n3; delete n4;
}
TEST_F(RRTPlannerTest, ExtractPath_ReversesCorrectly) {
    TreeNode* n1 = new TreeNode(0.0, 0.0, nullptr);
    TreeNode* n2 = new TreeNode(1.0, 1.0, n1);
    TreeNode* n3 = new TreeNode(2.0, 2.0, n2);
    auto path = planner->extractPath(n3);
    if(path.size() == 3) {
        EXPECT_FLOAT_EQ(path[0].x, 0.0);
        EXPECT_FLOAT_EQ(path[2].x, 2.0);
    }
    delete n1; delete n2; delete n3;
}
TEST_F(RRTPlannerTest, ExtractPath_CheckValues) {
    TreeNode* start = new TreeNode(-1.0, 2.0, nullptr);
    TreeNode* goal = new TreeNode(3.5, -4.2, start);
    auto path = planner->extractPath(goal);
    if(path.size() == 2) {
        EXPECT_FLOAT_EQ(path[0].x, -1.0); EXPECT_FLOAT_EQ(path[0].y, 2.0);
        EXPECT_FLOAT_EQ(path[1].x, 3.5);  EXPECT_FLOAT_EQ(path[1].y, -4.2);
    }
    delete start; delete goal;
}

// ==============================================================================
// 12. findPath Tests (5 cases)
// ==============================================================================
TEST_F(RRTPlannerTest, FindPath_SuccessEmptyMap) {
    std::vector<int8_t> flat_map(100, 0); // 10x10 free map
    auto path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9);
    EXPECT_GT(path.size(), 0);
}
TEST_F(RRTPlannerTest, FindPath_StartInObstacle) {
    std::vector<int8_t> flat_map(100, 100); // Totally blocked map
    auto path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9);
    EXPECT_EQ(path.size(), 0);
}
TEST_F(RRTPlannerTest, FindPath_GoalInObstacle) {
    std::vector<int8_t> flat_map(100, 0); 
    flat_map[99] = 100; // Goal is blocked
    auto path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9);
    EXPECT_EQ(path.size(), 0); // Planner should eventually fail to reach it
}
TEST_F(RRTPlannerTest, FindPath_OutOfBoundsGoal) {
    std::vector<int8_t> flat_map(100, 0);
    // Goal is (5.0, 5.0) on a 1.0x1.0m map
    auto path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 5.0, 5.0);
    EXPECT_EQ(path.size(), 0);
}
TEST_F(RRTPlannerTest, FindPath_PathAroundSimpleObstacle) {
    std::vector<int8_t> flat_map(100, 0);
    // Block the middle row completely
    for(int i=40; i<50; i++) { flat_map[i] = 100; }
    // Start at bottom (0,0), Goal at top (0.9, 0.9)
    auto path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9);
    // RRT might be able to go around the sides if bounds permit, 
    // or fail if it's completely blocking the whole width
    // Let's leave a gap at index 49 (x=9)
    flat_map[49] = 0;
    path = planner->findPath(flat_map, 10, 10, 0.1, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9);
    EXPECT_GT(path.size(), 0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
