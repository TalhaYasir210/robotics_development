// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "autonomous_navigation/msg/navigation_command.hpp"


#ifndef AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__BUILDER_HPP_
#define AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "autonomous_navigation/msg/detail/navigation_command__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace autonomous_navigation
{

namespace msg
{

namespace builder
{

class Init_NavigationCommand_waypoints
{
public:
  explicit Init_NavigationCommand_waypoints(::autonomous_navigation::msg::NavigationCommand & msg)
  : msg_(msg)
  {}
  ::autonomous_navigation::msg::NavigationCommand waypoints(::autonomous_navigation::msg::NavigationCommand::_waypoints_type arg)
  {
    msg_.waypoints = std::move(arg);
    return std::move(msg_);
  }

private:
  ::autonomous_navigation::msg::NavigationCommand msg_;
};

class Init_NavigationCommand_is_waypoint_nav
{
public:
  Init_NavigationCommand_is_waypoint_nav()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_NavigationCommand_waypoints is_waypoint_nav(::autonomous_navigation::msg::NavigationCommand::_is_waypoint_nav_type arg)
  {
    msg_.is_waypoint_nav = std::move(arg);
    return Init_NavigationCommand_waypoints(msg_);
  }

private:
  ::autonomous_navigation::msg::NavigationCommand msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::autonomous_navigation::msg::NavigationCommand>()
{
  return autonomous_navigation::msg::builder::Init_NavigationCommand_is_waypoint_nav();
}

}  // namespace autonomous_navigation

#endif  // AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__BUILDER_HPP_
