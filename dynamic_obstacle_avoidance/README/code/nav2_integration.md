# Nav2 Integration with LQR Velocity Controller

This guide explains how to run the `dynamic_obstacle_avoidance` package using the **Nav2 Custom Global Planner** integration. In this setup, Nav2 acts strictly as the "brain" (path planner) using your custom A* or RRT plugin, while your custom **LQR Velocity Controller** remains the absolute only node driving the robot.

## Prerequisites
1. Build your workspace:
   ```bash
   cd ~/ros2_ws
   colcon build --packages-select dynamic_obstacle_avoidance
   ```
2. Source your workspace in every new terminal:
   ```bash
   source ~/ros2_ws/install/setup.bash
   ```
3. Export your TurtleBot3 model (add this to your `~/.bashrc` to avoid doing it every time):
   ```bash
   export TURTLEBOT3_MODEL=waffle  # (or burger)
   ```

---

## Execution Steps

You will need to open **6 separate terminals** to launch the complete environment.

### Terminal 1: Launch Gazebo Simulation
Start the Gazebo world. The robot will physically spawn at `[-2.0, -0.5]` between the pillars.
```bash
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

### Terminal 2: Start the Map Server
Load the static map of the world so Nav2 knows where the walls are. 
```bash
ros2 run nav2_map_server map_server --ros-args -p yaml_filename:=/home/deadsec/ros2_ws/src/turtlebot3/turtlebot3_navigation2/map/map.yaml -p use_sim_time:=true
```
*(Note: If the map does not appear immediately in RViz later, you may need to activate the lifecycle node in another terminal by running: `ros2 run nav2_util lifecycle_bringup map_server`)*

### Terminal 3: Publish the Map-to-Odom Offset
Because the robot spawns at `[-2.0, -0.5]` but its odometry starts counting from `[0.0, 0.0]`, we must tell RViz and Nav2 about this offset so the map aligns perfectly with the robot's odometry.
```bash
ros2 run tf2_ros static_transform_publisher -2.0 -0.5 0 0 0 0 map odom
```

### Terminal 4: Launch Nav2 (Planner Only Wrapper)
We use a custom launch file that boots up Nav2 with our custom `nav2_params.yaml`, but explicitly intercepts and disables Nav2's movement commands. Nav2 will calculate the path but will never touch the wheels.
```bash
ros2 launch dynamic_obstacle_avoidance planner_only.launch.py
```

### Terminal 5: Start the Velocity Controller
Run your dedicated LQR path follower node. It will sit in `IDLE` state and passively wait for Nav2 to publish a path to the `/plan` topic.
```bash
ros2 run dynamic_obstacle_avoidance velocity_controller_node --ros-args -p use_sim_time:=true
```

### Terminal 6: Launch RViz2 for Visualization
Launch RViz2 to see the map, the Nav2 planned path, and command the robot.
```bash
rviz2
```
**In RViz2, make sure to add the following displays:**
1. **Map**: Set topic to `/map`
2. **Path**: Set topic to `/plan`. (Recommended: Set *Line Style* to *Billboards*, *Line Width* to `0.04`, and *Color* to bright Green).
3. **2D Goal Pose Tool**: Use the tool at the top of RViz to click your desired destination!

*(Note: You might see a "ghost" robot stuck at `[0,0]` in Gazebo/RViz. You can safely ignore or delete it from the Gazebo Entity tree; your real robot is the one at `[-2.0, -0.5]`.)*

---

## How to Switch Planners (A* vs RRT)

By default, the `Nav2CustomPlanner` is configured to use your **A* Planner**. 
If you want to switch to **RRT**:
1. Open `src/nav2_custom_planner.cpp`.
2. Scroll down to the `createPlan()` function (around line 90).
3. Comment out the `AStarPlanner` block.
4. Uncomment the `RRTPlanner` block.
5. Re-run `colcon build` to compile the change.
