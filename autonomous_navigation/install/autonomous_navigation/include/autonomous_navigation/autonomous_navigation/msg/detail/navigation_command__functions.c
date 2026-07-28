// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from autonomous_navigation:msg/NavigationCommand.idl
// generated code does not contain a copyright notice
#include "autonomous_navigation/msg/detail/navigation_command__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `waypoints`
#include "geometry_msgs/msg/detail/pose_stamped__functions.h"

bool
autonomous_navigation__msg__NavigationCommand__init(autonomous_navigation__msg__NavigationCommand * msg)
{
  if (!msg) {
    return false;
  }
  // is_waypoint_nav
  // waypoints
  if (!geometry_msgs__msg__PoseStamped__Sequence__init(&msg->waypoints, 0)) {
    autonomous_navigation__msg__NavigationCommand__fini(msg);
    return false;
  }
  return true;
}

void
autonomous_navigation__msg__NavigationCommand__fini(autonomous_navigation__msg__NavigationCommand * msg)
{
  if (!msg) {
    return;
  }
  // is_waypoint_nav
  // waypoints
  geometry_msgs__msg__PoseStamped__Sequence__fini(&msg->waypoints);
}

bool
autonomous_navigation__msg__NavigationCommand__are_equal(const autonomous_navigation__msg__NavigationCommand * lhs, const autonomous_navigation__msg__NavigationCommand * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // is_waypoint_nav
  if (lhs->is_waypoint_nav != rhs->is_waypoint_nav) {
    return false;
  }
  // waypoints
  if (!geometry_msgs__msg__PoseStamped__Sequence__are_equal(
      &(lhs->waypoints), &(rhs->waypoints)))
  {
    return false;
  }
  return true;
}

bool
autonomous_navigation__msg__NavigationCommand__copy(
  const autonomous_navigation__msg__NavigationCommand * input,
  autonomous_navigation__msg__NavigationCommand * output)
{
  if (!input || !output) {
    return false;
  }
  // is_waypoint_nav
  output->is_waypoint_nav = input->is_waypoint_nav;
  // waypoints
  if (!geometry_msgs__msg__PoseStamped__Sequence__copy(
      &(input->waypoints), &(output->waypoints)))
  {
    return false;
  }
  return true;
}

autonomous_navigation__msg__NavigationCommand *
autonomous_navigation__msg__NavigationCommand__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  autonomous_navigation__msg__NavigationCommand * msg = (autonomous_navigation__msg__NavigationCommand *)allocator.allocate(sizeof(autonomous_navigation__msg__NavigationCommand), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(autonomous_navigation__msg__NavigationCommand));
  bool success = autonomous_navigation__msg__NavigationCommand__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
autonomous_navigation__msg__NavigationCommand__destroy(autonomous_navigation__msg__NavigationCommand * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    autonomous_navigation__msg__NavigationCommand__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
autonomous_navigation__msg__NavigationCommand__Sequence__init(autonomous_navigation__msg__NavigationCommand__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  autonomous_navigation__msg__NavigationCommand * data = NULL;

  if (size) {
    if (size > SIZE_MAX / sizeof(autonomous_navigation__msg__NavigationCommand)) {
      return false;
    }
    data = (autonomous_navigation__msg__NavigationCommand *)allocator.zero_allocate(size, sizeof(autonomous_navigation__msg__NavigationCommand), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = autonomous_navigation__msg__NavigationCommand__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        autonomous_navigation__msg__NavigationCommand__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
autonomous_navigation__msg__NavigationCommand__Sequence__fini(autonomous_navigation__msg__NavigationCommand__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      autonomous_navigation__msg__NavigationCommand__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

autonomous_navigation__msg__NavigationCommand__Sequence *
autonomous_navigation__msg__NavigationCommand__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  autonomous_navigation__msg__NavigationCommand__Sequence * array = (autonomous_navigation__msg__NavigationCommand__Sequence *)allocator.allocate(sizeof(autonomous_navigation__msg__NavigationCommand__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = autonomous_navigation__msg__NavigationCommand__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
autonomous_navigation__msg__NavigationCommand__Sequence__destroy(autonomous_navigation__msg__NavigationCommand__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    autonomous_navigation__msg__NavigationCommand__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
autonomous_navigation__msg__NavigationCommand__Sequence__are_equal(const autonomous_navigation__msg__NavigationCommand__Sequence * lhs, const autonomous_navigation__msg__NavigationCommand__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!autonomous_navigation__msg__NavigationCommand__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
autonomous_navigation__msg__NavigationCommand__Sequence__copy(
  const autonomous_navigation__msg__NavigationCommand__Sequence * input,
  autonomous_navigation__msg__NavigationCommand__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    if (input->size > SIZE_MAX / sizeof(autonomous_navigation__msg__NavigationCommand)) {
      return false;
    }
    const size_t allocation_size =
      input->size * sizeof(autonomous_navigation__msg__NavigationCommand);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    autonomous_navigation__msg__NavigationCommand * data =
      (autonomous_navigation__msg__NavigationCommand *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!autonomous_navigation__msg__NavigationCommand__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          autonomous_navigation__msg__NavigationCommand__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!autonomous_navigation__msg__NavigationCommand__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
