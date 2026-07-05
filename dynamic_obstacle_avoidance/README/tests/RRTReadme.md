# RRT Planner Unit Tests

This document describes the unit test cases created for the `RRTPlanner` class. The tests are written using the Google Test (`gtest`) framework.

## 1. Coordinate & Distance Utilities Tests
These tests ensure that continuous physical coordinates correctly map to discrete grid indices and that geometric math works.
*   **`WorldToGrid_PositiveExact`**: Verifies that a positive floating-point coordinate correctly maps to a grid index.
*   **`GridToWorld_PositiveIndex`**: Verifies the inverse; grid index maps to the expected world coordinate.
*   **`CalculateDistance_SamePoint`**: Checks that the distance between a point and itself is exactly `0.0`.
*   **`CalculateDistance_StraightLine`**: Checks a simple horizontal line distance calculation.
*   **`CalculateDistance_Diagonal`**: Uses a 3-4-5 triangle logic to verify diagonal distance calculation.

## 2. Collision Checking Phase Tests
These tests ensure the planner correctly identifies map boundaries and obstacles.
*   **`IsWithinBounds_Center`**: Tests a coordinate well within the grid boundaries.
*   **`IsWithinBounds_OutOfBounds`**: Tests coordinates that are negative or exceed the grid width/height.
*   **`IsObstacle_FreeSpace`**: Verifies that a known free grid cell returns false for an obstacle check.
*   **`IsObstacle_Hit`**: Verifies that a known occupied grid cell returns true for an obstacle check.
*   **`IsPathCollisionFree_ClearPath`**: Simulates stepping across multiple empty grid cells and verifies the line is unblocked.
*   **`IsPathCollisionFree_BlockedPath`**: Simulates a straight line that crosses an obstacle wall, ensuring it returns false.

## 3. Core RRT Mechanics Tests
These test the randomized tree-growing mechanics of RRT.
*   **`GetRandomPoint_WithinLimits`**: Ensures that the generated random coordinates strictly fall within the map's minimum and maximum boundaries.
*   **`GetNearestNode_FindsClosest`**: Creates a mini-tree of 3 nodes and ensures the algorithm correctly identifies the node with the shortest Euclidean distance to a target point.
*   **`Steer_TowardsTarget`**: Ensures that given a start node, target point, and step size, the math correctly calculates a new point exactly `step_size` distance away along the correct vector.
*   **`IsGoalReached_WithinTolerance`**: Checks if a node inside the designated goal radius returns true.
*   **`IsGoalReached_OutsideTolerance`**: Checks if a node outside the designated goal radius returns false.

## 4. Path Output Phase Tests
These tests ensure the tree is correctly walked backward to generate the path.
*   **`ExtractPath_ReversesCorrectly`**: Builds a mock tree of three linked nodes (Start -> Mid -> Goal). Verifies that `extractPath(goal)` returns a vector of 3 points starting at the Start node and ending at the Goal node.

## 5. Main Interface Tests
These are integration-level tests hitting the main `findPath` function.
*   **`FindPath_SuccessEmptyMap`**: Provides a completely free map and ensures the RRT algorithm eventually finds a valid path from Start to Goal.
*   **`FindPath_StartInObstacle`**: Places the start position inside an obstacle to verify the algorithm immediately aborts and returns an empty path.
