cmake_minimum_required(VERSION 3.8)
project(dynamic_obstacle_avoidance)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

# 1. Tell the compiler where to find your .hpp blueprint files
include_directories(include)

# 2. Compile BOTH the ROS 2 node and the A* math engine into one program
add_executable(planner_node 
  src/planner_node.cpp 
  src/a_star_planner.cpp
)
ament_target_dependencies(planner_node rclcpp nav_msgs geometry_msgs tf2_ros tf2_geometry_msgs)

# 3. Compile the mock publisher
add_executable(mock_map_publisher src/mock_map_publisher.cpp)
ament_target_dependencies(mock_map_publisher rclcpp nav_msgs geometry_msgs)

# 4. Install them so ROS 2 can run them
install(TARGETS
  planner_node
  mock_map_publisher
  DESTINATION lib/${PROJECT_NAME}
)

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

ament_package()