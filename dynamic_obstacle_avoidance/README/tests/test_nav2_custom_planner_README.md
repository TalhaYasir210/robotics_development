# Nav2 Custom Planner Unit Test Guide

This document explains how to run and understand the unit test for the `Nav2CustomPlanner` plugin.

## Overview
The `test_nav2_custom_planner.cpp` file contains a Google Test (`gtest`) to verify the basic initialization and structural integrity of the custom Nav2 global planner plugin. 

Because a full Nav2 `createPlan()` simulation requires spinning up complex ROS 2 Lifecycle Nodes, TF2 buffers, and Costmap2D grids, this test serves as a crucial "sanity check" to guarantee that:
1. The `Nav2CustomPlanner` class can be instantiated correctly.
2. The class inherits the Nav2 plugin interfaces properly without crashing (`EXPECT_NO_THROW`).
3. The plugin can safely be loaded into memory by `pluginlib`.

## How to Run the Test

To execute this specific unit test, navigate to your workspace and run:

```bash
cd ~/ros2_ws
colcon build --packages-select dynamic_obstacle_avoidance
colcon test --packages-select dynamic_obstacle_avoidance --ctest-args -R test_nav2_custom_planner
```

## Viewing Test Results

To see the detailed output and confirm that the test passed, run:

```bash
colcon test-result --all --verbose
```
