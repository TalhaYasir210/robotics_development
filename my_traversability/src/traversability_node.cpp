#include "my_traversability/traversability_node.hpp"
#include "my_traversability/cloud_filter.hpp"

#include <cstring>
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

  // 5. Setup ROS 2 Publisher for 2D Traversability OccupancyGrid and Visualization Markers
  // QoS Transient Local / Reliability ensures Nav2 receives costmap cleanly
  occupancy_grid_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>(
    map_topic_, rclcpp::QoS(1).reliable());
  marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
    marker_topic_, rclcpp::QoS(1).reliable());

  // 6. Pre-allocate occupancy grid message buffer (Zero dynamic memory allocations at runtime)
  initializeOccupancyGridMessage();

  // 7. Setup Subscriber to Depth Camera PointCloud
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
  this->declare_parameter<std::string>("marker_topic", marker_topic_);

  this->declare_parameter<float>("grid_width", grid_width_);
  this->declare_parameter<float>("grid_length", grid_length_);
  this->declare_parameter<float>("resolution", resolution_);
  this->declare_parameter<float>("safe_step_threshold", safe_step_threshold_);
  this->declare_parameter<float>("obstacle_threshold", obstacle_threshold_);
  this->declare_parameter<float>("min_z_cutoff", min_z_cutoff_);
  this->declare_parameter<float>("max_z_cutoff", max_z_cutoff_);
  this->declare_parameter<bool>("enable_profiling", enable_profiling_);

  target_frame_ = this->get_parameter("target_frame").as_string();
  pointcloud_topic_ = this->get_parameter("pointcloud_topic").as_string();
  map_topic_ = this->get_parameter("map_topic").as_string();
  marker_topic_ = this->get_parameter("marker_topic").as_string();

  grid_width_ = static_cast<float>(this->get_parameter("grid_width").as_double());
  grid_length_ = static_cast<float>(this->get_parameter("grid_length").as_double());
  resolution_ = static_cast<float>(this->get_parameter("resolution").as_double());
  safe_step_threshold_ = static_cast<float>(this->get_parameter("safe_step_threshold").as_double());
  obstacle_threshold_ = static_cast<float>(this->get_parameter("obstacle_threshold").as_double());
  min_z_cutoff_ = static_cast<float>(this->get_parameter("min_z_cutoff").as_double());
  max_z_cutoff_ = static_cast<float>(this->get_parameter("max_z_cutoff").as_double());
  enable_profiling_ = this->get_parameter("enable_profiling").as_bool();
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
    } else if (name == "enable_profiling") {
      enable_profiling_ = param.as_bool();
      RCLCPP_INFO(
        this->get_logger(), "Runtime update: enable_profiling = %s",
        enable_profiling_ ? "true" : "false");
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
    initializeOccupancyGridMessage();
  }

  return result;
}

void TraversabilityNode::pointCloudCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)
{
  const auto t_frame_start = std::chrono::steady_clock::now();

  // 1. Look up transform from Camera Frame -> Robot Base Frame
  const auto t_tf_start = std::chrono::steady_clock::now();
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
  const auto t_tf_end = std::chrono::steady_clock::now();

  // 2. Wipe grid buffers in place (zero-allocation)
  grid_map_->reset();

  // 3. Extract byte offsets for x, y, and z fields from PointCloud2 metadata
  int offset_x = -1;
  int offset_y = -1;
  int offset_z = -1;

  for (const auto & field : msg->fields) {
    if (field.name == "x") {
      offset_x = static_cast<int>(field.offset);
    } else if (field.name == "y") {
      offset_y = static_cast<int>(field.offset);
    } else if (field.name == "z") {
      offset_z = static_cast<int>(field.offset);
    }
  }

  // Safety guard: skip frame if XYZ channels are missing
  if (offset_x < 0 || offset_y < 0 || offset_z < 0) {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 2000,
      "PointCloud2 is missing essential 'x', 'y', or 'z' field offsets.");
    return;
  }

  // 4. Fast Direct Pointer Arithmetic Parsing (Zero-Heap Ingestion)
  const auto t_ingest_start = std::chrono::steady_clock::now();
  const uint8_t * raw_ptr = msg->data.data();
  const uint32_t point_step = msg->point_step;
  const size_t total_points = static_cast<size_t>(msg->width * msg->height);
  const uint8_t * const raw_ptr_end = raw_ptr + (total_points * point_step);

  float in_x = 0.0f;
  float in_y = 0.0f;
  float in_z = 0.0f;
  float out_x = 0.0f;
  float out_y = 0.0f;
  float out_z = 0.0f;

  for (; raw_ptr < raw_ptr_end; raw_ptr += point_step) {
    // Standard-compliant, alignment-safe float read (compiles to single movss instruction)
    std::memcpy(&in_x, raw_ptr + offset_x, sizeof(float));
    std::memcpy(&in_y, raw_ptr + offset_y, sizeof(float));
    std::memcpy(&in_z, raw_ptr + offset_z, sizeof(float));

    // Check validity, transform coordinates into base_link, and filter bounds
    if (CloudFilter::filterAndTransform(
        sensor_to_base,
        in_x, in_y, in_z,
        out_x, out_y, out_z,
        min_z_cutoff_, max_z_cutoff_))
    {
      grid_map_->addPoint(out_x, out_y, out_z);
    }
  }
  const auto t_ingest_end = std::chrono::steady_clock::now();

  // 5. Evaluate Delta-Z and determine traversability cost per cell
  const auto t_compute_start = std::chrono::steady_clock::now();
  grid_map_->computeTraversability();
  const auto t_compute_end = std::chrono::steady_clock::now();

  // 6. Construct and publish standard Nav2 OccupancyGrid & Visualization Markers
  const auto t_pub_start = std::chrono::steady_clock::now();
  publishOccupancyGrid(msg->header);
  publishTraversabilityMarkers(msg->header);
  const auto t_pub_end = std::chrono::steady_clock::now();

  const auto t_frame_end = std::chrono::steady_clock::now();

  // 7. High-Resolution Latency & Profiling Bookkeeping (Issue 10)
  if (enable_profiling_) {
    const double tf_ms =
      std::chrono::duration<double, std::milli>(t_tf_end - t_tf_start).count();
    const double ingest_ms =
      std::chrono::duration<double, std::milli>(t_ingest_end - t_ingest_start).count();
    const double compute_ms =
      std::chrono::duration<double, std::milli>(t_compute_end - t_compute_start).count();
    const double pub_ms =
      std::chrono::duration<double, std::milli>(t_pub_end - t_pub_start).count();
    const double total_ms =
      std::chrono::duration<double, std::milli>(t_frame_end - t_frame_start).count();

    frame_count_++;
    total_latency_ms_ += total_ms;
    min_latency_ms_ = std::min(min_latency_ms_, total_ms);
    max_latency_ms_ = std::max(max_latency_ms_, total_ms);

    cumulative_tf_ms_ += tf_ms;
    cumulative_ingest_ms_ += ingest_ms;
    cumulative_compute_ms_ += compute_ms;
    cumulative_publish_ms_ += pub_ms;

    // Report periodic statistical summary every 100 frames
    if (frame_count_ % 100 == 0) {
      const double avg_total = total_latency_ms_ / static_cast<double>(frame_count_);
      const double avg_tf = cumulative_tf_ms_ / static_cast<double>(frame_count_);
      const double avg_ingest = cumulative_ingest_ms_ / static_cast<double>(frame_count_);
      const double avg_compute = cumulative_compute_ms_ / static_cast<double>(frame_count_);
      const double avg_pub = cumulative_publish_ms_ / static_cast<double>(frame_count_);
      const double est_fps = (avg_total > 0.0) ? (1000.0 / avg_total) : 0.0;

      RCLCPP_INFO(
        this->get_logger(),
        "[Profiling] Frames: %lu | Cur: %.2f ms (Avg: %.2f ms, Min: %.2f ms, Max: %.2f ms) | "
        "TF: %.2f ms | Ingest: %.2f ms | Compute: %.2f ms | Pub: %.2f ms | Max Capacity: %.0f Hz (Budget: 33.3 ms)",
        frame_count_, total_ms, avg_total, min_latency_ms_, max_latency_ms_,
        avg_tf, avg_ingest, avg_compute, avg_pub, est_fps);
    }
  }
}

void TraversabilityNode::initializeOccupancyGridMessage()
{
  occupancy_grid_msg_.header.frame_id = target_frame_;

  // Grid Map metadata (ROS REP-103 standard: width is along X-axis, height is along Y-axis)
  occupancy_grid_msg_.info.resolution = grid_map_->getResolution();
  occupancy_grid_msg_.info.width = static_cast<uint32_t>(grid_map_->getWidthX());
  occupancy_grid_msg_.info.height = static_cast<uint32_t>(grid_map_->getHeightY());

  // Set grid origin (cell [0, 0] is at X = 0.0, Y = -half_width_)
  occupancy_grid_msg_.info.origin.position.x = 0.0;
  occupancy_grid_msg_.info.origin.position.y = -grid_map_->getHalfWidth();
  occupancy_grid_msg_.info.origin.position.z = 0.0;
  occupancy_grid_msg_.info.origin.orientation.w = 1.0;

  // Pre-allocate message data buffer once with Nav2 unknown (-1)
  occupancy_grid_msg_.data.assign(grid_map_->getTotalCells(), static_cast<int8_t>(-1));
}

void TraversabilityNode::publishOccupancyGrid(const std_msgs::msg::Header & /*header*/)
{
  // Update timestamp to current ROS time
  occupancy_grid_msg_.header.stamp = this->now();

  // Copy contiguous costs directly into pre-allocated message buffer (Zero Heap Allocation)
  const auto & cost_map = grid_map_->getCostMap();
  std::copy(cost_map.begin(), cost_map.end(), occupancy_grid_msg_.data.begin());

  occupancy_grid_pub_->publish(occupancy_grid_msg_);
}

void TraversabilityNode::publishTraversabilityMarkers(const std_msgs::msg::Header & /*header*/)
{
  if (marker_pub_->get_subscription_count() == 0) {
    return;  // Zero overhead if no RViz client is listening
  }

  visualization_msgs::msg::MarkerArray marker_array;

  // 1. Grid Boundary Wireframe: Explicitly displays the full 4m x 4m local perception grid
  visualization_msgs::msg::Marker grid_marker;
  grid_marker.header.frame_id = target_frame_;
  // rclcpp::Time(0) locks to latest TF transform, eliminating simulation timestamp jitter / flickering
  grid_marker.header.stamp = rclcpp::Time(0);
  grid_marker.ns = "traversability_grid_boundary";
  grid_marker.id = 1;
  grid_marker.type = visualization_msgs::msg::Marker::LINE_LIST;
  grid_marker.action = visualization_msgs::msg::Marker::ADD;
  grid_marker.lifetime = rclcpp::Duration::from_seconds(0.15);
  grid_marker.scale.x = 0.015f;  // Line thickness in meters
  grid_marker.color.r = 0.40f;
  grid_marker.color.g = 0.65f;
  grid_marker.color.b = 0.85f;
  grid_marker.color.a = 0.45f;   // Subtle guide lines

  const float x_max = grid_length_;
  const float y_min = -grid_map_->getHalfWidth();
  const float y_max = grid_map_->getHalfWidth();
  const float z_floor = 0.005f;

  auto add_line = [&](float x1, float y1, float x2, float y2) {
    geometry_msgs::msg::Point p1, p2;
    p1.x = x1; p1.y = y1; p1.z = z_floor;
    p2.x = x2; p2.y = y2; p2.z = z_floor;
    grid_marker.points.push_back(p1);
    grid_marker.points.push_back(p2);
  };

  // Outer 4m x 4m boundary
  add_line(0.0f, y_min, x_max, y_min);
  add_line(x_max, y_min, x_max, y_max);
  add_line(x_max, y_max, 0.0f, y_max);
  add_line(0.0f, y_max, 0.0f, y_min);

  // Range reference lines (every 1.0m forward)
  for (float x_tick = 1.0f; x_tick < x_max; x_tick += 1.0f) {
    add_line(x_tick, y_min, x_tick, y_max);
  }

  // Lateral center line (heading axis)
  add_line(0.0f, 0.0f, x_max, 0.0f);
  // Lateral reference lines (-1m and +1m)
  add_line(0.0f, -1.0f, x_max, -1.0f);
  add_line(0.0f,  1.0f, x_max,  1.0f);

  marker_array.markers.push_back(std::move(grid_marker));

  // 2. Traversability Cells: High-performance 3D CUBE_LIST
  visualization_msgs::msg::Marker cells_marker;
  cells_marker.header.frame_id = target_frame_;
  // rclcpp::Time(0) locks to latest TF transform, eliminating simulation timestamp jitter / flickering
  cells_marker.header.stamp = rclcpp::Time(0);
  cells_marker.ns = "traversability_cells";
  cells_marker.id = 0;
  cells_marker.type = visualization_msgs::msg::Marker::CUBE_LIST;
  cells_marker.action = visualization_msgs::msg::Marker::ADD;
  cells_marker.lifetime = rclcpp::Duration::from_seconds(0.15);

  const float res = grid_map_->getResolution();
  cells_marker.scale.x = res * 0.95f;
  cells_marker.scale.y = res * 0.95f;
  cells_marker.scale.z = 0.015f;

  const auto & cost_map = grid_map_->getCostMap();
  const size_t total = cost_map.size();

  cells_marker.points.reserve(total);
  cells_marker.colors.reserve(total);

  geometry_msgs::msg::Point pt;
  std_msgs::msg::ColorRGBA color;

  for (size_t i = 0; i < total; ++i) {
    const int8_t cost = cost_map[i];
    if (cost < 0) {
      continue;  // Unknown cells are not rendered - eliminates background clutter!
    }

    float cx = 0.0f;
    float cy = 0.0f;
    grid_map_->getCellCenter(i, cx, cy);

    pt.x = cx;
    pt.y = cy;

    if (cost == 0) {
      // Safe Floor (Cost 0) -> Vivid Green
      pt.z = 0.01f;
      color.r = 0.10f;
      color.g = 0.85f;
      color.b = 0.20f;
      color.a = 0.60f;
    } else if (cost == 100) {
      // Lethal Obstacle (Cost 100) -> Vivid Red
      pt.z = 0.06f;
      color.r = 0.95f;
      color.g = 0.05f;
      color.b = 0.05f;
      color.a = 0.85f;
    } else {
      // Risky Slope/Roughness (Cost 1-99) -> Vivid Yellow
      pt.z = 0.035f;
      color.r = 0.95f;
      color.g = 0.85f;
      color.b = 0.05f;
      color.a = 0.75f;
    }

    cells_marker.points.push_back(pt);
    cells_marker.colors.push_back(color);
  }

  marker_array.markers.push_back(std::move(cells_marker));
  marker_pub_->publish(marker_array);
}

}  // namespace my_traversability
