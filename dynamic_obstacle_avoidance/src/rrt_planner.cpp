#include "dynamic_obstacle_avoidance/rrt_planner.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <limits>

RRTPlanner::RRTPlanner() {
    std::random_device rd;
    rng_ = std::mt19937(rd());
}

RRTPlanner::~RRTPlanner() {}

// ==========================================================
// 2. Coordinate Conversion
// ==========================================================
int RRTPlanner::worldToGrid(double world_coord, double origin, double resolution) {
    return static_cast<int>(std::floor((world_coord - origin) / resolution));
}

double RRTPlanner::gridToWorld(int grid_index, double origin, double resolution) {
    return (grid_index * resolution) + origin;
}

// ==========================================================
// 3. Distance
// ==========================================================
double RRTPlanner::calculateDistance(double x1, double y1, double x2, double y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

// ==========================================================
// 4. Map Validation (Bounds Check Phase)
// ==========================================================
bool RRTPlanner::isWithinBounds(int x, int y, int width, int height) {
    return (x >= 0 && x < width && y >= 0 && y < height);
}

// ==========================================================
// 5. Map Validation (Obstacle Check Phase)
// ==========================================================
bool RRTPlanner::isObstacle(const std::vector<std::vector<int>>& grid, int x, int y) {
    if (grid.empty()) return false;
    return grid[y][x] == 1; // y is row, x is col
}

std::vector<std::vector<int>> RRTPlanner::buildGridFromMap(const std::vector<int8_t>& flat_map_data, int width, int height) {
    std::vector<std::vector<int>> grid(height, std::vector<int>(width, 0));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = x + (y * width);
            if (flat_map_data[index] >= 50) {
                grid[y][x] = 1; 
            }
        }
    }
    return grid;
}

// ==========================================================
// 6. Path Validation
// ==========================================================
bool RRTPlanner::isPathCollisionFree(double x1, double y1, double x2, double y2, const std::vector<std::vector<int>>& grid, double origin_x, double origin_y, double resolution) {
    if (grid.empty()) return true;
    int height = grid.size();
    int width = grid[0].size();
    
    double dist = calculateDistance(x1, y1, x2, y2);
    int num_steps = static_cast<int>(dist / (resolution * 0.5)); // step size half of resolution for fine checking
    if (num_steps == 0) num_steps = 1;

    for (int i = 0; i <= num_steps; ++i) {
        double t = static_cast<double>(i) / num_steps;
        double cx = x1 + t * (x2 - x1);
        double cy = y1 + t * (y2 - y1);
        
        int gx = worldToGrid(cx, origin_x, resolution);
        int gy = worldToGrid(cy, origin_y, resolution);
        
        if (!isWithinBounds(gx, gy, width, height)) return false;
        if (isObstacle(grid, gx, gy)) return false;
    }
    return true;
}

// ==========================================================
// 7. Core RRT Mechanics
// ==========================================================
Point2D RRTPlanner::getRandomPoint(double goal_x, double goal_y, double min_x, double max_x, double min_y, double max_y) {
    std::uniform_real_distribution<double> dist_prob(0.0, 1.0);
    // 5% goal bias
    if (dist_prob(rng_) < 0.05) {
        return {goal_x, goal_y, 0.0f, 0.0f};
    }
    std::uniform_real_distribution<double> dist_x(min_x, max_x);
    std::uniform_real_distribution<double> dist_y(min_y, max_y);
    return {dist_x(rng_), dist_y(rng_), 0.0f, 0.0f};
}

TreeNode* RRTPlanner::getNearestNode(const std::vector<TreeNode*>& tree, double rand_x, double rand_y) {
    TreeNode* nearest = nullptr;
    double min_dist = std::numeric_limits<double>::max();
    for (TreeNode* node : tree) {
        double d = calculateDistance(node->x, node->y, rand_x, rand_y);
        if (d < min_dist) {
            min_dist = d;
            nearest = node;
        }
    }
    return nearest;
}

TreeNode RRTPlanner::steer(TreeNode* nearest_node, double rand_x, double rand_y, double step_size) {
    double dist = calculateDistance(nearest_node->x, nearest_node->y, rand_x, rand_y);
    if (dist <= step_size) {
        return TreeNode(rand_x, rand_y, nearest_node);
    }
    double theta = std::atan2(rand_y - nearest_node->y, rand_x - nearest_node->x);
    return TreeNode(nearest_node->x + step_size * std::cos(theta), nearest_node->y + step_size * std::sin(theta), nearest_node);
}

bool RRTPlanner::isGoalReached(double x, double y, double goal_x, double goal_y, double tolerance) {
    return calculateDistance(x, y, goal_x, goal_y) <= tolerance;
}

// ==========================================================
// 8. Path Output Phase
// ==========================================================
std::vector<Point2D> RRTPlanner::extractPath(TreeNode* final_node) {
    std::vector<Point2D> path;
    TreeNode* current = final_node;
    while (current != nullptr) {
        path.push_back({current->x, current->y, 0.0f, 0.0f});
        current = current->parent;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// ==========================================================
// 1. Main Interface
// ==========================================================
std::vector<Point2D> RRTPlanner::findPath(
    const std::vector<int8_t>& map_data, 
    int width, 
    int height, 
    double resolution, 
    double origin_x, 
    double origin_y, 
    double start_x, 
    double start_y, 
    double goal_x, 
    double goal_y) 
{
    std::vector<std::vector<int>> grid = buildGridFromMap(map_data, width, height);

    int start_gx = worldToGrid(start_x, origin_x, resolution);
    int start_gy = worldToGrid(start_y, origin_y, resolution);
    int goal_gx = worldToGrid(goal_x, origin_x, resolution);
    int goal_gy = worldToGrid(goal_y, origin_y, resolution);

    if (!isWithinBounds(start_gx, start_gy, width, height) || isObstacle(grid, start_gx, start_gy)) {
        if (debug_mode_) std::cout << "[RRTPlanner] Start is invalid or in obstacle." << std::endl;
        return {};
    }
    if (!isWithinBounds(goal_gx, goal_gy, width, height) || isObstacle(grid, goal_gx, goal_gy)) {
        if (debug_mode_) std::cout << "[RRTPlanner] Goal is invalid or in obstacle." << std::endl;
        return {};
    }

    std::vector<TreeNode*> tree;
    tree.push_back(new TreeNode(start_x, start_y));

    double min_x = origin_x;
    double max_x = origin_x + width * resolution;
    double min_y = origin_y;
    double max_y = origin_y + height * resolution;
    
    // Configurable parameters
    double step_size = resolution * 2.0; 
    double tolerance = resolution * 1.5; 
    int max_iters = 50000;

    auto total_start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < max_iters; ++i) {
        Point2D rand_pt = getRandomPoint(goal_x, goal_y, min_x, max_x, min_y, max_y);
        TreeNode* nearest = getNearestNode(tree, rand_pt.x, rand_pt.y);
        
        TreeNode new_node_data = steer(nearest, rand_pt.x, rand_pt.y, step_size);
        
        if (isPathCollisionFree(nearest->x, nearest->y, new_node_data.x, new_node_data.y, grid, origin_x, origin_y, resolution)) {
            TreeNode* new_node = new TreeNode(new_node_data.x, new_node_data.y, nearest);
            tree.push_back(new_node);
            
            if (isGoalReached(new_node->x, new_node->y, goal_x, goal_y, tolerance)) {
                if (debug_mode_) {
                    auto goal_found_time = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> total_duration = goal_found_time - total_start_time;
                    std::cout << "[RRTPlanner] DEBUG: Goal reached in " << i << " iterations. Total time: " << total_duration.count() << " ms." << std::endl;
                }
                
                std::vector<Point2D> path = extractPath(new_node);
                for (auto n : tree) delete n;
                return path;
            }
        }
    }

    if (debug_mode_) {
        auto fail_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> total_duration = fail_time - total_start_time;
        std::cout << "[RRTPlanner] DEBUG: Failed to reach goal after " << max_iters << " iterations. Total time: " << total_duration.count() << " ms." << std::endl;
    }
    
    for (auto n : tree) delete n;
    return {};
}
