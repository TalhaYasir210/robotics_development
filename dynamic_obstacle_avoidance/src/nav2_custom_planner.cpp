#include "dynamic_obstacle_avoidance/nav2_custom_planner.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace dynamic_obstacle_avoidance
{

void Nav2CustomPlanner::configure(
  const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
  std::string name, std::shared_ptr<tf2_ros::Buffer> tf,
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros)
{
  (void)tf;
  name_ = name;
  costmap_ros_ = costmap_ros;
  costmap_ = costmap_ros_->getCostmap();
  auto node = parent.lock();
  logger_ = node->get_logger();

  RCLCPP_INFO(logger_, "Configuring Nav2CustomPlanner...");

  // Initialize planners
  a_star_planner_ = std::make_shared<AStarPlanner>();
  rrt_planner_ = std::make_shared<RRTPlanner>();
}

void Nav2CustomPlanner::cleanup()
{
  RCLCPP_INFO(logger_, "Cleaning up Nav2CustomPlanner...");
  a_star_planner_.reset();
  rrt_planner_.reset();
}

void Nav2CustomPlanner::activate()
{
  RCLCPP_INFO(logger_, "Activating Nav2CustomPlanner...");
}

void Nav2CustomPlanner::deactivate()
{
  RCLCPP_INFO(logger_, "Deactivating Nav2CustomPlanner...");
}

nav_msgs::msg::Path Nav2CustomPlanner::createPlan(
  const geometry_msgs::msg::PoseStamped & start,
  const geometry_msgs::msg::PoseStamped & goal,
  std::function<bool()> cancel_checker)
{
  (void)cancel_checker; // Unused for now
  
  RCLCPP_INFO(logger_, "Creating plan from (%.2f, %.2f) to (%.2f, %.2f)",
              start.pose.position.x, start.pose.position.y,
              goal.pose.position.x, goal.pose.position.y);

  nav_msgs::msg::Path global_path;
  global_path.header.stamp = rclcpp::Clock().now();
  global_path.header.frame_id = costmap_ros_->getGlobalFrameID();

  // Extract map data from Costmap2D
  int width = costmap_->getSizeInCellsX();
  int height = costmap_->getSizeInCellsY();
  double resolution = costmap_->getResolution();
  double origin_x = costmap_->getOriginX();
  double origin_y = costmap_->getOriginY();
  
  unsigned char* data = costmap_->getCharMap();
  std::vector<int8_t> map_data(width * height);
  for (int i = 0; i < width * height; ++i) {
    if (data[i] == 255) {
      map_data[i] = -1;
    } else if (data[i] == 254 || data[i] == 253) {
      map_data[i] = 100;
    } else {
      map_data[i] = (int8_t)((data[i] / 252.0) * 100.0);
    }
  }

  // ==========================================
  // UNCOMMENT THE PLANNER YOU WANT TO USE
  // ==========================================
  
  // Option 1: A* Planner
  std::vector<Point2D> point_path = a_star_planner_->findPath(
    map_data, width, height, resolution, origin_x, origin_y,
    start.pose.position.x, start.pose.position.y,
    goal.pose.position.x, goal.pose.position.y
  );
  
  // Option 2: RRT Planner (Uncomment below to use RRT instead)
  /*
  std::vector<Point2D> point_path = rrt_planner_->findPath(
    map_data, width, height, resolution, origin_x, origin_y,
    start.pose.position.x, start.pose.position.y,
    goal.pose.position.x, goal.pose.position.y
  );
  */
  // ==========================================

  if (point_path.empty()) {
    RCLCPP_ERROR(logger_, "[Nav2CustomPlanner::createPlan] ERROR: Failed to find a valid global path! Goal might be unreachable or inside an obstacle.");
    return global_path;
  }

  // Convert Point2D to PoseStamped
  std::vector<geometry_msgs::msg::PoseStamped> path_poses;
  for (const auto& pt : point_path) {
    geometry_msgs::msg::PoseStamped pose;
    pose.header = global_path.header;
    pose.pose.position.x = pt.x;
    pose.pose.position.y = pt.y;
    pose.pose.position.z = 0.0;
    pose.pose.orientation.w = 1.0;
    path_poses.push_back(pose);
  }

  global_path.poses = path_poses;
  return global_path;
}

}  // namespace dynamic_obstacle_avoidance

PLUGINLIB_EXPORT_CLASS(dynamic_obstacle_avoidance::Nav2CustomPlanner, nav2_core::GlobalPlanner)
