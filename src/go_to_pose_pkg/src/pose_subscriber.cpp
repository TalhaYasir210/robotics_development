#include "rclcpp/rclcpp.hpp"
#include "turtlesim/msg/pose.hpp" // Note: We use turtlesim pose now!

class PoseSubscriber : public rclcpp::Node {
public:
    PoseSubscriber() : Node("pose_subscriber") {
        // We subscribe to the topic where the turtle sends its position
        subscription_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose", 10, std::bind(&PoseSubscriber::pose_callback, this, std::placeholders::_1));
    }

private:
    void pose_callback(const turtlesim::msg::Pose::SharedPtr msg) {
        // This function runs every time the turtle updates its position
        RCLCPP_INFO(this->get_logger(), "Current Pose: x=%f, y=%f, theta=%f", msg->x, msg->y, msg->theta);
    }
    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr subscription_;
};

int main(int argc, char * argv[]) {
    // 1. Initialize ROS 2
    rclcpp::init(argc, argv);
    
    // 2. Create the node and spin it
    rclcpp::spin(std::make_shared<PoseSubscriber>());
    
    // 3. Clean up
    rclcpp::shutdown();
    return 0;
}