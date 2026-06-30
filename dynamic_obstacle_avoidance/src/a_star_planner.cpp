#include "dynamic_obstacle_avoidance/a_star_planner.hpp"
#include <cmath>
#include <queue>

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
            neighbor->g_cost = current_node->g_cost + move_cost;
            neighbor->h_cost = calculateHeuristic(new_x, new_y, goal.x, goal.y);

            open_list.push(neighbor);
        }
    }
    
    return std::vector<GridNode*>(); 
}