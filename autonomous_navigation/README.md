# Autonomous Navigation Project

This package provides a standalone and well-optimized setup for autonomous navigation and mapping in ROS 2. It utilizes custom Gazebo environments, SLAM Toolbox, Nav2, and frontier exploration.

## Project Setup & Build

This project is completely self-contained. The exploration package (`m-explore-ros2`) is included in the `robotics_development/extras` directory so it builds automatically alongside our custom package.

To build the workspace:
```bash
# Navigate to your workspace root
cd ~/ros2_ws

# Build the packages
colcon build

# Source the workspace setup
source install/setup.bash
```

## How to Run: Auto SLAM Mapping (Step 5)

We have created an automated SLAM mapping setup where the robot uses frontier exploration to automatically drive around and map an unknown environment. 

1. **Launch the Auto SLAM environment:**
   You can choose between the `office` or `warehouse` environment by passing the `env` argument.
   
   To run in the office environment (default):
   ```bash
   ros2 launch autonomous_navigation auto_slam.launch.py env:=office
   ```
   
   To run in the warehouse environment:
   ```bash
   ros2 launch autonomous_navigation auto_slam.launch.py env:=warehouse
   ```

2. **Watch the Exploration:**
   - Gazebo will launch with the selected environment.
   - RViz will automatically open. You will see the robot start driving towards the green/blue frontiers (unknown areas).
   - Wait until the robot finishes exploring the entire environment.

3. **Save the Generated Map:**
   Once the robot stops moving and the map is complete, open a new terminal, source the workspace, and run our helper script:
   ```bash
   cd ~/ros2_ws/src/robotics_development/autonomous_navigation
   ./scripts/save_map.sh my_new_map
   ```
   This will save the `.pgm` and `.yaml` map files into the `maps/` directory.
