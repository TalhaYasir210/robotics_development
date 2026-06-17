cmake_minimum_required(VERSION 3.8)
project(go_to_pose_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(turtlesim REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# add_executable(first_publisher src/first_publisher.cpp)
# ament_target_dependencies(first_publisher rclcpp geometry_msgs)

# install(TARGETS
#   first_publisher
#   DESTINATION lib/${PROJECT_NAME}
# )

# add_executable(pose_subscriber src/pose_subscriber.cpp)

# # Define the dependencies for the subscriber
# ament_target_dependencies(pose_subscriber rclcpp turtlesim)

# Tell ROS where to install the executable
# install(TARGETS
#   pose_subscriber
#   DESTINATION lib/${PROJECT_NAME}
# )

add_executable(controller_node src/controller_node.cpp)
ament_target_dependencies(controller_node rclcpp geometry_msgs turtlesim)

install(TARGETS
  controller_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
