cmake_minimum_required(VERSION 3.8)
project(go_to_pose_pkg)

# Default to C++14 or C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(turtlesim REQUIRED)

# Define executables and their dependencies
add_executable(first_publisher src/first_publisher.cpp)
ament_target_dependencies(first_publisher rclcpp geometry_msgs)

add_executable(pose_subscriber src/pose_subscriber.cpp)
ament_target_dependencies(pose_subscriber rclcpp turtlesim)

add_executable(controller_node src/controller_node.cpp)
ament_target_dependencies(controller_node rclcpp geometry_msgs turtlesim)

# Install all executables in one block
install(TARGETS
  first_publisher
  pose_subscriber
  controller_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()