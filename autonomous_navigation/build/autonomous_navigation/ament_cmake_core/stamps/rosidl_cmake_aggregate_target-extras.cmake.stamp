# generated from rosidl_cmake/cmake/rosidl_cmake_aggregate_target-extras.cmake.in

# Create a convenience aggregate target autonomous_navigation::autonomous_navigation
# that links all generated interface targets, so downstream packages can use
# a single modern CMake target name instead of ${autonomous_navigation_TARGETS}.
if(autonomous_navigation_TARGETS AND NOT TARGET autonomous_navigation::autonomous_navigation)
  add_library(autonomous_navigation::autonomous_navigation INTERFACE IMPORTED)
  set_target_properties(autonomous_navigation::autonomous_navigation PROPERTIES
    INTERFACE_LINK_LIBRARIES "${autonomous_navigation_TARGETS}")
endif()
