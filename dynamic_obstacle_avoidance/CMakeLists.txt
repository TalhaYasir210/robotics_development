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
find_package(visualization_msgs REQUIRED)

# 1. Tell the compiler where to find your .hpp blueprint files
include_directories(include)

# 2. Compile BOTH the ROS 2 node and the A* math engine into one program
add_executable(planner_node 
  src/planner_node.cpp 
  src/a_star_planner.cpp
)
ament_target_dependencies(planner_node rclcpp nav_msgs geometry_msgs tf2_ros tf2_geometry_msgs)

# 2.5. Compile the RRT Node
add_executable(node_for_rrt
  src/node_for_rrt.cpp
  src/rrt_planner.cpp
)
ament_target_dependencies(node_for_rrt rclcpp nav_msgs geometry_msgs tf2_ros tf2_geometry_msgs visualization_msgs)

# 3. Compile the mock publisher
add_executable(mock_map_publisher src/mock_map_publisher.cpp)
ament_target_dependencies(mock_map_publisher rclcpp nav_msgs geometry_msgs)

# 3.5. Compile the unified RRT Navigator
add_executable(rrt_navigator_node 
  src/rrt_navigator_node.cpp
  src/rrt_planner.cpp
)
ament_target_dependencies(rrt_navigator_node rclcpp nav_msgs geometry_msgs tf2 tf2_geometry_msgs visualization_msgs)

# 4. Install them so ROS 2 can run them
install(TARGETS
  planner_node
  node_for_rrt
  rrt_navigator_node
  mock_map_publisher
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  
  ament_add_gtest(test_a_star_planner 
    test/test_a_star_planner.cpp
    src/a_star_planner.cpp
  )
  target_include_directories(test_a_star_planner PUBLIC include)

  ament_add_gtest(test_rrt_planner 
    test/test_rrt_planner.cpp
    src/rrt_planner.cpp
  )
  target_include_directories(test_rrt_planner PUBLIC include)
endif()

ament_package()