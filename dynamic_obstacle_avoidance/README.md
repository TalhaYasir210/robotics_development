# Dynamic Obstacle Avoidance & Nav2 Integration

## About This Repository
This package provides a **Nav2 Custom Global Planner** integration for the TurtleBot3 within a Gazebo simulation. In this setup, Nav2 acts as both the "brain" (path planner) using a custom **A*** or **RRT** plugin, and the local controller (using a tightly-tuned DWB local planner) combined with a custom dynamic obstacle detector to enable true **dynamic obstacle avoidance**.

### Prerequisites
- ROS 2 (Jazzy)
- Gazebo Simulator & TurtleBot3 Packages
- Navigation2 (Nav2) Stack
- colcon build tools

## Expected Output

- **Phase 1: Global Path Planning**
  Upon receiving a 2D Goal Pose in RViz, the custom `Nav2CustomPlanner` invokes either the A* or RRT algorithm to calculate an optimal global path avoiding static obstacles known to the map server.
  
- **Phase 2: Local Tracking & Dynamic Avoidance**
  The tightly-tuned DWB local planner strictly follows the global path. Simultaneously, the `dynamic_obstacle_detector_node` clusters incoming LiDAR data to track moving objects. If a dynamic obstacle enters the critical danger zone at a high relative velocity, the robot will intelligently brake or aggressively steer away.
  
- **Phase 3: Real-Time Logging**
  The `passive_tracking_logger_node` silently observes the robot's progress, continuously outputting a real-time console log comparing your robot's path-following distance errors, heading errors, and commanded velocities.

## Architecture Overview (How it Works)

To make the robot intelligent and modular, the code utilizes the standard ROS 2 Nav2 stack alongside custom C++ plugins and nodes. Here is a breakdown of how they work together:

1. **The Global Brain (`nav2_custom_planner.cpp` & Planners)**
   This component integrates directly into Nav2 as a Global Planner plugin. It accesses the costmap and uses either an `AStarPlanner` or `RRTPlanner` to calculate the shortest collision-free path to the destination.

2. **The Gatekeeper (`dynamic_obstacle_detector_node.cpp`)**
   This standalone node acts as an emergency override. It subscribes to raw laser scans, groups points into clusters using Euclidean clustering, and calculates absolute velocities to identify *moving* obstacles. If an obstacle is on a collision course in the "Danger Zone", it intercepts velocity commands to trigger an emergency brake.

3. **The Local Muscle (DWB Local Planner)**
   Managed through Nav2 parameters, the standard DWB controller calculates the exact physical wheel speeds needed to smoothly follow the global path and dodge unmapped obstacles on the fly.

4. **The Observer (`passive_tracking_logger_node.cpp`)**
   A dedicated monitoring node that passively compares the robot's current odometry against the Nav2 planned path. It logs the total travel time, distance remaining, and path deviation in real-time.

---

## 🔀 How to Switch Planners (A* vs RRT)

By default, the `Nav2CustomPlanner` is configured to use your **A* Planner**. 
If you want to switch to **RRT**:
1. Open `src/nav2_custom_planner.cpp`.
2. Scroll down to the `createPlan()` function (around line 90).
3. Comment out the `AStarPlanner` block.
4. Uncomment the `RRTPlanner` block.
5. Re-run `colcon build` to compile the change.

---

## How to Run

> [!NOTE]
> If you are experiencing ghost processes, map linking errors, or your robot won't spawn properly, refer to the Troubleshooting section at the bottom to clean up lingering processes before launching.

## 🚀 Execution Steps
*(Follow these commands exactly to set up and run the complete environment)*

We have simplified the launch process into a single bringup file. You will only need **2 terminals**: one to launch the entire simulation and Nav2 stack, and an optional second terminal if you want to manually drive the dynamic obstacle bot.

### Terminal 1: Workspace Setup & Launch Everything
Open a terminal (`Ctrl+Alt+T`), build the workspace, and launch the single bringup file. This will automatically start Gazebo, the map server, Nav2, our custom nodes, and RViz.
```bash
# Compile the package
cd ~/ros2_ws
colcon build --packages-select dynamic_obstacle_avoidance

# Source your workspace
source ~/ros2_ws/install/setup.bash

# Export your TurtleBot3 model
export TURTLEBOT3_MODEL=waffle

# Launch the entire simulation environment
ros2 launch dynamic_obstacle_avoidance bringup.launch.py
```

**In RViz2, make sure to add the following displays (if not automatically loaded):**
1. **Map**: Set topic to `/map`
2. **Path**: Set topic to `/plan`. (Recommended: Set *Line Style* to *Billboards*, *Line Width* to `0.04`, and *Color* to bright Green).
3. **2D Goal Pose Tool**: Use the tool at the top of RViz to click your desired destination! Watch the robot calculate the global path and navigate.

*(Note: You might see a "ghost" robot stuck at `[0,0]` in Gazebo/RViz. You can safely ignore or delete it from the Gazebo Entity tree; your real robot is the one at `[-2.0, -0.5]`.)*

### Terminal 2: Control the Dynamic Obstacle Bot (Optional)
The `bringup.launch.py` file also spawned a secondary obstacle bot in Gazebo. You can manually drive this bot around using your keyboard to test the main bot's dynamic obstacle avoidance.
```bash
source ~/ros2_ws/install/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args --remap cmd_vel:=/obstacle_bot/cmd_vel
```

---

## 🧹 Gazebo & ROS 2 Clean-Up (Troubleshooting)

> [!NOTE]
> If you are experiencing ghost processes, map linking errors, or your robot won't spawn properly, use these commands to completely reset the Gazebo and ROS 2 communication daemons before launching:

```bash
# Kill any lingering Gazebo processes
killall -9 gzserver gzclient
killall -9 gz ruby

# Stop the hidden ROS 2 communication daemon
ros2 daemon stop

# Start a fresh, clean daemon
ros2 daemon start
```

---

## 🛑 Safety Failsafes & Notes

- **Dynamic Braking:** The `dynamic_obstacle_detector_node` uses Euclidean clustering to calculate the absolute velocity of surrounding objects. If an object is moving towards the robot at a high speed (`> 0.15 m/s`) within the configured danger zone, it overrides the Nav2 local planner and safely brings the robot to a halt.
- **Nav2 Failsafes:** If the local planner is completely blocked, Nav2's built-in recovery behaviors (like clearing costmaps or spinning) will engage to try and find a new route.
