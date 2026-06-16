#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "turtlesim/msg/pose.hpp"
#include <cmath>

class ControllerNode : public rclcpp::Node
{
public:
    ControllerNode() : Node("controller_node")
    {
        this->declare_parameter("target_x", 5.0);
        this->declare_parameter("target_y", 5.0);
        
        publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);
        subscription_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose", 10, std::bind(&ControllerNode::pose_callback, this, std::placeholders::_1));
        timer_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&ControllerNode::control_loop, this));
    }

private:
    void pose_callback(const turtlesim::msg::Pose::SharedPtr msg) { current_pose_ = *msg; }

    void control_loop()
    {
        double goal_x = this->get_parameter("target_x").as_double();
        double goal_y = this->get_parameter("target_y").as_double();

        // 1. Automatic Shutdown on Error
        if (goal_x < 0 || goal_y < 0) {
            RCLCPP_ERROR(this->get_logger(), "CRITICAL: Negative coordinates! Shutting down.");
            rclcpp::shutdown(); 
            return; 
        }

        // 2. Math Logic
        double dx = goal_x - current_pose_.x;
        double dy = goal_y - current_pose_.y;
        double distance = std::sqrt(dx * dx + dy * dy);
        double angle_error = std::atan2(dy, dx) - current_pose_.theta;

        // Normalize angle
        while (angle_error > M_PI) angle_error -= 2 * M_PI;
        while (angle_error < -M_PI) angle_error += 2 * M_PI;

        auto msg = geometry_msgs::msg::Twist();

        // 3. Controller & Automatic Shutdown on Goal
        if (distance > 0.1) {
            msg.linear.x = 0.5 * distance;
            msg.angular.z = 4.0 * angle_error;
        } else {
            msg.linear.x = 0.0;
            msg.angular.z = 0.0;
            publisher_->publish(msg); // Final stop command
            
            RCLCPP_INFO(this->get_logger(), "Goal reached! Shutting down.");
            rclcpp::shutdown(); 
        }
        publisher_->publish(msg);
    }

    turtlesim::msg::Pose current_pose_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr subscription_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ControllerNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}