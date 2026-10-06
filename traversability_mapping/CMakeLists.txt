# ==============================================================================
# CMake Build Configuration for traversability_mapping package
# ==============================================================================
# Configures C++17 build targets:
# - traversability_core: Pure C++ static library (ElevationGridMap & CloudFilter)
# - traversability_node: ROS 2 component shared library
# - traversability_mapping_node: Standalone executable
# - GTest unit tests for core algorithms
# Installs binaries, configs, models, URDF, launch files, and RViz presets.
# ==============================================================================

cmake_minimum_required(VERSION 3.8)
project(traversability_mapping)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ROS 2 and third-party dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(eigen3_cmake_module REQUIRED)
find_package(Eigen3 REQUIRED)

# ==============================================================================
# 1. Core Engine: Pure C++ Elevation & Traversability Library (No ROS 2 deps)
# ==============================================================================
add_library(traversability_core
  src/elevation_grid_map.cpp
  src/cloud_filter.cpp
)

target_include_directories(traversability_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(traversability_core
  Eigen3::Eigen
)

# ==============================================================================
# 2. ROS 2 Adapter: Component Shared Library
# ==============================================================================
add_library(traversability_node_component SHARED
  src/traversability_node.cpp
)

target_include_directories(traversability_node_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(traversability_node_component
  traversability_core
)

ament_target_dependencies(traversability_node_component
  rclcpp
  rclcpp_components
  sensor_msgs
  nav_msgs
  geometry_msgs
  tf2
  tf2_ros
  tf2_eigen
  Eigen3
)

# Register component for dynamic loading
rclcpp_components_register_node(traversability_node_component
  PLUGIN "traversability_mapping::TraversabilityNode"
  EXECUTABLE traversability_node_component_exe
)

# ==============================================================================
# 3. Standalone Executable Entrypoint
# ==============================================================================
add_executable(traversability_node
  src/main.cpp
)

target_link_libraries(traversability_node
  traversability_node_component
)

ament_target_dependencies(traversability_node
  rclcpp
)

# ==============================================================================
# 4. Unit Testing (Separation of Core Logic allows pure C++ tests)
# ==============================================================================
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)

  # Configure linters to allow strict GTest execution
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_uncrustify_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  ament_add_gtest(test_elevation_grid_map test/test_elevation_grid_map.cpp)
  target_link_libraries(test_elevation_grid_map traversability_core)

  ament_add_gtest(test_cloud_filter test/test_cloud_filter.cpp)
  target_link_libraries(test_cloud_filter traversability_core)
endif()

# ==============================================================================
# 5. Install Targets & Assets
# ==============================================================================
install(TARGETS
  traversability_core
  traversability_node_component
  traversability_node
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config launch rviz models worlds urdf
  DESTINATION share/${PROJECT_NAME}
)

# Export interfaces for external package composition
ament_export_include_directories(include)
ament_export_libraries(traversability_core traversability_node_component)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  rclcpp_components
  sensor_msgs
  nav_msgs
  geometry_msgs
  tf2
  tf2_ros
  tf2_eigen
  Eigen3
)

ament_package()
