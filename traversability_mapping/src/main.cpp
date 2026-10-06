/**
 * @file main.cpp
 * @brief Standalone executable entry point for the traversability_mapping ROS 2 node.
 *
 * This file initializes the rclcpp runtime, instantiates the TraversabilityNode
 * component, and executes the ROS 2 single-threaded executor to process incoming
 * depth camera point clouds and publish traversability costmaps until shutdown.
 */

#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "traversability_mapping/traversability_node.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  rclcpp::NodeOptions options;
  auto node = std::make_shared<traversability_mapping::TraversabilityNode>(options);

  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}
