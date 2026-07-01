#ifndef A_STAR_PLANNER_HPP
#define A_STAR_PLANNER_HPP

#include <vector>
#include <cmath>
#include <cstdint>

// Used to return the final path
struct Point2D {
    double x;
    double y;
    float g_cost;
    float h_cost;
};

// Represents a single cell in the grid
struct GridNode {
    int x, y; // In our code, x = row (grid_y) and y = col (grid_x)
    float g_cost, h_cost;
    GridNode* parent;

    GridNode(int x_pos, int y_pos);
};

class AStarPlanner {
public:
    // --- 1. Main Interface ---
    std::vector<Point2D> findPath(
        const std::vector<int8_t>& map_data, 
        int width, 
        int height, 
        double resolution, 
        double origin_x, 
        double origin_y, 
        double start_x, 
        double start_y, 
        double goal_x, 
        double goal_y);
        
    void setDebugMode(bool debug) { debug_mode_ = debug; }

    // --- 2. Grid Construction (Map Creation Phase) ---
    std::vector<std::vector<int>> buildGridFromMap(const std::vector<int8_t>& flat_map_data, int width, int height);

    // --- 3. Coordinate Conversion (Input Phase) ---
    int worldToGrid(double world_coord, double origin, double resolution);

    // --- 4. Map Validation (Bounds Check Phase) ---
    bool isWithinBounds(int x, int y, int width, int height);
    
    // --- 5. Map Validation (Obstacle Check Phase) ---
    bool isObstacle(const std::vector<std::vector<int>>& grid, int x, int y);

    // --- 6. Core A* Mechanics (Path Calculation Phase) ---
    std::vector<GridNode*> getValidNeighbors(GridNode* current_node, const std::vector<std::vector<int>>& grid, const std::vector<std::vector<bool>>& closed_list, GridNode goal);
    bool isCornerCutting(const std::vector<std::vector<int>>& grid, int current_x, int current_y, int dx, int dy);
    float calculateMoveCost(int dx, int dy);

    // --- 7. Path Output Conversion (Final Phase) ---
    std::vector<Point2D> convertGridPathToWorldPath(const std::vector<GridNode*>& grid_path, double origin_x, double origin_y, double resolution);
    double gridToWorld(int grid_index, double origin, double resolution);

private:
    // --- Internal Engine ---
    std::vector<GridNode*> computePath(const std::vector<std::vector<int>>& grid, GridNode start, GridNode goal);
    float calculateHeuristic(int x1, int y1, int x2, int y2);
    
    bool debug_mode_ = false;
};

#endif // A_STAR_PLANNER_HPP
