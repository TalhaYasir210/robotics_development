// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "autonomous_navigation/msg/detail/navigation_command__rosidl_typesupport_introspection_c.h"
#include "autonomous_navigation/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "autonomous_navigation/msg/detail/navigation_command__functions.h"
#include "autonomous_navigation/msg/detail/navigation_command__struct.h"


// Include directives for member types
// Member `waypoints`
#include "geometry_msgs/msg/pose_stamped.h"
// Member `waypoints`
#include "geometry_msgs/msg/detail/pose_stamped__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  autonomous_navigation__msg__NavigationCommand__init(message_memory);
}

void autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_fini_function(void * message_memory)
{
  autonomous_navigation__msg__NavigationCommand__fini(message_memory);
}

size_t autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__size_function__NavigationCommand__waypoints(
  const void * untyped_member)
{
  const geometry_msgs__msg__PoseStamped__Sequence * member =
    (const geometry_msgs__msg__PoseStamped__Sequence *)(untyped_member);
  return member->size;
}

const void * autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__get_const_function__NavigationCommand__waypoints(
  const void * untyped_member, size_t index)
{
  const geometry_msgs__msg__PoseStamped__Sequence * member =
    (const geometry_msgs__msg__PoseStamped__Sequence *)(untyped_member);
  return &member->data[index];
}

void * autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__get_function__NavigationCommand__waypoints(
  void * untyped_member, size_t index)
{
  geometry_msgs__msg__PoseStamped__Sequence * member =
    (geometry_msgs__msg__PoseStamped__Sequence *)(untyped_member);
  return &member->data[index];
}

void autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__fetch_function__NavigationCommand__waypoints(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const geometry_msgs__msg__PoseStamped * item =
    ((const geometry_msgs__msg__PoseStamped *)
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__get_const_function__NavigationCommand__waypoints(untyped_member, index));
  geometry_msgs__msg__PoseStamped * value =
    (geometry_msgs__msg__PoseStamped *)(untyped_value);
  *value = *item;
}

void autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__assign_function__NavigationCommand__waypoints(
  void * untyped_member, size_t index, const void * untyped_value)
{
  geometry_msgs__msg__PoseStamped * item =
    ((geometry_msgs__msg__PoseStamped *)
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__get_function__NavigationCommand__waypoints(untyped_member, index));
  const geometry_msgs__msg__PoseStamped * value =
    (const geometry_msgs__msg__PoseStamped *)(untyped_value);
  *item = *value;
}

bool autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__resize_function__NavigationCommand__waypoints(
  void * untyped_member, size_t size)
{
  geometry_msgs__msg__PoseStamped__Sequence * member =
    (geometry_msgs__msg__PoseStamped__Sequence *)(untyped_member);
  geometry_msgs__msg__PoseStamped__Sequence__fini(member);
  return geometry_msgs__msg__PoseStamped__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_member_array[2] = {
  {
    "is_waypoint_nav",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_BOOLEAN,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(autonomous_navigation__msg__NavigationCommand, is_waypoint_nav),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "waypoints",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(autonomous_navigation__msg__NavigationCommand, waypoints),  // bytes offset in struct
    NULL,  // default value
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__size_function__NavigationCommand__waypoints,  // size() function pointer
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__get_const_function__NavigationCommand__waypoints,  // get_const(index) function pointer
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__get_function__NavigationCommand__waypoints,  // get(index) function pointer
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__fetch_function__NavigationCommand__waypoints,  // fetch(index, &value) function pointer
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__assign_function__NavigationCommand__waypoints,  // assign(index, value) function pointer
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__resize_function__NavigationCommand__waypoints  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_members = {
  "autonomous_navigation__msg",  // message namespace
  "NavigationCommand",  // message name
  2,  // number of fields
  sizeof(autonomous_navigation__msg__NavigationCommand),
  false,  // has_any_key_member_
  autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_member_array,  // message members
  autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_init_function,  // function to initialize message memory (memory has to be allocated)
  autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_type_support_handle = {
  0,
  &autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_members,
  get_message_typesupport_handle_function,
  &autonomous_navigation__msg__NavigationCommand__get_type_hash,
  &autonomous_navigation__msg__NavigationCommand__get_type_description,
  &autonomous_navigation__msg__NavigationCommand__get_type_description_sources,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_autonomous_navigation
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, autonomous_navigation, msg, NavigationCommand)() {
  autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_member_array[1].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, geometry_msgs, msg, PoseStamped)();
  if (!autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_type_support_handle.typesupport_identifier) {
    autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &autonomous_navigation__msg__NavigationCommand__rosidl_typesupport_introspection_c__NavigationCommand_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
