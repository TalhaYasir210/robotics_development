#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include <algorithm>

class SimpleVelocitySmoother : public rclcpp::Node
{
public:
  SimpleVelocitySmoother() : Node("simple_velocity_smoother")
  {
    publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("cmd_vel", 10);
    subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel_unstamped", 10,
      std::bind(&SimpleVelocitySmoother::topic_callback, this, std::placeholders::_1));

    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(50), // 20Hz
      std::bind(&SimpleVelocitySmoother::timer_callback, this));

    RCLCPP_INFO(this->get_logger(), "Simple Velocity Smoother initialized.");
  }

private:
  void topic_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    target_vel_ = *msg;
    
    // Cap target velocity to safe limits
    target_vel_.linear.x = std::clamp(target_vel_.linear.x, -max_vel_lin_, max_vel_lin_);
    target_vel_.angular.z = std::clamp(target_vel_.angular.z, -max_vel_ang_, max_vel_ang_);
    
    last_msg_time_ = this->get_clock()->now();
  }

  void timer_callback()
  {
    auto now = this->get_clock()->now();
    double dt = 0.05; // 20 Hz

    // Timeout if no message received for 0.5s
    if (last_msg_time_.nanoseconds() > 0 && (now - last_msg_time_).seconds() > 0.5) {
      target_vel_ = geometry_msgs::msg::Twist();
    }

    // Step linear velocity
    double step_lin = max_accel_lin_ * dt;
    if (target_vel_.linear.x > current_vel_.linear.x) {
      current_vel_.linear.x = std::min(target_vel_.linear.x, current_vel_.linear.x + step_lin);
    } else if (target_vel_.linear.x < current_vel_.linear.x) {
      current_vel_.linear.x = std::max(target_vel_.linear.x, current_vel_.linear.x - step_lin);
    }

    // Step angular velocity
    double step_ang = max_accel_ang_ * dt;
    if (target_vel_.angular.z > current_vel_.angular.z) {
      current_vel_.angular.z = std::min(target_vel_.angular.z, current_vel_.angular.z + step_ang);
    } else if (target_vel_.angular.z < current_vel_.angular.z) {
      current_vel_.angular.z = std::max(target_vel_.angular.z, current_vel_.angular.z - step_ang);
    }

    // Publish
    auto stamped_msg = geometry_msgs::msg::TwistStamped();
    stamped_msg.header.stamp = now;
    stamped_msg.header.frame_id = "base_link";
    stamped_msg.twist = current_vel_;
    publisher_->publish(stamped_msg);
  }

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscription_;
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;

  geometry_msgs::msg::Twist target_vel_;
  geometry_msgs::msg::Twist current_vel_;
  rclcpp::Time last_msg_time_ = rclcpp::Time(0, 0, RCL_ROS_TIME);

  // Safe limits for TurtleBot3
  double max_accel_lin_ = 0.05; // m/s^2
  double max_accel_ang_ = 0.20;  // rad/s^2
  
  double max_vel_lin_ = 0.20;   // m/s
  double max_vel_ang_ = 0.80;   // rad/s
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SimpleVelocitySmoother>());
  rclcpp::shutdown();
  return 0;
}
