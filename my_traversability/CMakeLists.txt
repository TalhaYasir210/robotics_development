cmake_minimum_required(VERSION 3.8)
project(my_traversability)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sanitizer options for memory leak & bounds profiling
option(ENABLE_SANITIZERS "Enable AddressSanitizer and LeakSanitizer" OFF)
if(ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address -fsanitize=leak -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address -fsanitize=leak)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(Eigen3 REQUIRED)

# 1. Core library (Separation of Concerns: Business logic + Node class)
add_library(my_traversability_core
  src/elevation_grid_map.cpp
  src/traversability_node.cpp
)

target_include_directories(my_traversability_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(my_traversability_core
  rclcpp
  sensor_msgs
  nav_msgs
  geometry_msgs
  visualization_msgs
  tf2
  tf2_ros
  tf2_eigen
  Eigen3
)

# 2. Main Executable Entry Point
add_executable(traversability_node
  src/main.cpp
)

target_link_libraries(traversability_node
  my_traversability_core
)

ament_target_dependencies(traversability_node
  rclcpp
)

# 3. Testing Configuration
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_cmake_gtest REQUIRED)

  # Cloud Filter test
  ament_add_gtest(test_cloud_filter test/test_cloud_filter.cpp)
  target_include_directories(test_cloud_filter PRIVATE include)
  target_link_libraries(test_cloud_filter Eigen3::Eigen)

  # Elevation Grid Map test
  ament_add_gtest(test_elevation_grid_map test/test_elevation_grid_map.cpp)
  target_include_directories(test_elevation_grid_map PRIVATE include)
  target_link_libraries(test_elevation_grid_map my_traversability_core)

  # Traversability Node test (Phase 4 / Issue 4)
  ament_add_gtest(test_traversability_node test/test_traversability_node.cpp)
  target_include_directories(test_traversability_node PRIVATE include)
  target_link_libraries(test_traversability_node my_traversability_core)

  # Latency & CPU Profiling Benchmark (Issue 10)
  ament_add_gtest(benchmark_traversability test/benchmark_traversability.cpp)
  target_include_directories(benchmark_traversability PRIVATE include)
  target_link_libraries(benchmark_traversability my_traversability_core)
endif()

# 4. Install rules for ROS 2 deployment
install(TARGETS
  my_traversability_core
  traversability_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch worlds models rviz
  DESTINATION share/${PROJECT_NAME}
)

# Export library for external downstream nodes
ament_export_include_directories(include)
ament_export_libraries(my_traversability_core)
ament_package()
