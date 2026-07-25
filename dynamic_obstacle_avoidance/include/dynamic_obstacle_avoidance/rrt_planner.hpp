#ifndef RRT_PLANNER_HPP
#define RRT_PLANNER_HPP

#include <vector>
#include <cmath>
#include <cstdint>
#include <random>
#include <chrono>
#include <utility>
#include <map>
#include <functional>

// Reusing Point2D from A*
#ifndef POINT2D_DEFINED
#define POINT2D_DEFINED
struct Point2D {
    double x;
    double y;
    float g_cost;
    float h_cost;
    double time_taken_ms;
};
#endif

// Represents a node in the continuous space tree
struct TreeNode {
    double x;
    double y;
    TreeNode* parent;
    std::chrono::time_point<std::chrono::high_resolution_clock> creation_time;

    TreeNode(double x_pos, double y_pos, TreeNode* p_node = nullptr) 
        : x(x_pos), y(y_pos), parent(p_node), creation_time(std::chrono::high_resolution_clock::now()) {}
};

class RRTPlanner {
public:
    RRTPlanner();
    ~RRTPlanner();

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

    std::vector<Point2D> getTreeAsPath() const { return tree_path_; }

    // --- 2. Coordinate Conversion ---
    int worldToGrid(double world_coord, double origin, double resolution);
    double gridToWorld(int grid_index, double origin, double resolution);

    // --- 3. Distance ---
    double calculateDistance(double x1, double y1, double x2, double y2);

    // --- 4. Map Validation (Bounds Check Phase) ---
    bool isWithinBounds(int x, int y, int width, int height);
    
    // --- 5. Map Validation (Obstacle Check Phase) ---
    bool isObstacle(const std::vector<std::vector<int>>& grid, int x, int y);

    // --- 6. Path Validation ---
    bool isPathCollisionFree(double x1, double y1, double x2, double y2, const std::vector<std::vector<int>>& grid, double origin_x, double origin_y, double resolution);

    // --- 7. Core RRT Mechanics ---
    Point2D getRandomPoint(double goal_x, double goal_y, double min_x, double max_x, double min_y, double max_y);
    TreeNode* getNearestNode(const std::vector<TreeNode*>& tree, double rand_x, double rand_y);
    TreeNode steer(TreeNode* nearest_node, double rand_x, double rand_y, double step_size);
    bool isGoalReached(double x, double y, double goal_x, double goal_y, double tolerance);

    // --- 8. Path Output Phase ---
    std::vector<Point2D> extractPath(TreeNode* final_node, std::chrono::time_point<std::chrono::high_resolution_clock> start_time);

private:
    bool debug_mode_ = false;
    std::mt19937 rng_;
    std::vector<Point2D> tree_path_;
    
    // Helper to build 2D grid from 1D array
    std::vector<std::vector<int>> buildGridFromMap(const std::vector<int8_t>& flat_map_data, int width, int height);
};

#endif // RRT_PLANNER_HPP
