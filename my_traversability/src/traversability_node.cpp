#include "my_traversability/traversability_node.hpp"
#include "my_traversability/cloud_filter.hpp"

#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <tf2_eigen/tf2_eigen.hpp>
#include <tf2/exceptions.h>

namespace my_traversability
{

TraversabilityNode::TraversabilityNode(const rclcpp::NodeOptions & options)
: Node("traversability_node", options)
{
  // 1. Declare and load parameters (with defaults)
  declareAndInitializeParameters();

  // 2. Instantiate core elevation map using std::make_unique (exclusive ownership)
  grid_map_ = std::make_unique<ElevationGridMap>(
    grid_width_,
    grid_length_,
    resolution_,
    safe_step_threshold_,
    obstacle_threshold_,
    min_z_cutoff_,
    max_z_cutoff_);

  // 3. Initialize TF2 transform buffer and listener with node clock
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // 4. Register dynamic parameter callback for live runtime tuning
  param_callback_handle_ = this->add_on_set_parameters_callback(
    std::bind(&TraversabilityNode::onParameterChange, this, std::placeholders::_1));

  // 5. Setup ROS 2 Publisher for 2D Traversability OccupancyGrid
  // QoS Transient Local / Reliability ensures Nav2 receives costmap cleanly
  occupancy_grid_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>(
    map_topic_, rclcpp::QoS(1).reliable());

  // 6. Setup Subscriber to Depth Camera PointCloud
  // SensorDataQoS uses Best Effort / Volatile for low-latency 30Hz streaming
  pointcloud_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
    pointcloud_topic_,
    rclcpp::SensorDataQoS(),
    std::bind(&TraversabilityNode::pointCloudCallback, this, std::placeholders::_1));

  RCLCPP_INFO(
    this->get_logger(),
    "TraversabilityNode initialized. Listening on '%s', Publishing to '%s'",
    pointcloud_topic_.c_str(), map_topic_.c_str());
}

void TraversabilityNode::declareAndInitializeParameters()
{
  this->declare_parameter<std::string>("target_frame", target_frame_);
  this->declare_parameter<std::string>("pointcloud_topic", pointcloud_topic_);
  this->declare_parameter<std::string>("map_topic", map_topic_);

  this->declare_parameter<float>("grid_width", grid_width_);
  this->declare_parameter<float>("grid_length", grid_length_);
  this->declare_parameter<float>("resolution", resolution_);
  this->declare_parameter<float>("safe_step_threshold", safe_step_threshold_);
  this->declare_parameter<float>("obstacle_threshold", obstacle_threshold_);
  this->declare_parameter<float>("min_z_cutoff", min_z_cutoff_);
  this->declare_parameter<float>("max_z_cutoff", max_z_cutoff_);

  target_frame_ = this->get_parameter("target_frame").as_string();
  pointcloud_topic_ = this->get_parameter("pointcloud_topic").as_string();
  map_topic_ = this->get_parameter("map_topic").as_string();

  grid_width_ = static_cast<float>(this->get_parameter("grid_width").as_double());
  grid_length_ = static_cast<float>(this->get_parameter("grid_length").as_double());
  resolution_ = static_cast<float>(this->get_parameter("resolution").as_double());
  safe_step_threshold_ = static_cast<float>(this->get_parameter("safe_step_threshold").as_double());
  obstacle_threshold_ = static_cast<float>(this->get_parameter("obstacle_threshold").as_double());
  min_z_cutoff_ = static_cast<float>(this->get_parameter("min_z_cutoff").as_double());
  max_z_cutoff_ = static_cast<float>(this->get_parameter("max_z_cutoff").as_double());
}

rcl_interfaces::msg::SetParametersResult TraversabilityNode::onParameterChange(
  const std::vector<rclcpp::Parameter> & parameters)
{
  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  result.reason = "success";

  bool rebuild_needed = false;

  for (const auto & param : parameters) {
    const auto & name = param.get_name();

    if (name == "safe_step_threshold") {
      safe_step_threshold_ = static_cast<float>(param.as_double());
      rebuild_needed = true;
      RCLCPP_INFO(this->get_logger(), "Runtime update: safe_step_threshold = %.3f", safe_step_threshold_);
    } else if (name == "obstacle_threshold") {
      obstacle_threshold_ = static_cast<float>(param.as_double());
      rebuild_needed = true;
      RCLCPP_INFO(this->get_logger(), "Runtime update: obstacle_threshold = %.3f", obstacle_threshold_);
    } else if (name == "min_z_cutoff") {
      min_z_cutoff_ = static_cast<float>(param.as_double());
      rebuild_needed = true;
      RCLCPP_INFO(this->get_logger(), "Runtime update: min_z_cutoff = %.3f", min_z_cutoff_);
    } else if (name == "max_z_cutoff") {
      max_z_cutoff_ = static_cast<float>(param.as_double());
      rebuild_needed = true;
      RCLCPP_INFO(this->get_logger(), "Runtime update: max_z_cutoff = %.3f", max_z_cutoff_);
    } else if (name == "resolution") {
      const float new_res = static_cast<float>(param.as_double());
      if (new_res <= 0.0f) {
        result.successful = false;
        result.reason = "Resolution must be greater than zero.";
        return result;
      }
      resolution_ = new_res;
      rebuild_needed = true;
      RCLCPP_INFO(this->get_logger(), "Runtime update: resolution = %.3f", resolution_);
    }
  }

  // If parameters changed, reconstruct the grid map safely with new parameters
  if (rebuild_needed) {
    grid_map_ = std::make_unique<ElevationGridMap>(
      grid_width_,
      grid_length_,
      resolution_,
      safe_step_threshold_,
      obstacle_threshold_,
      min_z_cutoff_,
      max_z_cutoff_);
  }

  return result;
}

void TraversabilityNode::pointCloudCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)
{
  // 1. Look up transform from Camera Frame -> Robot Base Frame
  geometry_msgs::msg::TransformStamped tf_stamped;
  try {
    tf_stamped = tf_buffer_->lookupTransform(
      target_frame_,
      msg->header.frame_id,
      msg->header.stamp,
      rclcpp::Duration::from_seconds(0.05));
  } catch (const tf2::TransformException & ex) {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 2000,
      "Could not transform '%s' to '%s': %s",
      msg->header.frame_id.c_str(), target_frame_.c_str(), ex.what());
    return;
  }

  // Convert ROS transform to Eigen Isometry3f (single 4x4 matrix representation)
  const Eigen::Isometry3f sensor_to_base = tf2::transformToEigen(tf_stamped).cast<float>();

  // 2. Wipe grid buffers in place (zero-allocation)
  grid_map_->reset();

  // 3. Fast Zero-Copy PointCloud2 Iterator
  sensor_msgs::PointCloud2ConstIterator<float> iter_x(*msg, "x");
  sensor_msgs::PointCloud2ConstIterator<float> iter_y(*msg, "y");
  sensor_msgs::PointCloud2ConstIterator<float> iter_z(*msg, "z");

  float out_x = 0.0f;
  float out_y = 0.0f;
  float out_z = 0.0f;

  for (; iter_x != iter_x.end(); ++iter_x, ++iter_y, ++iter_z) {
    // Check NaN/Inf and transform into base frame
    if (CloudFilter::filterAndTransform(
        sensor_to_base,
        *iter_x, *iter_y, *iter_z,
        out_x, out_y, out_z,
        min_z_cutoff_, max_z_cutoff_))
    {
      grid_map_->addPoint(out_x, out_y, out_z);
    }
  }

  // 4. Evaluate Delta-Z and determine traversability cost per cell
  grid_map_->computeTraversability();

  // 5. Construct and publish standard Nav2 OccupancyGrid
  publishOccupancyGrid(msg->header);
}

void TraversabilityNode::publishOccupancyGrid(const std_msgs::msg::Header & /*header*/)
{
  nav_msgs::msg::OccupancyGrid map_msg;

  // Header metadata
  map_msg.header.stamp = this->now();
  map_msg.header.frame_id = target_frame_;

  // Grid Map metadata
  map_msg.info.resolution = grid_map_->getResolution();
  map_msg.info.width = static_cast<uint32_t>(grid_map_->getCols());
  map_msg.info.height = static_cast<uint32_t>(grid_map_->getRows());

  // Set grid origin (cell [0, 0] is at X = 0.0, Y = -half_width_)
  map_msg.info.origin.position.x = 0.0;
  map_msg.info.origin.position.y = -grid_map_->getHalfWidth();
  map_msg.info.origin.position.z = 0.0;
  map_msg.info.origin.orientation.w = 1.0;

  // Direct move/copy of contiguous cost buffer (SoA efficiency)
  map_msg.data = grid_map_->getCostMap();

  occupancy_grid_pub_->publish(std::move(map_msg));
}

}  // namespace my_traversability
