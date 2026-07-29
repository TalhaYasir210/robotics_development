// generated from rosidl_typesupport_fastrtps_c/resource/idl__rosidl_typesupport_fastrtps_c.h.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice
#ifndef AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
#define AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_


#include <stddef.h>
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "autonomous_navigation/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "autonomous_navigation/msg/detail/navigation_command__struct.h"
#include "fastcdr/Cdr.h"

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
bool cdr_serialize_autonomous_navigation__msg__NavigationCommand(
  const autonomous_navigation__msg__NavigationCommand * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
bool cdr_deserialize_autonomous_navigation__msg__NavigationCommand(
  eprosima::fastcdr::Cdr &,
  autonomous_navigation__msg__NavigationCommand * ros_message);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
size_t get_serialized_size_autonomous_navigation__msg__NavigationCommand(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
size_t max_serialized_size_autonomous_navigation__msg__NavigationCommand(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
bool cdr_serialize_key_autonomous_navigation__msg__NavigationCommand(
  const autonomous_navigation__msg__NavigationCommand * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
size_t get_serialized_size_key_autonomous_navigation__msg__NavigationCommand(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
size_t max_serialized_size_key_autonomous_navigation__msg__NavigationCommand(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_autonomous_navigation
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, autonomous_navigation, msg, NavigationCommand)();

#ifdef __cplusplus
}
#endif

#endif  // AUTONOMOUS_NAVIGATION__MSG__DETAIL__NAVIGATION_COMMAND__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
