/**
 * @file traversability_node.cpp
 * @brief ROS 2 node implementation bridging depth point clouds to the 2.5D elevation grid map.
 *
 * This file implements the TraversabilityNode class. It subscribes to sensor_msgs/PointCloud2
 * depth camera topics, performs TF2 frame lookups to transform points into the robot's base frame,
 * filters points by range and footprint, feeds them to the ElevationGridMap engine, and publishes
 * the resulting traversability costmap as a nav_msgs/OccupancyGrid.
 */

#include "traversability_mapping/traversability_node.hpp"

#include <cmath>
#include <cstring>

#include "rclcpp_components/register_node_macro.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"
#include "tf2_eigen/tf2_eigen.hpp"

namespace traversability_mapping {

TraversabilityNode::TraversabilityNode(const rclcpp::NodeOptions &options)
    : Node("traversability_node", options) {
  declareAndLoadParameters();

  // Initialize pure C++ elevation engine
  elevation_map_.initialize(grid_width_, grid_length_, resolution_,
                            safe_step_threshold_, obstacle_threshold_,
                            min_z_cutoff_, max_z_cutoff_);

  initializeTemplatePayload();

  // TF2 Transform Buffer and Listener
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // QoS configurations
  // Depth cameras generate volatile sensor streams; use Best Effort
  // SensorDataQoS
  const auto sensor_qos = rclcpp::SensorDataQoS();

  point_cloud_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
      input_topic_, sensor_qos,
      std::bind(&TraversabilityNode::pointCloudCallback, this,
                std::placeholders::_1));

  // OccupancyGrid publisher (Reliable QoS with depth 1)
  const auto map_qos = rclcpp::QoS(1).reliable().durability_volatile();
  occupancy_grid_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>(
      output_topic_, map_qos);

  RCLCPP_INFO(this->get_logger(),
              "TraversabilityNode initialized. Input topic: '%s', Output "
              "topic: '%s', Base Frame: '%s'",
              input_topic_.c_str(), output_topic_.c_str(), base_frame_.c_str());
  RCLCPP_INFO(this->get_logger(),
              "Grid dimensions: %.2fm x %.2fm @ %.3fm resolution (%d rows x %d "
              "cols = %zu cells)",
              grid_length_, grid_width_, resolution_, elevation_map_.getRows(),
              elevation_map_.getCols(), elevation_map_.getTotalCells());
}

void TraversabilityNode::declareAndLoadParameters() {
  this->declare_parameter<float>("resolution", 0.05f);
  this->declare_parameter<float>("grid_width", 4.0f);
  this->declare_parameter<float>("grid_length", 4.0f);
  this->declare_parameter<float>("safe_step_threshold", 0.06f);
  this->declare_parameter<float>("obstacle_threshold", 0.12f);
  this->declare_parameter<float>("min_z_cutoff", -0.15f);
  this->declare_parameter<float>("max_z_cutoff", 1.50f);
  this->declare_parameter<float>("min_x_cutoff", 0.20f);
  this->declare_parameter<float>("max_range", 4.5f);
  this->declare_parameter<std::string>("base_frame", "base_link");
  this->declare_parameter<std::string>("input_topic", "/camera/depth/points");
  this->declare_parameter<std::string>("output_topic", "/traversability_map");

  resolution_ =
      static_cast<float>(this->get_parameter("resolution").as_double());
  grid_width_ =
      static_cast<float>(this->get_parameter("grid_width").as_double());
  grid_length_ =
      static_cast<float>(this->get_parameter("grid_length").as_double());
  safe_step_threshold_ = static_cast<float>(
      this->get_parameter("safe_step_threshold").as_double());
  obstacle_threshold_ =
      static_cast<float>(this->get_parameter("obstacle_threshold").as_double());
  min_z_cutoff_ =
      static_cast<float>(this->get_parameter("min_z_cutoff").as_double());
  max_z_cutoff_ =
      static_cast<float>(this->get_parameter("max_z_cutoff").as_double());
  min_x_cutoff_ =
      static_cast<float>(this->get_parameter("min_x_cutoff").as_double());
  max_range_ =
      static_cast<float>(this->get_parameter("max_range").as_double());
  base_frame_ = this->get_parameter("base_frame").as_string();
  input_topic_ = this->get_parameter("input_topic").as_string();
  output_topic_ = this->get_parameter("output_topic").as_string();
}

void TraversabilityNode::initializeTemplatePayload() {
  occupancy_grid_template_.header.frame_id = base_frame_;
  occupancy_grid_template_.info.resolution = resolution_;
  occupancy_grid_template_.info.width =
      static_cast<uint32_t>(elevation_map_.getCols());
  occupancy_grid_template_.info.height =
      static_cast<uint32_t>(elevation_map_.getRows());

  // In the robot base frame:
  // Forward axis (+X) corresponds to rows: x in [0, grid_length_]
  // Lateral axis (+Y) corresponds to columns: y in [-half_width_, +half_width_]
  // In OccupancyGrid coordinate frame:
  // Axis u (columns) maps to +Y, Axis v (rows) maps to +X
  // A 180° rotation around (1, 1, 0) / sqrt(2) aligns the local grid axes with
  // base_link: q = (x=sqrt(2)/2, y=sqrt(2)/2, z=0, w=0)
  constexpr double SQRT2_INV = 0.70710678118654752440;
  occupancy_grid_template_.info.origin.position.x = 0.0;
  occupancy_grid_template_.info.origin.position.y =
      -static_cast<double>(elevation_map_.getHalfWidth());
  occupancy_grid_template_.info.origin.position.z = 0.0;
  occupancy_grid_template_.info.origin.orientation.x = SQRT2_INV;
  occupancy_grid_template_.info.origin.orientation.y = SQRT2_INV;
  occupancy_grid_template_.info.origin.orientation.z = 0.0;
  occupancy_grid_template_.info.origin.orientation.w = 0.0;

  // Pre-allocate costmap data buffer once to prevent reallocations
  occupancy_grid_template_.data.resize(elevation_map_.getTotalCells(), -1);
}

void TraversabilityNode::pointCloudCallback(
    const sensor_msgs::msg::PointCloud2::ConstSharedPtr &cloud_msg) {
  if (!cloud_msg) {
    return;
  }

  // Single TF lookup per incoming cloud message
  geometry_msgs::msg::TransformStamped tf_msg;
  try {
    tf_msg = tf_buffer_->lookupTransform(
        base_frame_, cloud_msg->header.frame_id, cloud_msg->header.stamp,
        rclcpp::Duration::from_seconds(0.10));
  } catch (const tf2::TransformException &ex) {
    // Fallback to latest available transform if timestamps differ slightly in
    // simulation clock
    try {
      tf_msg = tf_buffer_->lookupTransform(
          base_frame_, cloud_msg->header.frame_id, tf2::TimePointZero);
    } catch (const tf2::TransformException &ex_latest) {
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                           "Failed to transform cloud from '%s' to '%s': %s",
                           cloud_msg->header.frame_id.c_str(),
                           base_frame_.c_str(), ex_latest.what());
      return;
    }
  }

  // Convert transform to Eigen::Isometry3f for vectorized projection
  const Eigen::Isometry3d T_double = tf2::transformToEigen(tf_msg);
  const Eigen::Isometry3f T_base_camera = T_double.cast<float>();

  // Reset elevation buffers in-place (std::fill, zero heap allocations)
  elevation_map_.reset();

  // Zero-copy direct byte iteration through PointCloud2 message
  sensor_msgs::PointCloud2ConstIterator<float> iter_x(*cloud_msg, "x");
  sensor_msgs::PointCloud2ConstIterator<float> iter_y(*cloud_msg, "y");
  sensor_msgs::PointCloud2ConstIterator<float> iter_z(*cloud_msg, "z");

  float base_x{0.0f};
  float base_y{0.0f};
  float base_z{0.0f};

  for (; iter_x != iter_x.end(); ++iter_x, ++iter_y, ++iter_z) {
    const float raw_x = *iter_x;
    const float raw_y = *iter_y;
    const float raw_z = *iter_z;

    if (!CloudFilter::isValid(raw_x, raw_y, raw_z)) {
      continue;
    }

    // Discard points beyond the forward mapping horizon (raw_z is depth in camera frame)
    if (raw_z > max_range_) {
      continue;
    }

    CloudFilter::transformPoint(T_base_camera, raw_x, raw_y, raw_z, base_x,
                                base_y, base_z);

    // Reject points inside the robot footprint / chassis bumper
    if (base_x < min_x_cutoff_ && std::abs(base_y) < 0.18f) {
      continue;
    }

    elevation_map_.addPoint(base_x, base_y, base_z);
  }

  // Compute traversability cost across all cells
  elevation_map_.computeTraversability();

  // Update template headers and copy cost buffer
  occupancy_grid_template_.header.stamp = cloud_msg->header.stamp;
  occupancy_grid_template_.header.frame_id = base_frame_;
  occupancy_grid_template_.info.map_load_time = cloud_msg->header.stamp;

  const auto &cost_data = elevation_map_.getCostMap();
  std::memcpy(occupancy_grid_template_.data.data(), cost_data.data(),
              cost_data.size() * sizeof(int8_t));

  // Publish traversability occupancy grid
  occupancy_grid_pub_->publish(occupancy_grid_template_);
}

} // namespace traversability_mapping

RCLCPP_COMPONENTS_REGISTER_NODE(traversability_mapping::TraversabilityNode)
