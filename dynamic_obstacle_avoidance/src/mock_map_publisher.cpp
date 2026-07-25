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
        
        // --- YOUR CUSTOM MAP ---
        // 0 = Free Space, 1 = Wall/Obstacle
        // Feel free to change the size or draw your own obstacles!
        std::vector<std::vector<int>> custom_map = {
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 1, 0, 0, 1, 0, 0},
            {0, 0, 0, 0, 1, 0, 0, 1, 0, 0},
            {0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0, 0, 1, 1, 1, 0},
            {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
            {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
        };

        int height = custom_map.size();
        int width = custom_map[0].size();

        // 1. Setup the map metadata
        message.header.stamp = this->now();
        message.header.frame_id = "map";
        message.info.resolution = 1.0;  // 1 meter per cell
        message.info.width = width;
        message.info.height = height;
        
        // 2. Convert visual 2D map into the flat 1D array ROS expects
        message.data.assign(width * height, 0); 
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (custom_map[y][x] == 1) {
                    int index = x + (y * width);
                    message.data[index] = 100; // 100 = Obstacle in ROS
                }
            }
        }

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