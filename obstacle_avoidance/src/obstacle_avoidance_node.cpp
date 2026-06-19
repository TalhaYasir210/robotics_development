#include <memory>
#include <cmath>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

using std::placeholders::_1;

class ObstacleAvoidanceNode : public rclcpp::Node {
public:
    ObstacleAvoidanceNode() : Node("obstacle_avoidance_node") {
        publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel", 10);
        
        // Subscriber 1: Odometry (Where am I?)
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", 10, std::bind(&ObstacleAvoidanceNode::odom_callback, this, _1));

        // Subscriber 2: Lidar (Am I going to crash?)
        scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            "/scan", 10, std::bind(&ObstacleAvoidanceNode::scan_callback, this, _1));
        
        RCLCPP_INFO(this->get_logger(), "Go-To-Goal + Avoidance Engaged. Target: Center (0,0)");
    }

private:
    // --- ROBOT STATE ---
    double current_x_ = 0.0;
    double current_y_ = 0.0;
    double current_yaw_ = 0.0;
    bool odom_received_ = false;
    // --- GOAL ---
    double goal_x_ = 2.0; // The dead center of the hexagonal maze
    double goal_y_ = 0.5;

    // --- FSM STATE ---
    bool avoiding_obstacle_ = false;
    bool is_turning_left_ = false;

    // Odometry Callback: Updates the robot's coordinates 10 times a second
    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        current_x_ = msg->pose.pose.position.x;
        current_y_ = msg->pose.pose.position.y;

        // Convert Quaternion to Euler Yaw (Heading)
        double qx = msg->pose.pose.orientation.x;
        double qy = msg->pose.pose.orientation.y;
        double qz = msg->pose.pose.orientation.z;
        double qw = msg->pose.pose.orientation.w;
        current_yaw_ = std::atan2(2.0 * (qw * qz + qx * qy), 1.0 - 2.0 * (qy * qy + qz * qz));
        odom_received_ = true;
    }


    // Scan Callback: The main brain combining Target Tracking and Survival
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

            if (i <= 20 || i >= 340) {
                if (d < min_front) min_front = d;
            } else if (i > 20 && i <= 90) {
                sum_left += d; count_left++;
            } else if (i >= 270 && i < 340) {
                sum_right += d; count_right++;
            }
        }

        float avg_left = (count_left > 0) ? (sum_left / count_left) : 0.0;
        float avg_right = (count_right > 0) ? (sum_right / count_right) : 0.0;

        float danger_distance = 0.50; 
        float clear_distance = 0.65;  

        // Check if we need to switch into survival mode
        if (!avoiding_obstacle_ && min_front < danger_distance) {
            avoiding_obstacle_ = true;
            is_turning_left_ = (avg_left > avg_right);
            RCLCPP_WARN(this->get_logger(), "Obstacle! Abandoning goal temporarily to dodge.");
        } 
        // Check if we are safe to resume tracking the goal
        else if (avoiding_obstacle_ && min_front > clear_distance) {
            avoiding_obstacle_ = false;
            RCLCPP_INFO(this->get_logger(), "Path clear. Re-acquiring target (0,0).");
        }

        // --- EXECUTE MOVEMENT ---
        if (avoiding_obstacle_) {
            // SURVIVAL MODE: Spin to escape the obstacle
            twist_msg.twist.linear.x = 0.0; 
            twist_msg.twist.angular.z = is_turning_left_ ? 0.5 : -0.5; 
        } 
        else {
            // TARGET TRACKING MODE: Calculate math to drive to X:0, Y:0
            double distance_to_goal = std::hypot(goal_x_ - current_x_, goal_y_ - current_y_);
            
            // If we are within 0.2 meters of the center, stop completely. We won!
            if (distance_to_goal < 0.2) {
                twist_msg.twist.linear.x = 0.0;
                twist_msg.twist.angular.z = 0.0;
                RCLCPP_INFO_ONCE(this->get_logger(), "ARRIVED AT GOAL!");
            } else {
                // Calculate the angle pointing to the center
                double angle_to_goal = std::atan2(goal_y_ - current_y_, goal_x_ - current_x_);
                
                // Calculate how much we need to turn to face the center
                double angle_error = angle_to_goal - current_yaw_;
                
                // Normalize the angle error to be between -Pi and Pi
                while (angle_error > M_PI) angle_error -= 2.0 * M_PI;
                while (angle_error < -M_PI) angle_error += 2.0 * M_PI;

                // Steer towards the goal while moving forward
                twist_msg.twist.linear.x = 0.2; 
                twist_msg.twist.angular.z = 0.8 * angle_error; // Proportional turning
            }
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