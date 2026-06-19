#include <memory>
#include <cmath>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

using std::placeholders::_1;

class LidarSubscriber : public rclcpp::Node {
public:
    LidarSubscriber() : Node("lidar_subscriber") {
        // Subscribe to the /scan topic
        subscription_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            "/scan", 10, std::bind(&LidarSubscriber::scan_callback, this, _1));
        
        RCLCPP_INFO(this->get_logger(), "Lidar Subscriber Node has started. Listening to /scan...");
    }

private:
    void scan_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg) const {
        // In the standard TurtleBot3 model, index 0 of the ranges array is directly in front.
        float front_distance = msg->ranges[0];

        // Lidar sensors return 'inf' (infinity) or 'nan' (not a number) if an object is too close, 
        // too far, or if there is nothing detected in that direction. We must filter this.
        if (std::isinf(front_distance) || std::isnan(front_distance)) {
            RCLCPP_INFO(this->get_logger(), "Front: Path is clear (inf/nan)");
        } else {
            RCLCPP_INFO(this->get_logger(), "Front Distance: %.2f meters", front_distance);
        }
    }

    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr subscription_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<LidarSubscriber>());
    rclcpp::shutdown();
    return 0;
}