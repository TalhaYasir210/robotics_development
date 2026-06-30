#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "dynamic_obstacle_avoidance/a_star_planner.hpp"
#include <vector>
#include <string>
#include <cmath>

using std::placeholders::_1;

class PathPlannerNode : public rclcpp::Node {
public:
    PathPlannerNode() : Node("path_planner_node") {
        // Subscribers
        map_subscriber_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
            "/map", 10, std::bind(&PathPlannerNode::mapCallback, this, _1));
            
        goal_subscriber_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "/goal_pose", 10, std::bind(&PathPlannerNode::goalCallback, this, _1));

        start_subscriber_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "/initialpose", 10, std::bind(&PathPlannerNode::startCallback, this, _1));
            
        // Publisher
        path_publisher_ = this->create_publisher<nav_msgs::msg::Path>("/planned_path", 10);
            
        RCLCPP_INFO(this->get_logger(), "Planner Node Started. Waiting for /map and /goal_pose...");
    }

private:
    void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
        current_map_ = msg;
        has_map_ = true;
        RCLCPP_INFO(this->get_logger(), "Map received and stored.");
        planPath();
    }

    void startCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg) {
        start_x_ = msg->pose.pose.position.x;
        start_y_ = msg->pose.pose.position.y;
        has_start_ = true;
        RCLCPP_INFO(this->get_logger(), "Manual start pose updated to: [%.2f, %.2f]", start_x_, start_y_);
        planPath();
    }

    void goalCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        goal_x_ = msg->pose.position.x;
        goal_y_ = msg->pose.position.y;
        has_goal_ = true;
        RCLCPP_INFO(this->get_logger(), "Goal pose updated to: [%.2f, %.2f]", goal_x_, goal_y_);
        planPath();
    }

    void planPath() {
        if (!has_map_ || !has_goal_ || !has_start_) {
            // Silently wait until we have a map, start, and goal
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Planning Path from Initial Location: [%.2f, %.2f] to Goal Location: [%.2f, %.2f]", start_x_, start_y_, goal_x_, goal_y_);

        AStarPlanner planner;
        auto path_coords = planner.findPath(
            current_map_->data,
            current_map_->info.width,
            current_map_->info.height,
            current_map_->info.resolution,
            current_map_->info.origin.position.x,
            current_map_->info.origin.position.y,
            start_x_, start_y_,
            goal_x_, goal_y_
        );

        if (!path_coords.empty()) {
            nav_msgs::msg::Path path_msg;
            path_msg.header.stamp = this->now();
            path_msg.header.frame_id = current_map_->header.frame_id;
            
            std::string path_str = "Planned Path Coordinates:\n";

            for (size_t i = 0; i < path_coords.size(); i++) {
                path_str += "[" + std::to_string(path_coords[i].x) + ", " + std::to_string(path_coords[i].y) + 
                            ", g: " + std::to_string(path_coords[i].g_cost) + ", h: " + std::to_string(path_coords[i].h_cost) + "]";
                if (i != path_coords.size() - 1) path_str += "\n -> ";
                
                geometry_msgs::msg::PoseStamped pose;
                pose.header = path_msg.header;
                
                pose.pose.position.x = path_coords[i].x;
                pose.pose.position.y = path_coords[i].y;
                pose.pose.position.z = 0.0;
                pose.pose.orientation.w = 1.0;
                
                path_msg.poses.push_back(pose);
            }
            
            path_publisher_->publish(path_msg);
            RCLCPP_INFO(this->get_logger(), "Path successfully planned and published!");
            RCLCPP_INFO(this->get_logger(), "%s\n", path_str.c_str());
            
            // --- SIMULATED MOVEMENT ---
            // Assume the robot drives to the goal perfectly.
            // We set the start position to the current goal, so the NEXT path starts from here.
            start_x_ = goal_x_;
            start_y_ = goal_y_;
            RCLCPP_INFO(this->get_logger(), "Simulated Arrival: Robot is now at [%.2f, %.2f]. Ready for next goal!", start_x_, start_y_);
            
        } else {
            RCLCPP_ERROR(this->get_logger(), "FAILED: No valid path exists.");
        }
    }

    // Node state
    nav_msgs::msg::OccupancyGrid::SharedPtr current_map_;
    bool has_map_ = false;
    
    // Default start position to (0,0) in world coordinates, but allow overriding
    bool has_start_ = true; 
    double start_x_ = 0.0;
    double start_y_ = 0.0;
    
    bool has_goal_ = false;
    double goal_x_ = 0.0;
    double goal_y_ = 0.0;

    // ROS objects
    rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_subscriber_;
    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr start_subscriber_;
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_subscriber_;
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_publisher_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PathPlannerNode>());
    rclcpp::shutdown();
    return 0;
}