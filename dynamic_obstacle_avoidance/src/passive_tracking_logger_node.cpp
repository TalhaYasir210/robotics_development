#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "visualization_msgs/msg/marker.hpp"
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <chrono>

using std::placeholders::_1;

class PassiveTrackingLoggerNode : public rclcpp::Node {
public:
    PassiveTrackingLoggerNode() : Node("passive_tracking_logger_node"), current_state_(State::IDLE) {
        // --- 1. Parameter Declarations ---
        // Removed LQR Parameters
        
        // Offset for where the robot spawns in the map
        this->declare_parameter<double>("map_offset_x", -2.0);
        this->declare_parameter<double>("map_offset_y", -0.5);
        
        // --- 2. Subscribers ---
        plan_subscriber_ = this->create_subscription<nav_msgs::msg::Path>(
            "/plan", 10, std::bind(&PassiveTrackingLoggerNode::planCallback, this, _1));
            
        rclcpp::QoS odom_qos = rclcpp::SensorDataQoS();
        odom_subscriber_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", odom_qos, std::bind(&PassiveTrackingLoggerNode::odomCallback, this, _1));

        cmd_vel_subscriber_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
            "/cmd_vel", 10, std::bind(&PassiveTrackingLoggerNode::cmdVelCallback, this, _1));

        // --- 3. Publishers (REMOVED - This is a passive logger) ---

        // --- 4. Control Loop Timer ---
        control_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100), std::bind(&PassiveTrackingLoggerNode::controlLoop, this));

        RCLCPP_INFO(this->get_logger(), "Passive Tracking Logger Started. Waiting for /plan and /odom...");
    }

private:
    enum class State {
        IDLE,       // Waiting for a goal
        FOLLOWING   // Path found, tracking the robot
    };

    // --- State Variables ---
    State current_state_;
    bool has_odom_ = false;

    // Robot Pose
    double robot_x_ = 0.0;
    double robot_y_ = 0.0;
    double robot_yaw_ = 0.0;
    
    // DWB tracking
    double dwb_v_ = 0.0;
    double dwb_w_ = 0.0;

    // Path Following
    std::vector<geometry_msgs::msg::PoseStamped> current_path_;
    size_t current_waypoint_index_ = 0;
    rclcpp::Time path_start_time_;

    // =========================================================================
    // CALLBACKS
    // =========================================================================

    void planCallback(const nav_msgs::msg::Path::SharedPtr msg) {
        if (!has_odom_) {
            RCLCPP_WARN(this->get_logger(), "Received path from Nav2, but no Odometry yet. Cannot follow.");
            return;
        }

        if (msg->poses.empty()) {
            RCLCPP_WARN(this->get_logger(), "Received empty path from Nav2.");
            return;
        }

        RCLCPP_INFO(this->get_logger(), "New Path Received from Nav2! Transitioning to FOLLOWING state.");
        
        current_path_ = msg->poses;
        current_waypoint_index_ = 0;
        path_start_time_ = this->now();
        current_state_ = State::FOLLOWING;
        
        RCLCPP_INFO(this->get_logger(), "=== Tracker Logging ===");
        RCLCPP_INFO(this->get_logger(), "   X    |   Y    | Dist Err | Head Err | DWB V  | DWB W  ");
        RCLCPP_INFO(this->get_logger(), "--------|--------|----------|----------|--------|--------");
    }

    void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        double map_offset_x = this->get_parameter("map_offset_x").as_double();
        double map_offset_y = this->get_parameter("map_offset_y").as_double();

        robot_x_ = msg->pose.pose.position.x + map_offset_x;
        robot_y_ = msg->pose.pose.position.y + map_offset_y;

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
    
    void cmdVelCallback(const geometry_msgs::msg::TwistStamped::SharedPtr msg) {
        dwb_v_ = msg->twist.linear.x;
        dwb_w_ = msg->twist.angular.z;
    }

    // =========================================================================
    // CONTROL LOOP LOGIC (Runs at 10 Hz)
    // =========================================================================
    void controlLoop() {
        if (current_state_ != State::FOLLOWING) {
            return; // Do nothing if we aren't following a path
        }

        if (current_waypoint_index_ >= current_path_.size()) {
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

        double distance_to_target = std::hypot(target_x - robot_x_, target_y - robot_y_);

        double reach_threshold = (current_waypoint_index_ == current_path_.size() - 1) ? 0.05 : 0.15;
        if (distance_to_target < reach_threshold) { 
            current_waypoint_index_++;
            return; 
        }

        double angle_to_target = std::atan2(target_y - robot_y_, target_x - robot_x_);
        double heading_error = angle_to_target - robot_yaw_;
        
        while (heading_error > M_PI) heading_error -= 2.0 * M_PI;
        while (heading_error < -M_PI) heading_error += 2.0 * M_PI;

        // --- Tracking Row Logging (Continuous) ---
        char buf[150];
        snprintf(buf, sizeof(buf), " %6.2f | %6.2f | %8.2f | %8.2f | %6.2f | %6.2f", 
                 target_x, target_y, distance_to_target, heading_error, dwb_v_, dwb_w_);
        RCLCPP_INFO(this->get_logger(), "%s", buf);
    }

    rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr plan_subscriber_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_subscriber_;
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_subscriber_;
    
    rclcpp::TimerBase::SharedPtr control_timer_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PassiveTrackingLoggerNode>());
    rclcpp::shutdown();
    return 0;
}
