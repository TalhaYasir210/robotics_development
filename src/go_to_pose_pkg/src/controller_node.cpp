#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "turtlesim/msg/pose.hpp"
#include <cmath> // Required for sqrt, atan2, etc.

class ControllerNode : public rclcpp::Node
{
public:
    ControllerNode() : Node("controller_node")
    {
        this->declare_parameter("target_x", 5.0);
        this->declare_parameter("target_y", 5.0);
        // Publisher to move the robot
        publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);

        // Subscriber to know where the robot is
        subscription_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose", 10, std::bind(&ControllerNode::pose_callback, this, std::placeholders::_1));

        // Timer to run the control loop
        timer_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&ControllerNode::control_loop, this));
    }

private:
    void pose_callback(const turtlesim::msg::Pose::SharedPtr msg)
    {
        current_pose_ = *msg;
    }

    void control_loop()
    {
        // This is where you will add your math logic next!
        RCLCPP_INFO(this->get_logger(), "Current: x=%f, y=%f, theta=%f", current_pose_.x, current_pose_.y, current_pose_.theta);
        // 1. Define your goal
        double goal_x = this->get_parameter("target_x").as_double();
        double goal_y = this->get_parameter("target_y").as_double();

        if (goal_x < 0 || goal_y < 0) {
        RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 5000, "Invalid goal! Coordinates must be positive.");
        return; // Stop the function here, turtle won't move
        }

        // 2. Calculate Distance to goal
        double dx = goal_x - current_pose_.x;
        double dy = goal_y - current_pose_.y;
        double distance = std::sqrt(dx * dx + dy * dy);

        // 3. Calculate desired angle (Heading)
        double desired_angle = std::atan2(dy, dx);
        double angle_error = desired_angle - current_pose_.theta;

        // 4. Create the command message
        auto msg = geometry_msgs::msg::Twist();

        // 5. Basic Proportional Controller logic
        if (distance > 0.1)
        {
            msg.linear.x = 0.5 * distance; // Speed proportional to distance
            // Normalize angle_error to keep it between -PI and PI
            while (angle_error > M_PI)
                angle_error -= 2 * M_PI;
            while (angle_error < -M_PI)
                angle_error += 2 * M_PI;
            msg.angular.z = 4.0 * angle_error; // Turn proportional to angle error
        }
        else
        {
            msg.linear.x = 0.0;
            msg.angular.z = 0.0;
            RCLCPP_INFO(this->get_logger(), "Goal reached!");
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
    rclcpp::spin(std::make_shared<ControllerNode>());
    rclcpp::shutdown();
    return 0;
}