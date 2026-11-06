#ifndef MY_TRAVERSABILITY_TRAVERSABILITY_NODE_HPP_
#define MY_TRAVERSABILITY_TRAVERSABILITY_NODE_HPP_

#include <chrono>
#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <rcl_interfaces/msg/set_parameters_result.hpp>

#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

#include "my_traversability/elevation_grid_map.hpp"

namespace my_traversability
{


class TraversabilityNode : public rclcpp::Node
{
public:
  explicit TraversabilityNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~TraversabilityNode() override = default;

  // Rule of 5: Prevent accidental copying of ROS 2 Node handles
  TraversabilityNode(const TraversabilityNode &) = delete;
  TraversabilityNode & operator=(const TraversabilityNode &) = delete;
  TraversabilityNode(TraversabilityNode &&) = delete;
  TraversabilityNode & operator=(TraversabilityNode &&) = delete;

  // --- Real-Time Latency & Profiling Queries (Issue 10) ---
  bool isProfilingEnabled() const noexcept { return enable_profiling_; }
  uint64_t getFrameCount() const noexcept { return frame_count_; }
  double getAverageLatencyMs() const noexcept {
    return frame_count_ > 0 ? (total_latency_ms_ / static_cast<double>(frame_count_)) : 0.0;
  }
  double getMinLatencyMs() const noexcept { return frame_count_ > 0 ? min_latency_ms_ : 0.0; }
  double getMaxLatencyMs() const noexcept { return max_latency_ms_; }
  void resetProfilingStats() noexcept
  {
    frame_count_ = 0;
    total_latency_ms_ = 0.0;
    min_latency_ms_ = std::numeric_limits<double>::max();
    max_latency_ms_ = 0.0;
    cumulative_tf_ms_ = 0.0;
    cumulative_ingest_ms_ = 0.0;
    cumulative_compute_ms_ = 0.0;
    cumulative_publish_ms_ = 0.0;
  }

private:
  // --- Perception Pipeline Callbacks ---
  void pointCloudCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg);

  // --- Dynamic Runtime Parameter Reconfiguration ---
  rcl_interfaces::msg::SetParametersResult onParameterChange(
    const std::vector<rclcpp::Parameter> & parameters);

  // --- Helpers ---
  void declareAndInitializeParameters();
  void initializeOccupancyGridMessage();
  void publishOccupancyGrid(const std_msgs::msg::Header & header);

  // --- ROS 2 Interfaces ---
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_sub_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr occupancy_grid_pub_;

  // --- Pre-allocated Message Buffer (Zero Heap Allocation at Runtime) ---
  nav_msgs::msg::OccupancyGrid occupancy_grid_msg_;

  // --- Coordinate Transformations (TF2) ---
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  // --- Dynamic Parameter Callback Handle ---
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_callback_handle_;

  // --- Core Perception Engine (Exclusive Ownership) ---
  std::unique_ptr<ElevationGridMap> grid_map_;

  // --- Node Configuration & Runtime Parameters ---
  std::string target_frame_{"base_link"};
  std::string pointcloud_topic_{"/camera/depth/points"};
  std::string map_topic_{"/traversability_map"};

  // Grid Extents & Step Thresholds
  float grid_width_{4.0f};
  float grid_length_{4.0f};
  float resolution_{0.05f};
  float safe_step_threshold_{0.06f};
  float obstacle_threshold_{0.12f};
  float min_z_cutoff_{-0.15f};
  float max_z_cutoff_{1.05f};

  // --- Real-Time Latency & CPU Profiling State (Issue 10) ---
  bool enable_profiling_{true};
  uint64_t frame_count_{0};
  double total_latency_ms_{0.0};
  double min_latency_ms_{std::numeric_limits<double>::max()};
  double max_latency_ms_{0.0};

  // Cumulative breakdown across pipeline stages
  double cumulative_tf_ms_{0.0};
  double cumulative_ingest_ms_{0.0};
  double cumulative_compute_ms_{0.0};
  double cumulative_publish_ms_{0.0};
};

}  // namespace my_traversability

#endif  // MY_TRAVERSABILITY_TRAVERSABILITY_NODE_HPP_
