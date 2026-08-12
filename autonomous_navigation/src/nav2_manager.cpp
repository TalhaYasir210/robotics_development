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

    auto goal_msg = FollowWaypoints::Goal();
    goal_msg.poses = msg->waypoints;

    auto send_goal_options = rclcpp_action::Client<FollowWaypoints>::SendGoalOptions();
    send_goal_options.goal_response_callback =
      std::bind(&Nav2Manager::goal_response_callback_waypoint, this, std::placeholders::_1);
    send_goal_options.feedback_callback =
      std::bind(&Nav2Manager::feedback_callback_waypoint, this, std::placeholders::_1,
        std::placeholders::_2);
    send_goal_options.result_callback =
      std::bind(&Nav2Manager::result_callback_waypoint, this, std::placeholders::_1);

    client_ptr_waypoints_->async_send_goal(goal_msg, send_goal_options);
    has_active_goal_ = true;

  } else {
    RCLCPP_INFO(this->get_logger(), "Received Single Pose command.");
    if (!client_ptr_navigate_->action_server_is_ready()) {
      RCLCPP_ERROR(this->get_logger(), "NavigateToPose Action server not available");
      return;
    }

    if (msg->waypoints.empty()) {
      RCLCPP_ERROR(this->get_logger(), "No waypoints provided for Single Pose command!");
      return;
    }

    auto goal_msg = NavigateToPose::Goal();
    goal_msg.pose = msg->waypoints.front();

    auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
    send_goal_options.goal_response_callback =
      std::bind(&Nav2Manager::goal_response_callback_nav, this, std::placeholders::_1);
    send_goal_options.feedback_callback =
      std::bind(&Nav2Manager::feedback_callback_nav, this, std::placeholders::_1,
        std::placeholders::_2);
    send_goal_options.result_callback =
      std::bind(&Nav2Manager::result_callback_nav, this, std::placeholders::_1);

    client_ptr_navigate_->async_send_goal(goal_msg, send_goal_options);
    has_active_goal_ = true;
  }
}

void Nav2Manager::cancel_goal()
{
  RCLCPP_INFO(this->get_logger(), "Cancel goal requested.");
  if (!has_active_goal_) {
    RCLCPP_WARN(this->get_logger(), "No active goal to cancel.");
    return;
  }

  if (active_goal_handle_navigate_ && client_ptr_navigate_->action_server_is_ready()) {
    client_ptr_navigate_->async_cancel_goal(active_goal_handle_navigate_);
  }

  if (active_goal_handle_waypoints_ && client_ptr_waypoints_->action_server_is_ready()) {
    client_ptr_waypoints_->async_cancel_goal(active_goal_handle_waypoints_);
  }
}

// --- Skeleton Callbacks for NavigateToPose ---
void Nav2Manager::goal_response_callback_nav(
  const GoalHandleNavigate::SharedPtr & goal_handle)
{
  if (!goal_handle) {
    RCLCPP_ERROR(this->get_logger(), "NavigateToPose goal was rejected by server");
    has_active_goal_ = false;
  } else {
    RCLCPP_INFO(this->get_logger(), "NavigateToPose goal accepted by server, waiting for result");
    active_goal_handle_navigate_ = goal_handle;
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
  has_active_goal_ = false;
  active_goal_handle_navigate_.reset();

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
    has_active_goal_ = false;
  } else {
    RCLCPP_INFO(this->get_logger(), "FollowWaypoints goal accepted by server, waiting for result");
    active_goal_handle_waypoints_ = goal_handle;
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
  has_active_goal_ = false;
  active_goal_handle_waypoints_.reset();

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
