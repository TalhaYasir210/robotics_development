// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "autonomous_navigation/msg/navigation_command.hpp"


#ifndef AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__STRUCT_HPP_
#define AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'waypoints'
#include "geometry_msgs/msg/detail/pose_stamped__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__autonomous_navigation__msg__NavigationCommand __attribute__((deprecated))
#else
# define DEPRECATED__autonomous_navigation__msg__NavigationCommand __declspec(deprecated)
#endif

namespace autonomous_navigation
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct NavigationCommand_
{
  using Type = NavigationCommand_<ContainerAllocator>;

  explicit NavigationCommand_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->is_waypoint_nav = false;
    }
  }

  explicit NavigationCommand_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->is_waypoint_nav = false;
    }
  }

  // field types and members
  using _is_waypoint_nav_type =
    bool;
  _is_waypoint_nav_type is_waypoint_nav;
  using _waypoints_type =
    std::vector<geometry_msgs::msg::PoseStamped_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<geometry_msgs::msg::PoseStamped_<ContainerAllocator>>>;
  _waypoints_type waypoints;

  // setters for named parameter idiom
  Type & set__is_waypoint_nav(
    const bool & _arg)
  {
    this->is_waypoint_nav = _arg;
    return *this;
  }
  Type & set__waypoints(
    const std::vector<geometry_msgs::msg::PoseStamped_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<geometry_msgs::msg::PoseStamped_<ContainerAllocator>>> & _arg)
  {
    this->waypoints = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    autonomous_navigation::msg::NavigationCommand_<ContainerAllocator> *;
  using ConstRawPtr =
    const autonomous_navigation::msg::NavigationCommand_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      autonomous_navigation::msg::NavigationCommand_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      autonomous_navigation::msg::NavigationCommand_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__autonomous_navigation__msg__NavigationCommand
    std::shared_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__autonomous_navigation__msg__NavigationCommand
    std::shared_ptr<autonomous_navigation::msg::NavigationCommand_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const NavigationCommand_ & other) const
  {
    if (this->is_waypoint_nav != other.is_waypoint_nav) {
      return false;
    }
    if (this->waypoints != other.waypoints) {
      return false;
    }
    return true;
  }
  bool operator!=(const NavigationCommand_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct NavigationCommand_

// alias to use template instance with default allocator
using NavigationCommand =
  autonomous_navigation::msg::NavigationCommand_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace autonomous_navigation

#endif  // AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__STRUCT_HPP_
