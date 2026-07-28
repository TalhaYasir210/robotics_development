// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "autonomous_navigation/msg/navigation_command.hpp"


#ifndef AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__TRAITS_HPP_
#define AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "autonomous_navigation/msg/detail/navigation_command__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'waypoints'
#include "geometry_msgs/msg/detail/pose_stamped__traits.hpp"

namespace autonomous_navigation
{

namespace msg
{

inline void to_flow_style_yaml(
  const NavigationCommand & msg,
  std::ostream & out)
{
  out << "{";
  // member: is_waypoint_nav
  {
    out << "is_waypoint_nav: ";
    rosidl_generator_traits::value_to_yaml(msg.is_waypoint_nav, out);
    out << ", ";
  }

  // member: waypoints
  {
    if (msg.waypoints.size() == 0) {
      out << "waypoints: []";
    } else {
      out << "waypoints: [";
      size_t pending_items = msg.waypoints.size();
      for (auto item : msg.waypoints) {
        to_flow_style_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const NavigationCommand & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: is_waypoint_nav
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "is_waypoint_nav: ";
    rosidl_generator_traits::value_to_yaml(msg.is_waypoint_nav, out);
    out << "\n";
  }

  // member: waypoints
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.waypoints.size() == 0) {
      out << "waypoints: []\n";
    } else {
      out << "waypoints:\n";
      for (auto item : msg.waypoints) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "-\n";
        to_block_style_yaml(item, out, indentation + 2);
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const NavigationCommand & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace autonomous_navigation

namespace rosidl_generator_traits
{

[[deprecated("use autonomous_navigation::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const autonomous_navigation::msg::NavigationCommand & msg,
  std::ostream & out, size_t indentation = 0)
{
  autonomous_navigation::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use autonomous_navigation::msg::to_yaml() instead")]]
inline std::string to_yaml(const autonomous_navigation::msg::NavigationCommand & msg)
{
  return autonomous_navigation::msg::to_yaml(msg);
}

template<>
inline const char * data_type<autonomous_navigation::msg::NavigationCommand>()
{
  return "autonomous_navigation::msg::NavigationCommand";
}

template<>
inline const char * name<autonomous_navigation::msg::NavigationCommand>()
{
  return "autonomous_navigation/msg/NavigationCommand";
}

template<>
struct has_fixed_size<autonomous_navigation::msg::NavigationCommand>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<autonomous_navigation::msg::NavigationCommand>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<autonomous_navigation::msg::NavigationCommand>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__TRAITS_HPP_
