/**
 * @file traversability_node.hpp
 * @brief Header definition for the ROS 2 TraversabilityNode lifecycle/component class.
 *
 * Declares the ROS 2 node adapter responsible for point cloud subscription, TF2 coordinate
 * transforms, ROS parameter management, template message allocation, and costmap publishing.
 */

#ifndef TRAVERSABILITY_MAPPING_TRAVERSABILITY_NODE_HPP_
#define TRAVERSABILITY_MAPPING_TRAVERSABILITY_NODE_HPP_

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

#include "traversability_mapping/elevation_grid_map.hpp"
#include "traversability_mapping/cloud_filter.hpp"

namespace traversability_mapping
{

/**
 * @class TraversabilityNode
 * @brief Thin ROS 2 Adapter wrapping the pure C++ ElevationGridMap engine.
 *
 * Responsibilities:
 * 1. Subscribes to sensor_msgs/msg/PointCloud2 with Best Effort SensorDataQoS.
 * 2. Queries tf2_ros::Buffer once per message to get sensor-to-base transform.
 * 3. Parses cloud bytes directly via PointCloud2ConstIterator (zero PCL overhead).
 * 4. Feeds points to ElevationGridMap and computes traversability cost.
 * 5. Copies cost bytes into a pre-allocated nav_msgs/msg/OccupancyGrid and publishes.
 */
class TraversabilityNode : public rclcpp::Node
{
public:
  /**
   * @brief Node constructor supporting ROS 2 component composition.
   * @param options ROS 2 NodeOptions.
   */
  explicit TraversabilityNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

  ~TraversabilityNode() override = default;

private:
  /**
   * @brief Loads and validates ROS 2 parameters from configuration / YAML.
   */
  void declareAndLoadParameters();

  /**
   * @brief Pre-allocates and configures the OccupancyGrid payload template.
   */
  void initializeTemplatePayload();

  /**
   * @brief Point cloud subscription callback with zero-copy iteration and single TF lookup.
   * @param cloud_msg Received depth camera point cloud.
   */
  void pointCloudCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr & cloud_msg);

  // ROS 2 Communication
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr point_cloud_sub_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr occupancy_grid_pub_;

  // Transform Listener
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  // Pure C++ Core Engine
  ElevationGridMap elevation_map_;

  // Pre-allocated OccupancyGrid message template (zero allocation on publish)
  nav_msgs::msg::OccupancyGrid occupancy_grid_template_;

  // Node Configuration Parameters
  float resolution_{0.05f};
  float grid_width_{4.0f};
  float grid_length_{4.0f};
  float safe_step_threshold_{0.06f};
  float obstacle_threshold_{0.12f};
  float min_z_cutoff_{-0.15f};
  float max_z_cutoff_{1.50f};
  float min_x_cutoff_{0.20f};
  float max_range_{4.5f};
  std::string base_frame_{"base_link"};
  std::string input_topic_{"/camera/depth/points"};
  std::string output_topic_{"/traversability_map"};
};

}  // namespace traversability_mapping

#endif  // TRAVERSABILITY_MAPPING_TRAVERSABILITY_NODE_HPP_
