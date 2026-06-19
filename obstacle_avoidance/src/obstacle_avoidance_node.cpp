#include <memory>
#include <cmath>
#include <algorithm> // Required for std::clamp
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

using std::placeholders::_1;

class ObstacleAvoidanceNode : public rclcpp::Node {
public:
    ObstacleAvoidanceNode() : Node("obstacle_avoidance_node") {
        // --- DECLARE ROS 2 PARAMETERS ---
        this->declare_parameter("goal_x", 2.0);
        this->declare_parameter("goal_y", 0.5);

        // Fetch the parameters
        goal_x_ = this->get_parameter("goal_x").as_double();
        goal_y_ = this->get_parameter("goal_y").as_double();

        publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel", 10);
        
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", 10, std::bind(&ObstacleAvoidanceNode::odom_callback, this, _1));

        scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            "/scan", 10, std::bind(&ObstacleAvoidanceNode::scan_callback, this, _1));
        
        RCLCPP_INFO(this->get_logger(), "Advanced FSM Autonomy Engaged. Target Acquired: X=%.2f, Y=%.2f", goal_x_, goal_y_);
    }

private:
    double current_x_ = 0.0;
    double current_y_ = 0.0;
    double current_yaw_ = 0.0;
    bool odom_received_ = false; 
    
    double goal_x_ = 0.0; 
    double goal_y_ = 0.0;

    // --- UPGRADED 5-STATE FSM ---
    enum class Mode { TRACKING, DODGING, RECOVERING, ARRIVED, BLOCKED };
    Mode current_mode_ = Mode::TRACKING;
    
    bool is_turning_left_ = false;
    int recovery_ticks_ = 0;

    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        current_x_ = msg->pose.pose.position.x;
        current_y_ = msg->pose.pose.position.y;

        double qx = msg->pose.pose.orientation.x;
        double qy = msg->pose.pose.orientation.y;
        double qz = msg->pose.pose.orientation.z;
        double qw = msg->pose.pose.orientation.w;
        current_yaw_ = std::atan2(2.0 * (qw * qz + qx * qy), 1.0 - 2.0 * (qy * qy + qz * qz));

        odom_received_ = true; 
    }

    void scan_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
        if (!odom_received_) return; 
        
        auto twist_msg = geometry_msgs::msg::TwistStamped();
        twist_msg.header.stamp = this->get_clock()->now();
        twist_msg.header.frame_id = "base_link";

        float min_front = 10.0;
        float sum_left = 0.0;
        int count_left = 0;
        float sum_right = 0.0;
        int count_right = 0;

        for (size_t i = 0; i < msg->ranges.size(); i++) {
            float d = msg->ranges[i];
            if (std::isinf(d) || std::isnan(d) || d == 0.0) d = 10.0; 

            // Slightly wider front cone to prevent clipping during recovery
            if (i <= 25 || i >= 335) {
                if (d < min_front) min_front = d;
            } else if (i > 25 && i <= 90) {
                sum_left += d; count_left++;
            } else if (i >= 270 && i < 335) {
                sum_right += d; count_right++;
            }
        }

        float avg_left = (count_left > 0) ? (sum_left / count_left) : 0.0;
        float avg_right = (count_right > 0) ? (sum_right / count_right) : 0.0;

        float danger_distance = 0.50; 
        float clear_distance = 0.65;  

        // --- STATE TRANSITIONS ---
        double distance_to_goal = std::hypot(goal_x_ - current_x_, goal_y_ - current_y_);

        // 1. TERMINAL STATES: Locked in
        if (current_mode_ == Mode::ARRIVED || current_mode_ == Mode::BLOCKED) {
            // Do nothing, maintain locked state
        }
        // 2. SUCCESS CHECK
        else if (distance_to_goal < 0.2) {
            current_mode_ = Mode::ARRIVED;
            RCLCPP_INFO_ONCE(this->get_logger(), "MISSION ACCOMPLISHED: Arrived at Target.");
        }
        // 3. ABORT CHECK: Goal is inside an obstacle
        else if (distance_to_goal < 0.65 && min_front < danger_distance) {
            current_mode_ = Mode::BLOCKED;
            RCLCPP_ERROR_ONCE(this->get_logger(), "MISSION ABORTED: Target coordinate is inside an obstacle!");
        }
        // 4. NORMAL TRACKING
        else if (current_mode_ == Mode::TRACKING) {
            if (min_front < danger_distance) {
                current_mode_ = Mode::DODGING;
                is_turning_left_ = (avg_left > avg_right);
                RCLCPP_WARN(this->get_logger(), "Obstacle detected. Initiating dodge maneuver.");
            }
        } 
        // 5. DODGING
        else if (current_mode_ == Mode::DODGING) {
            if (min_front > clear_distance) {
                current_mode_ = Mode::RECOVERING;
                recovery_ticks_ = 15; // Drive straight for 1.5s
                RCLCPP_INFO(this->get_logger(), "Gap found. Pushing through to clear obstacle...");
            }
        } 
        // 6. RECOVERING
        else if (current_mode_ == Mode::RECOVERING) {
            recovery_ticks_--;
            if (min_front < danger_distance) {
                current_mode_ = Mode::DODGING; // Hit another wall, go back to dodging
                is_turning_left_ = (avg_left > avg_right);
            } else if (recovery_ticks_ <= 0) {
                current_mode_ = Mode::TRACKING; // Cleared! Look for goal again
                RCLCPP_INFO(this->get_logger(), "Obstacle cleared. Re-acquiring target.");
            }
        }

        // --- MOVEMENT EXECUTION ---
        if (current_mode_ == Mode::ARRIVED || current_mode_ == Mode::BLOCKED) {
            twist_msg.twist.linear.x = 0.0;
            twist_msg.twist.angular.z = 0.0;
        } 
        else if (current_mode_ == Mode::DODGING) {
            twist_msg.twist.linear.x = 0.0; 
            twist_msg.twist.angular.z = is_turning_left_ ? 0.6 : -0.6; 
        } 
        else if (current_mode_ == Mode::RECOVERING) {
            twist_msg.twist.linear.x = 0.25; 
            twist_msg.twist.angular.z = 0.0; 
        }
        else if (current_mode_ == Mode::TRACKING) {
            double angle_to_goal = std::atan2(goal_y_ - current_y_, goal_x_ - current_x_);
            double angle_error = angle_to_goal - current_yaw_;
            
            while (angle_error > M_PI) angle_error -= 2.0 * M_PI;
            while (angle_error < -M_PI) angle_error += 2.0 * M_PI;

            twist_msg.twist.linear.x = 0.2; 
            twist_msg.twist.angular.z = std::clamp(0.6 * angle_error, -0.5, 0.5); 
        }

        publisher_->publish(twist_msg);
    }

    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr publisher_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ObstacleAvoidanceNode>());
    rclcpp::shutdown();
    return 0;
}