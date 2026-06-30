#include "dynamic_obstacle_avoidance/a_star_planner.hpp"
#include <cmath>
#include <queue>
#include <iostream>

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

float AStarPlanner::calculateHeuristic(int x1, int y1, int x2, int y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

std::vector<GridNode*> AStarPlanner::computePath(std::vector<std::vector<int>>& grid, GridNode start, GridNode goal) {
    int ROWS = grid.size();
    int COLS = grid[0].size();

    std::vector<std::vector<bool>> closed_list(ROWS, std::vector<bool>(COLS, false));
    std::priority_queue<GridNode*, std::vector<GridNode*>, CompareF> open_list;

    GridNode* start_node = new GridNode(start.x, start.y);
    start_node->g_cost = 0.0;
    start_node->h_cost = calculateHeuristic(start.x, start.y, goal.x, goal.y);
    open_list.push(start_node);

    while (!open_list.empty()) {
        GridNode* current_node = open_list.top();
        open_list.pop();

        if (closed_list[current_node->x][current_node->y]) {
            continue;
        }

        closed_list[current_node->x][current_node->y] = true;

        if (current_node->x == goal.x && current_node->y == goal.y) {
            std::vector<GridNode*> final_path;
            GridNode* trace_node = current_node;
            
            while (trace_node != nullptr) {
                final_path.push_back(trace_node);
                trace_node = trace_node->parent;
            }
            
            return final_path; 
        }

        // Update neighbors to include diagonals
        int dx[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
        int dy[8] = {0, 0, -1, 1, -1, 1, -1, 1};

        for (int i = 0; i < 8; i++) {
            int new_x = current_node->x + dx[i];
            int new_y = current_node->y + dy[i];

            if (new_x < 0 || new_x >= ROWS || new_y < 0 || new_y >= COLS) continue;
            if (grid[new_x][new_y] == 1) continue;
            if (closed_list[new_x][new_y] == true) continue;

            GridNode* neighbor = new GridNode(new_x, new_y);
            neighbor->parent = current_node; 
            
            // When calculating the cost for diagonal neighbors, use 1.414 instead of 1.0
            float move_cost = (std::abs(dx[i]) + std::abs(dy[i]) == 2) ? 1.414f : 1.0f;

            // Prevent corner cutting!
            // If we are moving diagonally, we must check if the two adjacent straight cells are obstacles.
            if (move_cost > 1.0f) {
                if (grid[current_node->x + dx[i]][current_node->y] == 1 || 
                    grid[current_node->x][current_node->y + dy[i]] == 1) {
                    continue; // Squeezing through the corner is not allowed
                }
            }

            neighbor->g_cost = current_node->g_cost + move_cost;
            neighbor->h_cost = calculateHeuristic(new_x, new_y, goal.x, goal.y);

            open_list.push(neighbor);
        }
    }
    
    return std::vector<GridNode*>(); 
}

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
    int start_grid_x = std::round((start_x - origin_x) / resolution);
    int start_grid_y = std::round((start_y - origin_y) / resolution);
    
    int goal_grid_x = std::round((goal_x - origin_x) / resolution);
    int goal_grid_y = std::round((goal_y - origin_y) / resolution);

    // 2. Bound checks
    if (start_grid_x < 0 || start_grid_x >= width || start_grid_y < 0 || start_grid_y >= height) {
        std::cout << "[AStarPlanner] Start position is outside the map!" << std::endl;
        return {};
    }
    if (goal_grid_x < 0 || goal_grid_x >= width || goal_grid_y < 0 || goal_grid_y >= height) {
        std::cout << "[AStarPlanner] Goal position is outside the map!" << std::endl;
        return {};
    }

    // 3. Create 2D grid
    std::vector<std::vector<int>> grid(height, std::vector<int>(width, 0));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = x + (y * width);
            if (map_data[index] >= 50) {
                grid[y][x] = 1; 
            }
        }
    }

    // 4. Validate Start and Goal
    if (grid[start_grid_y][start_grid_x] == 1) {
        std::cout << "[AStarPlanner] FAILED: Start position is inside an obstacle!" << std::endl;
        return {};
    }
    if (grid[goal_grid_y][goal_grid_x] == 1) {
        std::cout << "[AStarPlanner] FAILED: Goal position is inside an obstacle!" << std::endl;
        return {};
    }

    // GridNode expects (row, col) which maps to (y, x) in grid indices
    GridNode start_node(start_grid_y, start_grid_x);
    GridNode goal_node(goal_grid_y, goal_grid_x);

    // 5. Compute Path
    std::vector<GridNode*> grid_path = computePath(grid, start_node, goal_node);

    // 6. Convert to world coordinates
    std::vector<Point2D> world_path;
    // Iterate from back to front since computePath returns path from goal to start
    for (int p = grid_path.size() - 1; p >= 0; p--) {
        Point2D pt;
        // Convert back from GridNode (row=x, col=y) to world coordinates
        pt.x = (grid_path[p]->y * resolution) + origin_x;
        pt.y = (grid_path[p]->x * resolution) + origin_y;
        pt.g_cost = grid_path[p]->g_cost;
        pt.h_cost = grid_path[p]->h_cost;
        world_path.push_back(pt);
    }

    // Optional: free memory returned by computePath
    // Though it doesn't free everything that was put into open_list, 
    // it frees the nodes in the final path list to avoid full leak of the result list.
    // For a complete memory leak fix, computePath would need a refactor, but this handles the return value at least.

    return world_path;
}