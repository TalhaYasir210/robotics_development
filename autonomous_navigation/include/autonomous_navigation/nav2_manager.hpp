#ifndef AUTONOMOUS_NAVIGATION__NAV2_MANAGER_HPP_
#define AUTONOMOUS_NAVIGATION__NAV2_MANAGER_HPP_

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "nav2_msgs/action/follow_waypoints.hpp"
#include "autonomous_navigation/msg/navigation_command.hpp"
#include <memory>
#include <string>

namespace autonomous_navigation
{

class Nav2Manager : public rclcpp::Node
{
public:
  // Type aliases to make the code much cleaner and easier to read
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using FollowWaypoints = nav2_msgs::action::FollowWaypoints;
  using GoalHandleNavigate = rclcpp_action::ClientGoalHandle<NavigateToPose>;
  using GoalHandleWaypoints = rclcpp_action::ClientGoalHandle<FollowWaypoints>;

  // Constructor
  explicit Nav2Manager(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

  // Function called when a new command arrives from the GUI
  void send_nav_command(const autonomous_navigation::msg::NavigationCommand::SharedPtr msg);

  // Cancels the current active goal
  void cancel_goal();

private:
  // Action Clients
  rclcpp_action::Client<NavigateToPose>::SharedPtr client_ptr_navigate_;
  rclcpp_action::Client<FollowWaypoints>::SharedPtr client_ptr_waypoints_;

  // Subscriber to our custom message
  rclcpp::Subscription<autonomous_navigation::msg::NavigationCommand>::SharedPtr command_sub_;

  // State to track if we have an active goal
  bool has_active_goal_{false};
  // Goal handles to allow cancellation
  GoalHandleNavigate::SharedPtr active_goal_handle_navigate_;
  GoalHandleWaypoints::SharedPtr active_goal_handle_waypoints_;

  // Callbacks for NavigateToPose Action Server
  void goal_response_callback_nav(const GoalHandleNavigate::SharedPtr & goal_handle);
  void feedback_callback_nav(
    GoalHandleNavigate::SharedPtr,
    const std::shared_ptr<const NavigateToPose::Feedback> feedback);
  void result_callback_nav(const GoalHandleNavigate::WrappedResult & result);

  // Callbacks for FollowWaypoints Action Server
  void goal_response_callback_waypoint(const GoalHandleWaypoints::SharedPtr & goal_handle);
  void feedback_callback_waypoint(
    GoalHandleWaypoints::SharedPtr,
    const std::shared_ptr<const FollowWaypoints::Feedback> feedback);
  void result_callback_waypoint(const GoalHandleWaypoints::WrappedResult & result);
};

}  // namespace autonomous_navigation

#endif  // AUTONOMOUS_NAVIGATION__NAV2_MANAGER_HPP_
