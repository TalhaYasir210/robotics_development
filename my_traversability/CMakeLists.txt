cmake_minimum_required(VERSION 3.8)
project(my_traversability)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(Eigen3 REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # ... other linter stuff if present ...
  # 1. Find the ROS 2 GoogleTest tool
  find_package(ament_cmake_gtest REQUIRED)
  # 2. Tell CMake to build an executable test called 'test_cloud_filter'
  ament_add_gtest(test_cloud_filter test/test_cloud_filter.cpp)
  # 3. Tell the test where to find your header file (in the 'include' folder)
  target_include_directories(test_cloud_filter PRIVATE include)
  # 4. Link Eigen to the test so it can do 3D math
  target_link_libraries(test_cloud_filter Eigen3::Eigen)
endif()

ament_package()
