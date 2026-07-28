// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "autonomous_navigation/msg/detail/navigation_command__functions.h"
#include "autonomous_navigation/msg/detail/navigation_command__struct.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

namespace autonomous_navigation
{

namespace msg
{

namespace rosidl_typesupport_introspection_cpp
{

void NavigationCommand_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) autonomous_navigation::msg::NavigationCommand(_init);
}

void NavigationCommand_fini_function(void * message_memory)
{
  auto typed_message = static_cast<autonomous_navigation::msg::NavigationCommand *>(message_memory);
  typed_message->~NavigationCommand();
}

size_t size_function__NavigationCommand__waypoints(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<geometry_msgs::msg::PoseStamped> *>(untyped_member);
  return member->size();
}

const void * get_const_function__NavigationCommand__waypoints(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<geometry_msgs::msg::PoseStamped> *>(untyped_member);
  return &member[index];
}

void * get_function__NavigationCommand__waypoints(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<geometry_msgs::msg::PoseStamped> *>(untyped_member);
  return &member[index];
}

void fetch_function__NavigationCommand__waypoints(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const geometry_msgs::msg::PoseStamped *>(
    get_const_function__NavigationCommand__waypoints(untyped_member, index));
  auto & value = *reinterpret_cast<geometry_msgs::msg::PoseStamped *>(untyped_value);
  value = item;
}

void assign_function__NavigationCommand__waypoints(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<geometry_msgs::msg::PoseStamped *>(
    get_function__NavigationCommand__waypoints(untyped_member, index));
  const auto & value = *reinterpret_cast<const geometry_msgs::msg::PoseStamped *>(untyped_value);
  item = value;
}

void resize_function__NavigationCommand__waypoints(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<geometry_msgs::msg::PoseStamped> *>(untyped_member);
  member->resize(size);
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember NavigationCommand_message_member_array[2] = {
  {
    "is_waypoint_nav",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(autonomous_navigation::msg::NavigationCommand, is_waypoint_nav),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "waypoints",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    ::rosidl_typesupport_introspection_cpp::get_message_type_support_handle<geometry_msgs::msg::PoseStamped>(),  // members of sub message
    false,  // is key
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(autonomous_navigation::msg::NavigationCommand, waypoints),  // bytes offset in struct
    nullptr,  // default value
    size_function__NavigationCommand__waypoints,  // size() function pointer
    get_const_function__NavigationCommand__waypoints,  // get_const(index) function pointer
    get_function__NavigationCommand__waypoints,  // get(index) function pointer
    fetch_function__NavigationCommand__waypoints,  // fetch(index, &value) function pointer
    assign_function__NavigationCommand__waypoints,  // assign(index, value) function pointer
    resize_function__NavigationCommand__waypoints  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers NavigationCommand_message_members = {
  "autonomous_navigation::msg",  // message namespace
  "NavigationCommand",  // message name
  2,  // number of fields
  sizeof(autonomous_navigation::msg::NavigationCommand),
  false,  // has_any_key_member_
  NavigationCommand_message_member_array,  // message members
  NavigationCommand_init_function,  // function to initialize message memory (memory has to be allocated)
  NavigationCommand_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t NavigationCommand_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &NavigationCommand_message_members,
  get_message_typesupport_handle_function,
  &autonomous_navigation__msg__NavigationCommand__get_type_hash,
  &autonomous_navigation__msg__NavigationCommand__get_type_description,
  &autonomous_navigation__msg__NavigationCommand__get_type_description_sources,
};

}  // namespace rosidl_typesupport_introspection_cpp

}  // namespace msg

}  // namespace autonomous_navigation


namespace rosidl_typesupport_introspection_cpp
{

template<>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<autonomous_navigation::msg::NavigationCommand>()
{
  return &::autonomous_navigation::msg::rosidl_typesupport_introspection_cpp::NavigationCommand_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, autonomous_navigation, msg, NavigationCommand)() {
  return &::autonomous_navigation::msg::rosidl_typesupport_introspection_cpp::NavigationCommand_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
