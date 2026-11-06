#include <memory>
#include <rclcpp/rclcpp.hpp>
#include "my_traversability/traversability_node.hpp"

int main(int argc, char ** argv)
{
  // Initialize ROS 2 communications
  rclcpp::init(argc, argv);

  // Use std::make_shared to allocate Node and control block in a single heap chunk
  auto node = std::make_shared<my_traversability::TraversabilityNode>();

  // Spin the node to process incoming callbacks (depth clouds, parameters)
  rclcpp::spin(node);

  // Clean shutdown of ROS 2 context
  rclcpp::shutdown();
  return 0;
}

