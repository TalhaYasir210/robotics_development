#ifndef A_STAR_PLANNER_HPP
#define A_STAR_PLANNER_HPP

#include <vector>
#include <cstdint>

struct Point2D {
    double x;
    double y;
    float g_cost;
    float h_cost;
};

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
    std::vector<GridNode*> computePath(std::vector<std::vector<int>>& grid, GridNode start, GridNode goal);
    bool debug_mode_ = false;

public:
    void setDebugMode(bool debug) { debug_mode_ = debug; }

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
        double goal_y
    );
};

#endif // A_STAR_PLANNER_HPP