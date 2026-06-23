#include <cmath>
#include <algorithm>
#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

using std::placeholders::_1;

class AutonomousSurveyorNode : public rclcpp::Node {
public:
    AutonomousSurveyorNode() : Node("autonomous_surveyor_node") {
        publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel", 10);
        
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", 10, std::bind(&AutonomousSurveyorNode::odom_callback, this, _1));

        scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            "/scan", 10, std::bind(&AutonomousSurveyorNode::scan_callback, this, _1));
        
        RCLCPP_INFO(this->get_logger(), "Autonomous Surveyor Active! Seeking nearest wall...");
    }

private:
    bool first_reading_ = true;
    double min_x_ = 0.0, max_x_ = 0.0;
    double min_y_ = 0.0, max_y_ = 0.0;
    
    // FSM States for Wall Following
    enum class State { SEEK_WALL, ALIGN_LEFT, HUG_WALL };
    State current_state_ = State::SEEK_WALL;

    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        double current_x = msg->pose.pose.position.x;
        double current_y = msg->pose.pose.position.y;

        if (first_reading_) {
            min_x_ = max_x_ = current_x;
            min_y_ = max_y_ = current_y;
            first_reading_ = false;
            return;
        }

        // Continuously push the boundary limits outward
        if (current_x < min_x_) min_x_ = current_x;
        if (current_x > max_x_) max_x_ = current_x;
        if (current_y < min_y_) min_y_ = current_y;
        if (current_y > max_y_) max_y_ = current_y;

        // Print the boundaries to the terminal every ~1.5 seconds
        static int tick = 0;
        if (tick++ % 15 == 0) {
            RCLCPP_INFO(this->get_logger(), 
                "MAP LIMITS -> X: [%.2f to %.2f] | Y: [%.2f to %.2f]", 
                min_x_, max_x_, min_y_, max_y_);
        }
    }

    void scan_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
        auto twist_msg = geometry_msgs::msg::TwistStamped();
        twist_msg.header.stamp = this->get_clock()->now();
        twist_msg.header.frame_id = "base_link";

        float min_front = 10.0;
        float min_right = 10.0;

        for (size_t i = 0; i < msg->ranges.size(); i++) {
            float d = msg->ranges[i];
            if (std::isinf(d) || std::isnan(d) || d == 0.0) d = 10.0; 

            // Front Cone
            if (i <= 20 || i >= 340) {
                if (d < min_front) min_front = d;
            }
            // Right Side Cone
            else if (i >= 260 && i <= 280) {
                if (d < min_right) min_right = d;
            }
        }

        // --- WALL FOLLOWER LOGIC ---
        if (current_state_ == State::SEEK_WALL) {
            twist_msg.twist.linear.x = 0.2; 
            twist_msg.twist.angular.z = 0.0;
            
            if (min_front < 0.50) {
                current_state_ = State::ALIGN_LEFT;
            }
        } 
        else if (current_state_ == State::ALIGN_LEFT) {
            twist_msg.twist.linear.x = 0.0; 
            twist_msg.twist.angular.z = 0.5; // Spin left
            
            if (min_front > 0.60) {
                current_state_ = State::HUG_WALL;
            }
        } 
        else if (current_state_ == State::HUG_WALL) {
            if (min_front < 0.50) {
                // Hit a corner, spin left again
                current_state_ = State::ALIGN_LEFT;
            } else {
                twist_msg.twist.linear.x = 0.2;
                
                // Keep the wall exactly 0.4 meters to our right
                float error = 0.40 - min_right; 
                float steering_correction = error * 2.5; 
                twist_msg.twist.angular.z = std::clamp(steering_correction, -0.4f, 0.4f);
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
    rclcpp::spin(std::make_shared<AutonomousSurveyorNode>());
    rclcpp::shutdown();
    return 0;
}