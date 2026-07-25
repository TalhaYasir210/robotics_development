#include "autonomous_navigation/nav2_manager.hpp"

namespace autonomous_navigation
{

Nav2Manager::Nav2Manager(const rclcpp::NodeOptions & options)
: Node("nav2_manager", options)
{
  // 1. Initialize Action Clients
  client_ptr_navigate_ = rclcpp_action::create_client<NavigateToPose>(
    this, "navigate_to_pose");
  client_ptr_waypoints_ = rclcpp_action::create_client<FollowWaypoints>(
    this, "follow_waypoints");

  // 2. Initialize Subscriber to listen for GUI commands
  command_sub_ = this->create_subscription<autonomous_navigation::msg::NavigationCommand>(
    "gui_nav_command", 10,
    std::bind(&Nav2Manager::send_nav_command, this, std::placeholders::_1));

  RCLCPP_INFO(this->get_logger(), "Nav2Manager Node has been started.");
}

void Nav2Manager::send_nav_command(
  const autonomous_navigation::msg::NavigationCommand::SharedPtr msg)
{
  // Basic logic skeleton - we will expand this to pass the GTests!
  if (msg->is_waypoint_nav) {
    RCLCPP_INFO(this->get_logger(), "Received Waypoint command. Route contains %zu waypoints.",
        msg->waypoints.size());
    if (!client_ptr_waypoints_->action_server_is_ready()) {
      RCLCPP_ERROR(this->get_logger(), "FollowWaypoints Action server not available");
      return;
    }
    // send goal (dummy logic for now)
  } else {
    RCLCPP_INFO(this->get_logger(), "Received Single Pose command.");
    if (!client_ptr_navigate_->action_server_is_ready()) {
      RCLCPP_ERROR(this->get_logger(), "NavigateToPose Action server not available");
      return;
    }
    // send goal (dummy logic for now)
  }
}

void Nav2Manager::cancel_goal()
{
  RCLCPP_INFO(this->get_logger(), "Cancel goal requested.");
}

// --- Skeleton Callbacks for NavigateToPose ---
void Nav2Manager::goal_response_callback_nav(const GoalHandleNavigate::SharedPtr & goal_handle)
{
  if (!goal_handle) {
    RCLCPP_ERROR(this->get_logger(), "NavigateToPose goal was rejected by server");
  } else {
    RCLCPP_INFO(this->get_logger(), "NavigateToPose goal accepted by server, waiting for result");
  }
}

void Nav2Manager::feedback_callback_nav(
  GoalHandleNavigate::SharedPtr,
  const std::shared_ptr<const NavigateToPose::Feedback> feedback)
{
  RCLCPP_INFO(this->get_logger(), "Distance remaining: %.2f", feedback->distance_remaining);
}

void Nav2Manager::result_callback_nav(const GoalHandleNavigate::WrappedResult & result)
{
  switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      RCLCPP_INFO(this->get_logger(), "NavigateToPose goal succeeded!");
      break;
    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_ERROR(this->get_logger(), "NavigateToPose goal was aborted");
      return;
    case rclcpp_action::ResultCode::CANCELED:
      RCLCPP_WARN(this->get_logger(), "NavigateToPose goal was canceled");
      return;
    default:
      RCLCPP_ERROR(this->get_logger(), "Unknown result code");
      return;
  }
}

// --- Skeleton Callbacks for FollowWaypoints ---
void Nav2Manager::goal_response_callback_waypoint(
  const GoalHandleWaypoints::SharedPtr & goal_handle)
{
  if (!goal_handle) {
    RCLCPP_ERROR(this->get_logger(), "FollowWaypoints goal was rejected by server");
  } else {
    RCLCPP_INFO(this->get_logger(), "FollowWaypoints goal accepted by server, waiting for result");
  }
}

void Nav2Manager::feedback_callback_waypoint(
  GoalHandleWaypoints::SharedPtr,
  const std::shared_ptr<const FollowWaypoints::Feedback> feedback)
{
  RCLCPP_INFO(this->get_logger(), "Currently executing waypoint: %d", feedback->current_waypoint);
}

void Nav2Manager::result_callback_waypoint(const GoalHandleWaypoints::WrappedResult & result)
{
  switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      RCLCPP_INFO(this->get_logger(), "FollowWaypoints goal succeeded!");
      break;
    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_ERROR(this->get_logger(), "FollowWaypoints goal was aborted");
      return;
    case rclcpp_action::ResultCode::CANCELED:
      RCLCPP_WARN(this->get_logger(), "FollowWaypoints goal was canceled");
      return;
    default:
      RCLCPP_ERROR(this->get_logger(), "Unknown result code");
      return;
  }
}

}  // namespace autonomous_navigation
