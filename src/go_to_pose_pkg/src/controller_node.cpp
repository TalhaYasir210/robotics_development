#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "turtlesim/msg/pose.hpp"
#include <cmath>

enum class State { NAVIGATING, ADJUSTING_ANGLE, FINISHED };

class ControllerNode : public rclcpp::Node
{
public:
    ControllerNode() : Node("controller_node")
    {
        this->declare_parameter("target_x", 5.0);
        this->declare_parameter("target_y", 5.0);
        this->declare_parameter("target_theta", 0.0);
        
        publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);
        subscription_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose", 10, std::bind(&ControllerNode::pose_callback, this, std::placeholders::_1));
        timer_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&ControllerNode::control_loop, this));
    }

private:
    State current_state_ = State::NAVIGATING;
    turtlesim::msg::Pose current_pose_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr subscription_;
    rclcpp::TimerBase::SharedPtr timer_;

    void pose_callback(const turtlesim::msg::Pose::SharedPtr msg) { current_pose_ = *msg; }

    void control_loop()
    {
        double goal_x = this->get_parameter("target_x").as_double();
        double goal_y = this->get_parameter("target_y").as_double();
        double target_theta = this->get_parameter("target_theta").as_double();

        // 1. Validation: Shutdown on negative input
        if (goal_x < 0 || goal_y < 0) {
            RCLCPP_ERROR(this->get_logger(), "Invalid input: Coordinates must be positive! Shutting down.");
            rclcpp::shutdown();
            return;
        }

        auto msg = geometry_msgs::msg::Twist();

        switch (current_state_) {
            case State::NAVIGATING:
                {
                    double dx = goal_x - current_pose_.x;
                    double dy = goal_y - current_pose_.y;
                    double distance = std::sqrt(dx * dx + dy * dy);
                    double angle_error = std::atan2(dy, dx) - current_pose_.theta;
                    
                    while (angle_error > M_PI) angle_error -= 2 * M_PI;
                    while (angle_error < -M_PI) angle_error += 2 * M_PI;

                    // Debugging Telemetry
                    RCLCPP_DEBUG(this->get_logger(), "Pose: [x:%.2f, y:%.2f, θ:%.2f] | Dist: %.2f | AngErr: %.2f", 
                                 current_pose_.x, current_pose_.y, current_pose_.theta, distance, angle_error);

                    if (distance > 0.1) {
                        msg.linear.x = 0.5 * distance;
                        msg.angular.z = 4.0 * angle_error;
                    } else {
                        current_state_ = State::ADJUSTING_ANGLE;
                    }
                }
                break;

            case State::ADJUSTING_ANGLE:
                {
                    double angle_error = target_theta - current_pose_.theta;
                    while (angle_error > M_PI) angle_error -= 2 * M_PI;
                    while (angle_error < -M_PI) angle_error += 2 * M_PI;

                    if (std::abs(angle_error) > 0.05) {
                        msg.angular.z = 2.0 * angle_error;
                    } else {
                        current_state_ = State::FINISHED;
                    }
                }
                break;

            case State::FINISHED:
                RCLCPP_INFO(this->get_logger(), "Mission accomplished! Shutting down.");
                rclcpp::shutdown();
                break;
        }
        publisher_->publish(msg);
    }
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ControllerNode>());
    rclcpp::shutdown();
    return 0;
}