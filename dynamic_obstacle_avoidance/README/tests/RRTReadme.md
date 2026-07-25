# RRT Planner Unit Tests

This document describes the 60 unit test cases (5 per function) created for the `RRTPlanner` class. The tests are written using the Google Test (`gtest`) framework.

## 1. worldToGrid Tests
Tests the conversion from physical world coordinates to discrete grid indices.
1.  **`WorldToGrid_PositiveExact`**: Verifies that a positive floating-point coordinate correctly maps to a precise grid index.
2.  **`WorldToGrid_PositiveRoundDown`**: Verifies that a decimal value appropriately rounds down to the nearest grid index.
3.  **`WorldToGrid_NegativeExact`**: Verifies conversion works correctly for negative world coordinates.
4.  **`WorldToGrid_ZeroOrigin`**: Ensures a zero coordinate correctly maps to index zero when origin is zero.
5.  **`WorldToGrid_NegativeOrigin`**: Validates conversion scaling when the map's origin is offset (negative).

## 2. gridToWorld Tests
Tests the inverse conversion from grid indices back to physical world coordinates.
1.  **`GridToWorld_PositiveIndex`**: Verifies conversion of a standard positive index.
2.  **`GridToWorld_ZeroIndex`**: Verifies that index zero maps exactly to the origin coordinate.
3.  **`GridToWorld_NegativeIndex`**: Checks that negative indices output the correct negative world coordinates.
4.  **`GridToWorld_NegativeOrigin`**: Validates reverse conversion when dealing with a negative map origin.
5.  **`GridToWorld_HighResolution`**: Ensures the conversion correctly scales with a highly detailed map resolution (e.g., 0.001).

## 3. calculateDistance Tests
Validates Euclidean distance calculations in continuous 2D space.
1.  **`CalculateDistance_SamePoint`**: Checks that the distance from a point to itself is exactly `0.0`.
2.  **`CalculateDistance_HorizontalLine`**: Verifies distance calculation across the x-axis only.
3.  **`CalculateDistance_VerticalLine`**: Verifies distance calculation across the y-axis only.
4.  **`CalculateDistance_Diagonal`**: Uses a 3-4-5 triangle to verify standard diagonal distance calculations.
5.  **`CalculateDistance_NegativeCoordinates`**: Ensures distance calculations correctly handle negative starting and ending points.

## 4. isWithinBounds Tests
Ensures array index checks safely prevent out-of-bounds segmentation faults.
1.  **`IsWithinBounds_Center`**: Tests an index located safely in the middle of the grid.
2.  **`IsWithinBounds_TopLeftEdge`**: Tests the absolute `(0, 0)` edge of the grid.
3.  **`IsWithinBounds_NegativeX`**: Ensures a negative X coordinate returns false.
4.  **`IsWithinBounds_NegativeY`**: Ensures a negative Y coordinate returns false.
5.  **`IsWithinBounds_ExceedsDimensions`**: Ensures coordinates that exceed or equal the map width/height return false.

## 5. isObstacle Tests
Validates the grid occupancy logic for collision checking.
1.  **`IsObstacle_FreeSpace`**: Verifies that a known empty `0` cell returns false.
2.  **`IsObstacle_HitCenter`**: Verifies that a known occupied `1` cell returns true.
3.  **`IsObstacle_HitEdge`**: Checks for an obstacle hit along the outer edge of the map.
4.  **`IsObstacle_MultipleObstacles_Hit`**: Creates a complex map and ensures picking an obstacle cell correctly returns true.
5.  **`IsObstacle_MultipleObstacles_Miss`**: Creates a complex map and ensures picking an empty cell adjacent to obstacles correctly returns false.

## 6. isPathCollisionFree Tests
Validates that straight lines between two points in continuous space do not intersect obstacle grid cells.
1.  **`IsPathCollisionFree_ClearPath`**: Simulates a straight line across multiple free grid cells and verifies it is unblocked.
2.  **`IsPathCollisionFree_BlockedMiddle`**: Simulates a straight line that crosses a single obstacle wall in the middle, ensuring it returns false.
3.  **`IsPathCollisionFree_BlockedStart`**: Ensures it returns false if the starting point of the line is inside an obstacle.
4.  **`IsPathCollisionFree_BlockedEnd`**: Ensures it returns false if the destination point of the line is inside an obstacle.
5.  **`IsPathCollisionFree_DiagonalClear`**: Specifically tests a diagonal trajectory threading between obstacles to ensure free paths aren't falsely flagged.

## 7. getRandomPoint Tests
Validates the randomized sampling logic of the RRT algorithm.
1.  **`GetRandomPoint_WithinLimits_Center`**: Ensures the generated coordinates strictly fall within standard minimum and maximum boundaries.
2.  **`GetRandomPoint_WithinLimits_Edges`**: Tests edge case boundaries (starting exactly from 0.0).
3.  **`GetRandomPoint_RespectsMinBounds`**: Explicitly tests that generated points are never less than the map's minimum physical coordinates.
4.  **`GetRandomPoint_RespectsMaxBounds`**: Explicitly tests that generated points are never greater than the map's maximum physical coordinates.
5.  **`GetRandomPoint_NegativeMapLimits`**: Validates randomness correctly operates within negative coordinate spaces (e.g., -5.0 to 0.0).

## 8. getNearestNode Tests
Validates the closest-node search mechanic in the RRT tree.
1.  **`GetNearestNode_ExactMatch`**: Tests providing a target point that lands exactly on top of an existing node in the tree.
2.  **`GetNearestNode_ClearClosest`**: Tests a target point that is blatantly closest to only one specific node in the tree.
3.  **`GetNearestNode_EquidistantTiebreaker`**: Tests behavior when a target point is exactly equidistant between two existing nodes.
4.  **`GetNearestNode_NegativeCoordinates`**: Ensures nearest node searching functions correctly with negative values.
5.  **`GetNearestNode_LargeDistance`**: Tests nearest node searching when nodes are thousands of units apart.

## 9. steer Tests
Validates the algorithm's ability to take fixed-size steps toward a random target.
1.  **`Steer_StraightX`**: Verifies steering horizontally across the x-axis.
2.  **`Steer_StraightY`**: Verifies steering vertically across the y-axis.
3.  **`Steer_Diagonal`**: Verifies steering diagonally toward a point using Pythagorean math.
4.  **`Steer_StepSizeLargerThanDistance`**: Tests the edge case where the step size is larger than the distance to the target; it should cap at the target point.
5.  **`Steer_NegativeDirection`**: Verifies steering correctly calculates angles and outputs negative coordinates when steering backwards.

## 10. isGoalReached Tests
Validates the continuous-space goal tolerance check.
1.  **`IsGoalReached_ExactMatch`**: Tests a node resting on the exact mathematical coordinate of the goal.
2.  **`IsGoalReached_InsideTolerance`**: Tests a node inside the designated acceptable goal radius.
3.  **`IsGoalReached_OutsideTolerance`**: Tests a node slightly outside the acceptable goal radius (should return false).
4.  **`IsGoalReached_OnToleranceBoundary`**: Tests a node sitting precisely on the border of the tolerance radius.
5.  **`IsGoalReached_NegativeCoordinates`**: Ensures the tolerance radius check works for negative goals.

## 11. extractPath Tests
Validates the tree-tracing logic that converts the RRT structure into a sequential path.
1.  **`ExtractPath_SingleNode`**: Tests extraction when the start node is immediately the goal node.
2.  **`ExtractPath_TwoNodes`**: Tests extraction on a simple Start -> Goal tree.
3.  **`ExtractPath_MultipleNodes`**: Tests tracing a long lineage of nested parent nodes.
4.  **`ExtractPath_ReversesCorrectly`**: Ensures the extracted path begins at the Start point and finishes at the Goal point (not backwards).
5.  **`ExtractPath_CheckValues`**: Verifies that the continuous (x, y) coordinate values are perfectly preserved during extraction.

## 12. findPath Tests
Tests the main public interface for the entire RRT planner.
1.  **`FindPath_SuccessEmptyMap`**: Provides a completely free map and ensures the RRT algorithm successfully finds a valid path.
2.  **`FindPath_StartInObstacle`**: Places the start position inside an obstacle to verify the algorithm safely aborts.
3.  **`FindPath_GoalInObstacle`**: Places the goal inside an obstacle to verify the planner exits safely after exhausting iterations.
4.  **`FindPath_OutOfBoundsGoal`**: Ensures providing a goal outside the physical map fails gracefully.
5.  **`FindPath_PathAroundSimpleObstacle`**: Creates a physical wall with a small gap, ensuring the planner correctly navigates the gap to reach the goal.
