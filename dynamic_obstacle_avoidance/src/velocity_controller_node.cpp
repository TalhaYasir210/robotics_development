#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "visualization_msgs/msg/marker.hpp"
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
        //plan_subscriber_ = this->create_subscription<nav_msgs::msg::Path>(
          //  "/plan", 10, std::bind(&VelocityControllerNode::planCallback, this, _1));
            
        rclcpp::QoS odom_qos = rclcpp::SensorDataQoS();
        odom_subscriber_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", odom_qos, std::bind(&VelocityControllerNode::odomCallback, this, _1));

        // --- 3. Publishers ---
        cmd_vel_publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel", 10);

        // --- 4. Control Loop Timer ---
        control_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100), std::bind(&VelocityControllerNode::controlLoop, this));

        RCLCPP_INFO(this->get_logger(), "LQR Velocity Controller Started. Waiting for /plan and /odom...");
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

    // 1. Plan Callback from Nav2
    void planCallback(const nav_msgs::msg::Path::SharedPtr msg) {
        if (!has_odom_) {
            RCLCPP_ERROR(this->get_logger(), "[VelocityControllerNode::planCallback] ERROR: Received path from Nav2, but Odometry is unavailable. Cannot start following.");
            return;
        }

        if (msg->poses.empty()) {
            RCLCPP_ERROR(this->get_logger(), "[VelocityControllerNode::planCallback] ERROR: Received empty path from Nav2. Planning failed or invalid goal.");
            return;
        }

        RCLCPP_INFO(this->get_logger(), "New Path Received from Nav2! Transitioning to FOLLOWING state.");
        
        current_path_ = msg->poses;
        current_waypoint_index_ = 0;
        path_start_time_ = this->now();
        current_state_ = State::FOLLOWING;
        
        RCLCPP_INFO(this->get_logger(), "=== LQR Tracking Log ===");
        RCLCPP_INFO(this->get_logger(), "   X    |   Y    | Dist Err | Head Err | LQR V  | LQR W  ");
        RCLCPP_INFO(this->get_logger(), "--------|--------|----------|----------|--------|--------");
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

    // No internal path planning logic required anymore; Nav2 handles this.

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

        // --- Tracking Row Logging (Continuous) ---
        char buf[150];
        snprintf(buf, sizeof(buf), " %6.2f | %6.2f | %8.2f | %8.2f | %6.2f | %6.2f", 
                 target_x, target_y, distance_to_target, heading_error, v_cmd, omega_cmd);
        RCLCPP_INFO(this->get_logger(), "%s", buf);

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
    rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr plan_subscriber_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_subscriber_;
    
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
