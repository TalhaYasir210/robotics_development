#ifndef DYNAMIC_OBSTACLE_AVOIDANCE__NAV2_CUSTOM_PLANNER_HPP_
#define DYNAMIC_OBSTACLE_AVOIDANCE__NAV2_CUSTOM_PLANNER_HPP_

#include <string>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/path.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav2_core/global_planner.hpp"
#include "nav2_costmap_2d/costmap_2d_ros.hpp"
#include "nav2_util/lifecycle_node.hpp"
#include "nav2_util/node_utils.hpp"

#include "dynamic_obstacle_avoidance/a_star_planner.hpp"
#include "dynamic_obstacle_avoidance/rrt_planner.hpp"

namespace dynamic_obstacle_avoidance
{

class Nav2CustomPlanner : public nav2_core::GlobalPlanner
{
public:
  Nav2CustomPlanner() = default;
  ~Nav2CustomPlanner() = default;

  // nav2_core::GlobalPlanner interface methods
  void configure(
    const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
    std::string name, std::shared_ptr<tf2_ros::Buffer> tf,
    std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros) override;

  void cleanup() override;
  void activate() override;
  void deactivate() override;

  nav_msgs::msg::Path createPlan(
    const geometry_msgs::msg::PoseStamped & start,
    const geometry_msgs::msg::PoseStamped & goal,
    std::function<bool()> cancel_checker) override;

private:

  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros_;
  nav2_costmap_2d::Costmap2D * costmap_;
  rclcpp::Logger logger_{rclcpp::get_logger("Nav2CustomPlanner")};
  std::string name_;

  // Our custom planners
  std::shared_ptr<AStarPlanner> a_star_planner_;
  std::shared_ptr<RRTPlanner> rrt_planner_;
};

}  // namespace dynamic_obstacle_avoidance

#endif  // DYNAMIC_OBSTACLE_AVOIDANCE__NAV2_CUSTOM_PLANNER_HPP_
