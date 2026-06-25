cmake_minimum_required(VERSION 3.8)
project(obstacle_avoidance)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- find dependencies ---
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
# 🚨 ADDED std_msgs
find_package(std_msgs REQUIRED)

# --- Tell CMake where headers are ---
include_directories(include)

# --- Node: Obstacle Avoidance Node ---
add_executable(obstacle_avoidance_node 
  src/obstacle_avoidance_node.cpp
  src/sensor_processor.cpp
  src/autonomy_fsm.cpp
  src/motion_controller.cpp
)

# 🚨 ADDED std_msgs to the target dependencies
ament_target_dependencies(obstacle_avoidance_node 
  rclcpp 
  sensor_msgs 
  geometry_msgs 
  nav_msgs 
  std_msgs
)

# --- Install Rules ---
install(TARGETS
  obstacle_avoidance_node
  DESTINATION lib/${PROJECT_NAME}
)

# --- Testing ---
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()