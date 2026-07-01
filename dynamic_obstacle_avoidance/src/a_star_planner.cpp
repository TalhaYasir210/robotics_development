#include "dynamic_obstacle_avoidance/a_star_planner.hpp"
#include <cmath>
#include <queue>
#include <iostream>
#include <chrono>

// Custom comparator for the priority queue
struct CompareF {
    bool operator()(GridNode* a, GridNode* b) {
        return (a->g_cost + a->h_cost) > (b->g_cost + b->h_cost);
    }
};

// Updated to GridNode
GridNode::GridNode(int x_pos, int y_pos) {
    x = x_pos;
    y = y_pos;
    g_cost = 0.0;
    h_cost = 0.0;
    parent = nullptr;
}

// ==========================================================
// 1. Main Interface
// ==========================================================
std::vector<Point2D> AStarPlanner::findPath(
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
    // 1. Convert world coordinates to grid indices
    int start_grid_x = worldToGrid(start_x, origin_x, resolution);
    int start_grid_y = worldToGrid(start_y, origin_y, resolution);
    
    int goal_grid_x = worldToGrid(goal_x, origin_x, resolution);
    int goal_grid_y = worldToGrid(goal_y, origin_y, resolution);

    // 2. Bound checks
    // Note: bounds checking is against height (ROWS) and width (COLS)
    if (!isWithinBounds(start_grid_y, start_grid_x, height, width)) {
        std::cout << "[AStarPlanner] Start position is outside the map!" << std::endl;
        return {};
    }
    if (!isWithinBounds(goal_grid_y, goal_grid_x, height, width)) {
        std::cout << "[AStarPlanner] Goal position is outside the map!" << std::endl;
        return {};
    }

    // 3. Create 2D grid
    std::vector<std::vector<int>> grid = buildGridFromMap(map_data, width, height);

    // 4. Validate Start and Goal
    if (isObstacle(grid, start_grid_y, start_grid_x)) {
        std::cout << "[AStarPlanner] FAILED: Start position is inside an obstacle!" << std::endl;
        return {};
    }
    if (isObstacle(grid, goal_grid_y, goal_grid_x)) {
        std::cout << "[AStarPlanner] FAILED: Goal position is inside an obstacle!" << std::endl;
        return {};
    }

    // GridNode expects (row, col) which maps to (y, x) in grid indices
    GridNode start_node(start_grid_y, start_grid_x);
    GridNode goal_node(goal_grid_y, goal_grid_x);

    // 5. Compute Path
    std::vector<GridNode*> grid_path = computePath(grid, start_node, goal_node);

    // 6. Convert to world coordinates
    return convertGridPathToWorldPath(grid_path, origin_x, origin_y, resolution);
}

// ==========================================================
// 2. Setup & Grid Construction
// ==========================================================
std::vector<std::vector<int>> AStarPlanner::buildGridFromMap(const std::vector<int8_t>& flat_map_data, int width, int height) {
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
// 3. Coordinate Conversion (Input)
// ==========================================================
int AStarPlanner::worldToGrid(double world_coord, double origin, double resolution) {
    return std::round((world_coord - origin) / resolution);
}

// ==========================================================
// 4. Map Validation
// ==========================================================
bool AStarPlanner::isWithinBounds(int x, int y, int width, int height) {
    return (x >= 0 && x < width && y >= 0 && y < height);
}

bool AStarPlanner::isObstacle(const std::vector<std::vector<int>>& grid, int x, int y) {
    return grid[x][y] == 1; // 1 means obstacle
}

// ==========================================================
// 5. Core A* Mechanics & Engine
// ==========================================================
std::vector<GridNode*> AStarPlanner::computePath(const std::vector<std::vector<int>>& grid, GridNode start, GridNode goal) {
    int ROWS = grid.size();
    int COLS = grid[0].size();

    std::vector<std::vector<bool>> closed_list(ROWS, std::vector<bool>(COLS, false));
    std::priority_queue<GridNode*, std::vector<GridNode*>, CompareF> open_list;

    GridNode* start_node = new GridNode(start.x, start.y);
    start_node->g_cost = 0.0;
    start_node->h_cost = calculateHeuristic(start.x, start.y, goal.x, goal.y);
    open_list.push(start_node);

    int step_count = 0;
    auto total_start_time = std::chrono::high_resolution_clock::now();

    while (!open_list.empty()) {
        auto step_start_time = std::chrono::high_resolution_clock::now();
        step_count++;

        GridNode* current_node = open_list.top();
        open_list.pop();

        if (closed_list[current_node->x][current_node->y]) {
            continue;
        }

        closed_list[current_node->x][current_node->y] = true;

        if (current_node->x == goal.x && current_node->y == goal.y) {
            if (debug_mode_) {
                auto goal_found_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> total_duration = goal_found_time - total_start_time;
                // Note: goal.x stores the Row (Y), and goal.y stores the Col (X).
                // We print (X, Y) here so it matches the final table output visually.
                std::cout << "[AStarPlanner] DEBUG: Calculated the right solution for the goal coordinates (" 
                          << goal.y << ", " << goal.x << ") in " << step_count << " steps. Total time: " 
                          << total_duration.count() << " ms." << std::endl;
            }

            std::vector<GridNode*> final_path;
            GridNode* trace_node = current_node;
            
            while (trace_node != nullptr) {
                final_path.push_back(trace_node);
                trace_node = trace_node->parent;
            }
            
            return final_path; 
        }

        // Generate and evaluate neighbors
        std::vector<GridNode*> neighbors = getValidNeighbors(current_node, grid, closed_list, goal);
        for (GridNode* neighbor : neighbors) {
            open_list.push(neighbor);
        }

        if (debug_mode_) {
            auto step_end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> step_duration = step_end_time - step_start_time;
            std::cout << "[AStarPlanner] DEBUG: Step " << step_count 
                      << " processed in " << step_duration.count() << " us. "
                      << "Evaluating Node(" << current_node->y << ", " << current_node->x << ")" << std::endl;
        }
    }
    
    if (debug_mode_) {
        auto fail_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> fail_duration = fail_time - total_start_time;
        std::cout << "[AStarPlanner] DEBUG: FAILED to reach goal coordinates. Total time: " 
                  << fail_duration.count() << " ms." << std::endl;
    }

    return std::vector<GridNode*>(); 
}

std::vector<GridNode*> AStarPlanner::getValidNeighbors(GridNode* current_node, const std::vector<std::vector<int>>& grid, const std::vector<std::vector<bool>>& closed_list, GridNode goal) {
    std::vector<GridNode*> neighbors;
    int ROWS = grid.size();
    int COLS = grid[0].size();
    
    int dx[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dy[8] = {0, 0, -1, 1, -1, 1, -1, 1};

    for (int i = 0; i < 8; i++) {
        int new_x = current_node->x + dx[i];
        int new_y = current_node->y + dy[i];

        if (!isWithinBounds(new_x, new_y, ROWS, COLS)) continue;
        if (isObstacle(grid, new_x, new_y)) continue;
        if (closed_list[new_x][new_y]) continue;
        if (isCornerCutting(grid, current_node->x, current_node->y, dx[i], dy[i])) continue;

        GridNode* neighbor = new GridNode(new_x, new_y);
        neighbor->parent = current_node; 
        
        float move_cost = calculateMoveCost(dx[i], dy[i]);
        neighbor->g_cost = current_node->g_cost + move_cost;
        neighbor->h_cost = calculateHeuristic(new_x, new_y, goal.x, goal.y);
        
        neighbors.push_back(neighbor);
    }
    return neighbors;
}

bool AStarPlanner::isCornerCutting(const std::vector<std::vector<int>>& grid, int current_x, int current_y, int dx, int dy) {
    float move_cost = calculateMoveCost(dx, dy);
    if (move_cost > 1.0f) {
        if (grid[current_x + dx][current_y] == 1 || 
            grid[current_x][current_y + dy] == 1) {
            return true;
        }
    }
    return false;
}

float AStarPlanner::calculateMoveCost(int dx, int dy) {
    return (std::abs(dx) + std::abs(dy) == 2) ? 1.414f : 1.0f;
}

float AStarPlanner::calculateHeuristic(int x1, int y1, int x2, int y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

// ==========================================================
// 6. Path Output Conversion
// ==========================================================
std::vector<Point2D> AStarPlanner::convertGridPathToWorldPath(const std::vector<GridNode*>& grid_path, double origin_x, double origin_y, double resolution) {
    std::vector<Point2D> world_path;
    // Iterate from back to front since computePath returns path from goal to start
    for (int p = grid_path.size() - 1; p >= 0; p--) {
        Point2D pt;
        // Note: in GridNode, x = row = grid_y, y = col = grid_x
        pt.x = gridToWorld(grid_path[p]->y, origin_x, resolution);
        pt.y = gridToWorld(grid_path[p]->x, origin_y, resolution);
        pt.g_cost = grid_path[p]->g_cost;
        pt.h_cost = grid_path[p]->h_cost;
        world_path.push_back(pt);
    }
    return world_path;
}

double AStarPlanner::gridToWorld(int grid_index, double origin, double resolution) {
    return (grid_index * resolution) + origin;
}
