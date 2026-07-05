# A* Planner Unit Tests

This document describes the unit test cases created for the `AStarPlanner` class. The tests are written using the Google Test (`gtest`) framework.

## 1. worldToGrid() Tests
Tests the conversion from physical world coordinates to discrete grid indices.
*   **Simple Cases**: Tests positive values, rounding up/down, and origin at `0.0`.
*   **Negative World Coordinates**: Checks correct behavior when coordinates are negative.
*   **Real ROS Maps**: Tests with an origin that is offset (negative origin), which is common in SLAM.
*   **Different Resolutions**: Verifies conversion scaling with different map resolutions.

## 2. gridToWorld() Tests
Tests the inverse conversion from grid indices back to physical world coordinates.
*   **Simple Cases**: Tests index `0` and positive indices.
*   **Math with Negative Numbers**: Tests negative indices.
*   **Real ROS Maps**: Validates conversion using negative origins.
*   **Different Resolutions**: Checks output against various resolutions.

## 3. isWithinBounds() Tests
Ensures array index checks do not cause segfaults.
*   **Inside the Map**: Tests center and edge cells.
*   **Outside the Map (Negative)**: Validates bounds checking against `-1`.
*   **Outside the Map (Too Big)**: Validates bounds checking against values greater than width/height.

## 4. isObstacle() Tests
Validates the grid occupancy logic.
*   **Free Space**: Checks a known `0` cell.
*   **Wall Center & Corner**: Checks known `1` (obstacle) cells.

## 5. buildGridFromMap() Tests
Validates the conversion of a flat 1D array to a 2D vector for the map structure.
*   **All Free Space**: Input of `0`s maps to a 2D grid of `0`s.
*   **All Obstacles**: Input of `100`s maps to a 2D grid of `1`s.
*   **Mixed Map with Thresholds**: Verifies that values like `50` (unknown/thresholded) or `-1` (unexplored) map to the appropriate free/obstacle states based on our logic.

## 6. calculateMoveCost() Tests
Validates movement costs.
*   **Straight Moves**: Checks Up, Down, Left, Right (should cost 1.0).
*   **Diagonal Moves**: Checks the four diagonals (should cost ~1.414).

## 7. calculateHeuristic() Tests
Validates the Euclidean distance heuristic.
*   **Same Point**: Should be 0.
*   **Horizontal / Vertical Line**: Should match the distance.
*   **Perfect Diagonal / Negative Coordinates**: Verifies the distance logic utilizing Pythagorean theorem (3-4-5 triangle).

## 8. isCornerCutting() Tests
Ensures the path does not cut diagonally across the corners of adjacent walls.
*   **Straight Move**: Validates straight moves are unhindered by adjacent corners.
*   **Diagonal Safe**: Validates a valid diagonal move.
*   **Diagonal One/Two Walls**: Validates that if a diagonal move grazes the corner of 1 or 2 walls, it is blocked.

## 9. getValidNeighbors() Tests
Integrates bounds, obstacles, and corner-cutting to filter neighbors.
*   **Gauntlet**: Given a complex layout with walls and visited nodes (`closed_list`), verifies that only the valid moves are returned and with correct costs attached.

## 10. computePath() Tests
Tests the internal A* search logic.
*   **Path Found**: Verifies that a traversable path can be found from Start to Goal.
*   **No Path Possible**: Verifies that if the Goal is completely walled off, the planner gracefully returns an empty path.

## 11. convertGridPathToWorldPath() Tests
Tests the post-processing phase.
*   **Reversal and Scaling**: Takes a backwards grid path (Goal -> Start), and ensures it reverses to (Start -> Goal) while properly scaling the grid coordinates back into physical map coordinates.

## 12. findPath() Tests (The Ultimate Boss)
Tests the main public interface for the entire planner.
*   **Success**: Validates the end-to-end functionality.
*   **Fail - Out of Bounds**: Ensures providing a goal outside the map fails safely.
*   **Fail - Start in Wall**: Ensures providing a start coordinate inside a wall fails safely.
