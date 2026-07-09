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
find_package(Eigen3 REQUIRED)
find_package(nav2_core REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(pluginlib REQUIRED)

# 1. Tell the compiler where to find your .hpp blueprint files
include_directories(
  include
  ${EIGEN3_INCLUDE_DIRS}
)

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

# 3.5. Compile the unified Navigator
add_executable(velocity_controller_node 
  src/velocity_controller_node.cpp
  # Uncomment the two lines below if you want to directly use A* or RRT internally instead of Nav2 subscription
  # src/a_star_planner.cpp
  # src/rrt_planner.cpp
  src/lqr_controller.cpp
)
ament_target_dependencies(velocity_controller_node rclcpp nav_msgs geometry_msgs visualization_msgs tf2 tf2_geometry_msgs)
target_link_libraries(velocity_controller_node Eigen3::Eigen)

add_executable(passive_tracking_logger_node 
  src/passive_tracking_logger_node.cpp
)
ament_target_dependencies(passive_tracking_logger_node rclcpp nav_msgs geometry_msgs visualization_msgs tf2 tf2_geometry_msgs)
target_link_libraries(passive_tracking_logger_node Eigen3::Eigen)

add_executable(dynamic_obstacle_detector_node
  src/dynamic_obstacle_detector_node.cpp
)
ament_target_dependencies(dynamic_obstacle_detector_node rclcpp nav_msgs geometry_msgs sensor_msgs)

# 4. Install them so ROS 2 can run them
install(TARGETS
  planner_node
  node_for_rrt
  velocity_controller_node
  passive_tracking_logger_node
  dynamic_obstacle_detector_node
  mock_map_publisher
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY 
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

# 5. Compile and install the Nav2 Plugin Library
add_library(nav2_custom_planner SHARED
  src/nav2_custom_planner.cpp
  src/a_star_planner.cpp
  src/rrt_planner.cpp
)
ament_target_dependencies(nav2_custom_planner
  rclcpp
  nav_msgs
  geometry_msgs
  nav2_core
  nav2_costmap_2d
  pluginlib
)
target_link_libraries(nav2_custom_planner Eigen3::Eigen)

install(TARGETS nav2_custom_planner
  DESTINATION lib
)

install(FILES global_planner_plugin.xml
  DESTINATION share/${PROJECT_NAME}
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
  
  ament_add_gtest(test_lqr_controller 
    test/test_lqr_controller.cpp
    src/lqr_controller.cpp
  )
  target_include_directories(test_lqr_controller PUBLIC include)
  target_link_libraries(test_lqr_controller Eigen3::Eigen)

  ament_add_gtest(test_nav2_custom_planner 
    test/test_nav2_custom_planner.cpp
    src/nav2_custom_planner.cpp
    src/a_star_planner.cpp
    src/rrt_planner.cpp
  )
  target_include_directories(test_nav2_custom_planner PUBLIC include)
  ament_target_dependencies(test_nav2_custom_planner rclcpp nav_msgs geometry_msgs nav2_core nav2_costmap_2d pluginlib)
  target_link_libraries(test_nav2_custom_planner Eigen3::Eigen)
endif()

pluginlib_export_plugin_description_file(nav2_core global_planner_plugin.xml)

ament_package()