#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "visualization_msgs/msg/marker.hpp"
#include "dynamic_obstacle_avoidance/a_star_planner.hpp"
#include "dynamic_obstacle_avoidance/rrt_planner.hpp"
#include "dynamic_obstacle_avoidance/lqr_controller.hpp"
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <chrono>

using std::placeholders::_1;

class VelocityControllerNode : public rclcpp::Node {
public:
    VelocityControllerNode() : Node("velocity_controller_node"), current_state_(State::IDLE) {
        // --- 1. Parameter Declarations ---
        this->declare_parameter<double>("q_distance", 2.0);
        this->declare_parameter<double>("q_heading", 5.0);
        this->declare_parameter<double>("r_v", 1.0);
        this->declare_parameter<double>("r_omega", 1.0);
        this->declare_parameter<double>("max_linear_speed", 0.22);
        this->declare_parameter<double>("max_angular_speed", 2.84);
        
        // Initialize LQR
        double dt = 0.1; // 100ms timer
        lqr_controller_.init_lqr_parameters(
            this->get_parameter("q_distance").as_double(),
            this->get_parameter("q_heading").as_double(),
            this->get_parameter("r_v").as_double(),
            this->get_parameter("r_omega").as_double(),
            dt
        );
        
        // Offset for where the robot spawns in the map
        this->declare_parameter<double>("map_offset_x", -2.0);
        this->declare_parameter<double>("map_offset_y", -0.5);
        
        // --- 2. Subscribers ---
        // Map from nav2_map_server (Transient Local durability so it receives the map even if it starts late)
        rclcpp::QoS map_qos(10);
        map_qos.transient_local();
        
        map_subscriber_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
            "/map", map_qos, std::bind(&VelocityControllerNode::mapCallback, this, _1));
            
        rclcpp::QoS odom_qos = rclcpp::SensorDataQoS();
        odom_subscriber_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", odom_qos, std::bind(&VelocityControllerNode::odomCallback, this, _1));
            
        goal_subscriber_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "/goal_pose", 10, std::bind(&VelocityControllerNode::goalCallback, this, _1));

        // --- 3. Publishers ---
        path_publisher_ = this->create_publisher<nav_msgs::msg::Path>("/planned_path", 10);
        tree_publisher_ = this->create_publisher<nav_msgs::msg::Path>("/rrt_tree", 10);
        cmd_vel_publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel", 10);

        // --- 4. Control Loop Timer ---
        control_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100), std::bind(&VelocityControllerNode::controlLoop, this));

        RCLCPP_INFO(this->get_logger(), "Unified RRT Navigator Node Started. Waiting for /map and /odom...");
    }

private:
    enum class State {
        IDLE,       // Waiting for a goal
        PLANNING,   // Goal received, running RRT
        FOLLOWING   // Path found, driving the robot
    };

    // --- State Variables ---
    State current_state_;
    nav_msgs::msg::OccupancyGrid::SharedPtr current_map_;
    bool has_map_ = false;
    bool has_odom_ = false;

    // Robot Pose
    double robot_x_ = 0.0;
    double robot_y_ = 0.0;
    double robot_yaw_ = 0.0;

    // Path Following
    std::vector<geometry_msgs::msg::PoseStamped> current_path_;
    size_t current_waypoint_index_ = 0;
    int last_logged_waypoint_ = -1;
    rclcpp::Time path_start_time_;
    double time_per_coord_ = 0.0;
    double total_calc_time_ms_ = 0.0;

    // =========================================================================
    // CALLBACKS
    // =========================================================================

    // 1. Map Callback
    void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
        current_map_ = msg;
        if (!has_map_) {
            has_map_ = true;
            RCLCPP_INFO(this->get_logger(), "Map received and stored!");
        }
    }

    // 2. Odometry Callback
    void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        // Offset between odom origin (0,0) and map origin where the robot actually spawns (-2.0, -0.5)
        double map_offset_x = this->get_parameter("map_offset_x").as_double();
        double map_offset_y = this->get_parameter("map_offset_y").as_double();

        robot_x_ = msg->pose.pose.position.x + map_offset_x;
        robot_y_ = msg->pose.pose.position.y + map_offset_y;

        // Convert Quaternion to Euler Yaw
        tf2::Quaternion q(
            msg->pose.pose.orientation.x,
            msg->pose.pose.orientation.y,
            msg->pose.pose.orientation.z,
            msg->pose.pose.orientation.w);
        tf2::Matrix3x3 m(q);
        double roll, pitch, yaw;
        m.getRPY(roll, pitch, yaw);
        robot_yaw_ = yaw;
        
        if (!has_odom_) {
            has_odom_ = true;
            RCLCPP_INFO(this->get_logger(), "Odometry received. Robot position: [%.2f, %.2f]", robot_x_, robot_y_);
        }
    }

    // 3. Goal Callback
    void goalCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        if (!has_map_) {
            RCLCPP_ERROR(this->get_logger(), "Cannot plan: Map not received yet.");
            return;
        }
        if (!has_odom_) {
            RCLCPP_ERROR(this->get_logger(), "Cannot plan: Odometry not received yet. Check if Gazebo is running.");
            return;
        }

        double goal_x = msg->pose.position.x;
        double goal_y = msg->pose.position.y;

        RCLCPP_INFO(this->get_logger(), "New Goal Received: [%.2f, %.2f]. Transitioning to PLANNING state.", goal_x, goal_y);
        
        // Automatically use current odometry as the starting point!
        planPath(robot_x_, robot_y_, goal_x, goal_y);
    }

    // =========================================================================
    // PLANNING LOGIC
    // =========================================================================
    void planPath(double start_x, double start_y, double goal_x, double goal_y) {
        if (!current_map_) {
            RCLCPP_WARN(this->get_logger(), "No map received yet!");
            return;
        }

        // =========================================================================
        // PLANNER SELECTION
        // To switch planners, comment out one block and uncomment the other.
        // =========================================================================

        // --- Option 1: A* Planner (Currently Active) ---
        RCLCPP_INFO(this->get_logger(), "Finding path with A* Planner...");
        AStarPlanner planner;
        
        auto calc_start = std::chrono::high_resolution_clock::now();
        
        auto path_coords = planner.findPath(
            current_map_->data,
            current_map_->info.width,
            current_map_->info.height,
            current_map_->info.resolution,
            current_map_->info.origin.position.x,
            current_map_->info.origin.position.y,
            start_x, start_y,
            goal_x, goal_y
        );
        
        auto calc_end = std::chrono::high_resolution_clock::now();
        double total_calc_time_ms = std::chrono::duration<double, std::milli>(calc_end - calc_start).count();
        
        std::vector<Point2D> tree_path_coords; // A* doesn't publish a tree

        // --- Option 2: RRT Planner (Commented Out) ---
        /*
        RCLCPP_INFO(this->get_logger(), "Finding path with RRT Planner...");
        RRTPlanner planner;
        // planner.setDebugMode(true);
        auto path_coords = planner.findPath(
            current_map_->data,
            current_map_->info.width,
            current_map_->info.height,
            current_map_->info.resolution,
            current_map_->info.origin.position.x,
            current_map_->info.origin.position.y,
            start_x, start_y,
            goal_x, goal_y
        );
        auto tree_path_coords = planner.getTreeAsPath(); // RRT publishes its tree
        */
        // =========================================================================

        if (!path_coords.empty()) {
            // Prepare Path Message
            nav_msgs::msg::Path path_msg;
            path_msg.header.stamp = this->now();
            path_msg.header.frame_id = current_map_->header.frame_id;
            
            current_path_.clear();

            for (size_t i = 0; i < path_coords.size(); i++) {
                geometry_msgs::msg::PoseStamped pose;
                pose.header = path_msg.header;
                pose.pose.position.x = path_coords[i].x;
                pose.pose.position.y = path_coords[i].y;
                pose.pose.position.z = 0.0;
                pose.pose.orientation.w = 1.0;
                
                path_msg.poses.push_back(pose);
                current_path_.push_back(pose);
            }
            
            // Publish Path to RViz
            path_publisher_->publish(path_msg);
            
            // Publish RRT Tree to RViz (Will just be empty for A*)
            nav_msgs::msg::Path tree_msg;
            tree_msg.header.stamp = this->now();
            tree_msg.header.frame_id = current_map_->header.frame_id;
            for (const auto& pt : tree_path_coords) {
                geometry_msgs::msg::PoseStamped pose;
                pose.header = tree_msg.header;
                pose.pose.position.x = pt.x;
                pose.pose.position.y = pt.y;
                pose.pose.orientation.w = 1.0;
                tree_msg.poses.push_back(pose);
            }
            tree_publisher_->publish(tree_msg);
            
            RCLCPP_INFO(this->get_logger(), "Path successfully planned! Transitioning to FOLLOWING state.");
            
            // --- A* Logging System ---
            total_calc_time_ms_ = total_calc_time_ms;
            time_per_coord_ = total_calc_time_ms / (current_path_.empty() ? 1.0 : current_path_.size());
            
            RCLCPP_INFO(this->get_logger(), "=== A* Planned Path Coordinates ===");
            RCLCPP_INFO(this->get_logger(), " Index |   X    |   Y    | Calc Time (ms) ");
            RCLCPP_INFO(this->get_logger(), "-------|--------|--------|----------------");
            for (size_t i = 0; i < current_path_.size(); ++i) {
                char buf[150];
                snprintf(buf, sizeof(buf), "  %3zu  | %6.2f | %6.2f | %14.4f ", 
                         i, current_path_[i].pose.position.x, current_path_[i].pose.position.y, time_per_coord_);
                RCLCPP_INFO(this->get_logger(), "%s", buf);
            }
            RCLCPP_INFO(this->get_logger(), "-------|--------|--------|----------------");
            RCLCPP_INFO(this->get_logger(), " Total Time to Calculate Path: %.4f ms", total_calc_time_ms_);

            RCLCPP_INFO(this->get_logger(), "=== A* LQR Tracking Log ===");
            RCLCPP_INFO(this->get_logger(), "   X    |   Y    | Dist Err | Head Err | LQR V  | LQR W  ");
            RCLCPP_INFO(this->get_logger(), "--------|--------|----------|----------|--------|--------");

            // --- RRT Logging System (Currently Commented Out) ---
            /*
            RCLCPP_INFO(this->get_logger(), "=== RRT Planned Path Coordinates ===");
            RCLCPP_INFO(this->get_logger(), " Index |   X    |   Y    | Calc Time (ms) ");
            RCLCPP_INFO(this->get_logger(), "-------|--------|--------|----------------");
            for (size_t i = 0; i < current_path_.size(); ++i) {
                char buf[150];
                snprintf(buf, sizeof(buf), "  %3zu  | %6.2f | %6.2f | %14.4f ", 
                         i, current_path_[i].pose.position.x, current_path_[i].pose.position.y, time_per_coord_);
                RCLCPP_INFO(this->get_logger(), "%s", buf);
            }
            RCLCPP_INFO(this->get_logger(), "-------|--------|--------|----------------");
            RCLCPP_INFO(this->get_logger(), " Total Time to Calculate Path: %.4f ms", total_calc_time_ms_);

            RCLCPP_INFO(this->get_logger(), "=== RRT LQR Tracking Log ===");
            RCLCPP_INFO(this->get_logger(), "   X    |   Y    | Dist Err | Head Err | LQR V  | LQR W  ");
            RCLCPP_INFO(this->get_logger(), "--------|--------|----------|----------|--------|--------");
            */
            
            current_waypoint_index_ = 0;
            last_logged_waypoint_ = -1;
            path_start_time_ = this->now();
            current_state_ = State::FOLLOWING;
            
        } else {
            RCLCPP_ERROR(this->get_logger(), "FAILED: No valid path exists to this goal. Returning to IDLE.");
            current_state_ = State::IDLE;
        }
    }

    // =========================================================================
    // CONTROL LOOP LOGIC (Runs at 10 Hz)
    // =========================================================================
    void controlLoop() {
        if (current_state_ != State::FOLLOWING) {
            return; // Do nothing if we aren't following a path
        }

        if (current_waypoint_index_ >= current_path_.size()) {
            // Path complete!
            publishStopCommand();
            double travel_time = (this->now() - path_start_time_).seconds();
            
            RCLCPP_INFO(this->get_logger(), "================================================================================");
            RCLCPP_INFO(this->get_logger(), " Total Time Taken to Reach Goal: %.2f seconds", travel_time);
            RCLCPP_INFO(this->get_logger(), "================================================================================");
            
            RCLCPP_INFO(this->get_logger(), "Goal reached! Robot position is now the new start point. Returning to IDLE.");
            current_state_ = State::IDLE;
            return;
        }

        // Get target waypoint from our path
        double target_x = current_path_[current_waypoint_index_].pose.position.x;
        double target_y = current_path_[current_waypoint_index_].pose.position.y;

        // Calculate distance and heading error
        double distance_to_target = std::hypot(target_x - robot_x_, target_y - robot_y_);

        // Check if waypoint is reached
        // Use a much tighter threshold for the final goal (0.05m) to ensure it reaches the exact coordinate
        double reach_threshold = (current_waypoint_index_ == current_path_.size() - 1) ? 0.05 : 0.15;
        if (distance_to_target < reach_threshold) { 
            current_waypoint_index_++;
            return; 
        }

        // Compute angle to the target point
        double angle_to_target = std::atan2(target_y - robot_y_, target_x - robot_x_);
        double heading_error = angle_to_target - robot_yaw_;
        
        // Normalize heading error to [-pi, pi]
        while (heading_error > M_PI) heading_error -= 2.0 * M_PI;
        while (heading_error < -M_PI) heading_error += 2.0 * M_PI;

        double max_linear_speed = this->get_parameter("max_linear_speed").as_double();
        double max_angular_speed = this->get_parameter("max_angular_speed").as_double();

        geometry_msgs::msg::TwistStamped cmd_vel;
        cmd_vel.header.stamp = this->now();
        cmd_vel.header.frame_id = "base_footprint";
        
        // Use LQR to compute optimal velocities
        lqr_controller_.update_state_error(distance_to_target, heading_error);
        double v_cmd = 0.0;
        double omega_cmd = 0.0;
        lqr_controller_.compute_control_command(v_cmd, omega_cmd);

        // LQR Control Logic with turning priority
        if (std::abs(heading_error) > 0.5) {
            // Prioritize turning if we are not facing the target
            cmd_vel.twist.linear.x = 0.0;
        } else {
            // Move forward and turn smoothly to correct heading
            cmd_vel.twist.linear.x = std::max(0.0, std::min(max_linear_speed, v_cmd));
            cmd_vel.twist.linear.x *= (1.0 - std::abs(heading_error) / M_PI);
        }
        
        cmd_vel.twist.angular.z = omega_cmd;
        
        // Clamp angular velocity to prevent spinning too fast
        if (cmd_vel.twist.angular.z > max_angular_speed) cmd_vel.twist.angular.z = max_angular_speed;
        if (cmd_vel.twist.angular.z < -max_angular_speed) cmd_vel.twist.angular.z = -max_angular_speed;

        // --- A* Tracking Row Logging (Continuous) ---
        char buf[150];
        snprintf(buf, sizeof(buf), " %6.2f | %6.2f | %8.2f | %8.2f | %6.2f | %6.2f", 
                 target_x, target_y, distance_to_target, heading_error, v_cmd, omega_cmd);
        RCLCPP_INFO(this->get_logger(), "%s", buf);

        // --- RRT Tracking Row Logging (Currently Commented Out) ---
        /*
        char buf_rrt[150];
        snprintf(buf_rrt, sizeof(buf_rrt), " %6.2f | %6.2f | %8.2f | %8.2f | %6.2f | %6.2f", 
                 target_x, target_y, distance_to_target, heading_error, v_cmd, omega_cmd);
        RCLCPP_INFO(this->get_logger(), "%s", buf_rrt);
        */

        cmd_vel_publisher_->publish(cmd_vel);
    }

    void publishStopCommand() {
        geometry_msgs::msg::TwistStamped cmd_vel;
        cmd_vel.header.stamp = this->now();
        cmd_vel.header.frame_id = "base_footprint";
        cmd_vel.twist.linear.x = 0.0;
        cmd_vel.twist.angular.z = 0.0;
        cmd_vel_publisher_->publish(cmd_vel);
    }

    // ROS Objects
    rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_subscriber_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_subscriber_;
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_subscriber_;
    
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_publisher_;
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr tree_publisher_;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_publisher_;
    
    rclcpp::TimerBase::SharedPtr control_timer_;
    LQRController lqr_controller_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<VelocityControllerNode>());
    rclcpp::shutdown();
    return 0;
}
