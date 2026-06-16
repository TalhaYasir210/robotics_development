#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"

class SimplePublisher : public rclcpp::Node {
public:
    SimplePublisher() : Node("simple_publisher") {
        publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(500), 
            std::bind(&SimplePublisher::timer_callback, this));
    }

private:
    void timer_callback() {
        auto msg = geometry_msgs::msg::Twist();
        msg.linear.x = 1.0; 
        msg.angular.z = 0.5;
        
        RCLCPP_INFO(this->get_logger(), "Publishing velocity: linear=%f, angular=%f", msg.linear.x, msg.angular.z);
        publisher_->publish(msg);
    }
    
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SimplePublisher>());
    rclcpp::shutdown();
    return 0;
}