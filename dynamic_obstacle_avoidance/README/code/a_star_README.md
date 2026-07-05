# Dynamic Obstacle Avoidance (A* Path Planner)

This package contains the A* path planning engine and mock nodes for dynamic obstacle avoidance in ROS 2.

All commands assume you are currently in the root of your ROS 2 workspace (`/home/deadsec/ros2_ws`).

---

## Part 1: Running the Project

To run the project, you will need to open **three separate terminals** to launch the map, the planner, and RViz2.

### Step 1: Build and Source
In **every** new terminal you open, make sure you build (if you haven't already) and source the workspace so ROS knows where your nodes are:
```bash
colcon build --packages-select dynamic_obstacle_avoidance
source install/setup.bash
```

### Step 2: Run the Mock Map Publisher (Terminal 1)
This node simulates a mapping algorithm (like SLAM) by broadcasting a fake 2D map with an obstacle for the robot to navigate around.
```bash
ros2 run dynamic_obstacle_avoidance mock_map_publisher
```

### Step 3: Run the A* Planner Node (Terminal 2)
This node listens to the map, waits for you to set a "Nav2 Goal", computes the A* path around the obstacles, and broadcasts the path back out.
```bash
ros2 run dynamic_obstacle_avoidance planner_node
```
*(Optional) If you want to see detailed algorithmic performance metrics (like how many microseconds each A* step took), you can enable the internal debug mode:*
```bash
ros2 run dynamic_obstacle_avoidance planner_node --ros-args -p enable_planner_debug:=true
```

### Step 4: Open RViz2 (Terminal 3)
RViz2 is the graphical interface to view the map and the path.
```bash
rviz2
```
*Note: In RViz2, make sure to add the `Map` display (listening to the `/map` topic) and the `Path` display (listening to the `/planned_path` topic).*
*To tell the robot where to go, click the **"2D Goal Pose"** button at the top of RViz2 and click anywhere on the free space in the map!*

---

## Part 2: Testing the Package

We have written an extensive 56-test gauntlet for the A* engine.

### Running Tests via Colcon (Standard)
The standard ROS 2 way to run tests is using the `colcon test` verb.
```bash
# Run tests for this package
colcon test --packages-select dynamic_obstacle_avoidance

# Summarize the test XML results to see passing/failing numbers
colcon test-result --all
```

If you want Colcon to print the live test progress directly to your terminal instead of hiding it:
```bash
colcon test --packages-select dynamic_obstacle_avoidance --event-handlers console_direct+
```

### Running the Executable Directly (Recommended for fast debugging)
For immediate feedback during development, it is often easier to run the compiled Google Test executable directly. This bypasses Colcon's wrappers and prints the familiar green/red `[ OK ]` and `[ FAILED ]` lines instantly.
```bash
# Run the test executable directly
./build/dynamic_obstacle_avoidance/test_a_star_planner
```

### Clearing "Ghost" Failures
If you remove tests or fix linters but `colcon test-result` still shows old failures, it is because Colcon keeps old XML log files. To clear the history and start fresh, simply delete the old results folder:
```bash
rm -rf build/dynamic_obstacle_avoidance/test_results/
```
