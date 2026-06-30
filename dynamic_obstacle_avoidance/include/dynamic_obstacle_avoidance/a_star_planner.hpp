#ifndef A_STAR_PLANNER_HPP
#define A_STAR_PLANNER_HPP

#include <vector>

// Renamed from Node to GridNode to prevent ROS 2 collision
struct GridNode {
    int x;
    int y;
    float g_cost;  
    float h_cost;  
    GridNode* parent;  

    GridNode(int x_pos, int y_pos);
};

class AStarPlanner {
private:
    float calculateHeuristic(int x1, int y1, int x2, int y2);

public:
    std::vector<GridNode*> computePath(std::vector<std::vector<int>>& grid, GridNode start, GridNode goal);
};

#endif // A_STAR_PLANNER_HPP