cmake_minimum_required(VERSION 3.8)
project(my_traversability)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(Eigen3 REQUIRED)

# 1. Compile the core C++ logic into a library
add_library(my_traversability_core
  src/elevation_grid_map.cpp
)

target_include_directories(my_traversability_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(my_traversability_core
  Eigen3
)

# 2. Testing Configuration
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_cmake_gtest REQUIRED)

  # Cloud Filter test (Phase 2)
  ament_add_gtest(test_cloud_filter test/test_cloud_filter.cpp)
  target_include_directories(test_cloud_filter PRIVATE include)
  target_link_libraries(test_cloud_filter Eigen3::Eigen)

  # Elevation Grid Map test (Phase 3)
  ament_add_gtest(test_elevation_grid_map test/test_elevation_grid_map.cpp)
  target_include_directories(test_elevation_grid_map PRIVATE include)
  target_link_libraries(test_elevation_grid_map my_traversability_core)
endif()

# Export library so other packages or nodes can link it
ament_export_include_directories(include)
ament_export_libraries(my_traversability_core)
ament_package()