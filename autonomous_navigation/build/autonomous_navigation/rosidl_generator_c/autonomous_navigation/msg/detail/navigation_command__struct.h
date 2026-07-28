// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "autonomous_navigation/msg/navigation_command.h"


#ifndef AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__STRUCT_H_
#define AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'waypoints'
#include "geometry_msgs/msg/detail/pose_stamped__struct.h"

/// Struct defined in msg/NavigationCommand in the package autonomous_navigation.
/**
  * True if this is a follow waypoints command, False if it's a single pose goal
 */
typedef struct autonomous_navigation__msg__NavigationCommand
{
  bool is_waypoint_nav;
  /// The target poses.
  /// If is_waypoint_nav is False, only the first pose in the array is used.
  /// If is_waypoint_nav is True, the robot will navigate through all poses sequentially.
  geometry_msgs__msg__PoseStamped__Sequence waypoints;
} autonomous_navigation__msg__NavigationCommand;

// Struct for a sequence of autonomous_navigation__msg__NavigationCommand.
typedef struct autonomous_navigation__msg__NavigationCommand__Sequence
{
  autonomous_navigation__msg__NavigationCommand * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} autonomous_navigation__msg__NavigationCommand__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__STRUCT_H_
