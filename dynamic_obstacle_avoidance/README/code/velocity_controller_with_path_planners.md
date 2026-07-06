# Velocity Controller with Path Planners

This document provides a complete, step-by-step guide to setting up and running the unified **Velocity Controller Node** (which currently uses the RRT Path Planner) in Gazebo and RViz. 

This node acts as a standalone manager: it receives the map, reads the robot's odometry, uses a path planner (like RRT or A*) to find a safe route, and mathematically calculates the exact motor velocities (`/cmd_vel`) needed to drive the robot smoothly to the goal.

## Prerequisites and Setup
1. Ensure your ROS 2 Workspace is sourced in every new terminal you open:
   ```bash
   source /opt/ros/jazzy/setup.bash
   source ~/ros2_ws/install/setup.bash
   ```
2. Make sure you have compiled the package recently:
   ```bash
   cd ~/ros2_ws
   colcon build --packages-select dynamic_obstacle_avoidance
   ```
3. Export your TurtleBot3 model (add this to your `~/.bashrc` to avoid doing it every time):
   ```bash
   export TURTLEBOT3_MODEL=waffle_pi  # (or burger)
   ```

---

## Execution Steps

You will need to open **5 separate terminals** to launch the complete environment.

### Terminal 1: Launch Gazebo Simulation
Start the Gazebo world. The robot will spawn physically at `[-2.0, -0.5]` between the pillars.
```bash
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

### Terminal 2: Start the Map Server
Load the static map of the world. 
```bash
ros2 run nav2_map_server map_server --ros-args -p yaml_filename:=/home/deadsec/ros2_ws/src/turtlebot3/turtlebot3_navigation2/map/map.yaml -p use_sim_time:=true
```
*(Note: If the map does not appear immediately, you may need to activate the lifecycle node in another terminal by running: `ros2 run nav2_util lifecycle_bringup map_server`)*

### Terminal 3: Publish the Map-to-Odom Offset
Because the robot spawns at `[-2.0, -0.5]` but its odometry starts counting from `[0.0, 0.0]`, we must tell RViz about this offset so the map aligns perfectly with the robot's odometry.
```bash
ros2 run tf2_ros static_transform_publisher -2.0 -0.5 0 0 0 0 map odom
```

### Terminal 4: Launch RViz2 for Visualization
Launch RViz2 to see the map, the planned path, and the RRT exploration tree.
```bash
rviz2
```
**In RViz2, make sure to add the following displays:**
1. **Map**: Set topic to `/map`
2. **Path (Green)**: Set topic to `/planned_path`. (Recommended: Set *Line Style* to *Billboards*, *Line Width* to `0.04`, and *Color* to bright Green).
3. **Path (Grey/Tree)**: Set topic to `/rrt_tree`. (Recommended: Set *Line Style* to *Billboards*, *Line Width* to `0.01`, and *Color* to dark grey).
4. **2D Goal Pose Tool**: Use the tool at the top of RViz to click your desired destination!

### Terminal 5: Start the Velocity Controller
Finally, run our unified velocity controller. It will wait in `IDLE` state until you click a goal in RViz.
```bash
ros2 run dynamic_obstacle_avoidance velocity_controller_node --ros-args -p use_sim_time:=true
```

---

## How it Works
1. Once all terminals are running, use the **2D Goal Pose** tool in RViz to click a destination.
2. The Velocity Controller node receives the goal and passes it to the RRT Planner.
3. The RRT Planner natively inflates the walls by 20cm to ensure the robot doesn't crash, and calculates a safe path.
4. The node prints the coordinate table in your terminal and immediately begins publishing `TwistStamped` messages to `/cmd_vel`.
5. The robot will drive smoothly until it is within **5cm (0.05m)** of the final target coordinate, at which point it will stop and print **"Goal Reached!"**.
