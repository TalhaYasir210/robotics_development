#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"

class MockMapPublisher : public rclcpp::Node {
public:
    MockMapPublisher() : Node("mock_map_publisher") {
        // Create a publisher on the /map topic
        publisher_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>("/map", 10);
        
        // Publish the map every 2 seconds
        timer_ = this->create_wall_timer(
            std::chrono::seconds(2),
            std::bind(&MockMapPublisher::publishMap, this));
            
        RCLCPP_INFO(this->get_logger(), "Mock Map Publisher Started. Broadcasting 10x10 map...");
    }

private:
    void publishMap() {
        auto message = nav_msgs::msg::OccupancyGrid();
        
        // 1. Setup the map metadata
        message.header.stamp = this->now();
        message.header.frame_id = "map";
        message.info.resolution = 1.0; 
        message.info.width = 10;
        message.info.height = 10;
        
        // 2. Create a flat 1D array of 100 zeros (Free space)
        message.data.assign(100, 0); 
        
        // 3. Draw our wall (Obstacles = 100 in ROS)
        // Formula to find 1D index: index = x + (y * width)
        // Our wall from the C++ test was at y=5, x=2,3,4,5,6
        message.data[2 + (5 * 10)] = 100;
        message.data[3 + (5 * 10)] = 100;
        message.data[4 + (5 * 10)] = 100;
        message.data[5 + (5 * 10)] = 100;
        message.data[6 + (5 * 10)] = 100;

        // Publish to the network
        publisher_->publish(message);
    }

    rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MockMapPublisher>());
    rclcpp::shutdown();
    return 0;
}