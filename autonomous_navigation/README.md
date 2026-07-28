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

## How to Run: Auto and Manual SLAM Mapping (Step 5)

We have created an automated SLAM mapping setup where the robot uses frontier exploration to automatically drive around and map an unknown environment. You can also choose to map manually.

1. **Launch the SLAM environment:**
   You can choose between the `office` or `warehouse` environment by passing the `env` argument = office or warehouse.
   ou can choose between the modes by just changing the mode = auto or manual in the cammnad prompt.
   
   To run Auto SLAM in the office environment (default):
   ```bash
   ros2 launch autonomous_navigation auto_slam.launch.py env:=office mode:=auto
   ```
   
   To run Manual SLAM in the warehouse environment:
   ```bash
   ros2 launch autonomous_navigation auto_slam.launch.py env:=warehouse mode:=manual
   ```
   *Note: change the name of the map office/warehouse which you want to load , In manual mode, the robot will not move on its own. You must open a new terminal and run a teleop node to drive it. Make sure to remap the topic so our stamper node can process it:*
   ```bash
   ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r cmd_vel:=cmd_vel_unstamped
   ```

2. **Watch the Exploration:**
   - Gazebo will launch with the selected environment.
   - RViz will automatically open. 
   - In `auto` mode, you will see the robot start driving towards the green/blue frontiers (unknown areas).
   - Wait until the robot finishes exploring the entire environment (or drive it manually until the map is complete).

3. **Save the Generated Map:**
   Once the robot stops moving and the map is complete, open a new terminal, source the workspace, and run our helper script:
   ```bash
   cd ~/ros2_ws/src/robotics_development/autonomous_navigation
   ./scripts/save_map.sh my_new_map
   ```
   This will save the `.pgm` and `.yaml` map files into the `maps/` directory.

### 4. Saving and Loading SLAM Checkpoints (Resume Mapping)

There is a major difference between saving a **Static Map** (for pure navigation later) and saving a **SLAM Checkpoint** (to resume exploring the same house tomorrow). The `map_saver_cli` command shown above saves a *Static Map*.

To pause and resume your SLAM exploration progress, use the **Serialization** feature in SLAM Toolbox:

**Saving a Checkpoint:**
1. While `auto_slam` is running, open the **RViz** window.
2. In the left panel, locate the **SlamToolboxPlugin** panel.
3. In the text box next to "Serialize Map", paste the absolute path to your maps folder and provide a checkpoint name, for example:
   `/home/deadsec/ros2_ws/src/robotics_development/autonomous_navigation/maps/office_checkpoint_1`
or 
   `/home/deadsec/ros2_ws/src/robotics_development/autonomous_navigation/maps/warehouse_checkpoint_1`
Note: you can chaneg the checkpoint name to anything you want (office_checkpoint_1) etc
4. Click the **Serialize Map** button. It will save a `.posegraph` and `.data` file.

**Loading a Checkpoint:**
By default, the robot starts with a fresh map. To have the robot automatically load your checkpoint the next time you launch `auto_slam.launch.py`:
1. Open the configuration file: `config/mapper_params_online_async.yaml`
2. Scroll to the `ros__parameters` section (around line 23).
3. Uncomment (remove the `#`) from the `map_file_name` and `map_start_pose` lines and set the path to your checkpoint.

For example, to load the **office** checkpoint:
```yaml
    map_file_name: "/home/deadsec/ros2_ws/src/robotics_development/autonomous_navigation/maps/office_checkpoint_1"
    map_start_pose: [0.0, 0.0, 0.0]
```

To load the **warehouse** checkpoint:
```yaml
    map_file_name: "/home/deadsec/ros2_ws/src/robotics_development/autonomous_navigation/maps/warehouse_checkpoint_1"
    map_start_pose: [0.0, 0.0, 0.0]
```

*Note: Make sure to re-comment these lines (add a `#` back in front of them) when you want to start a brand new map!*
4. Rebuild your workspace with `colcon build --packages-select autonomous_navigation`.
5. Launch `auto_slam` again! The robot will load the old map, and `explore_lite` will automatically see the remaining frontiers and resume exploring! 
*(Note: To start a brand-new map, simply comment out the `map_file_name` line in the YAML file).*

### 5. Fine-Tuning Environment Parameters

Different environments (like an open `office` vs a cluttered `warehouse`) sometimes require slightly different navigation parameters for optimal exploration.

For example, if the robot gets stuck in an infinite loop without moving, it usually means the `xy_goal_tolerance` is too large for a tight space.

We have set up environment-specific toggles in our main configuration file:
1. Open the configuration file: `config/nav2_params_slam.yaml`
2. Scroll to the `goal_checker` section (around line 118) or the `inflation_layer` sections (around lines 165 & 265).
3. Comment or uncomment the parameters based on the map you are running:
   ```yaml
      # --- ENVIRONMENT SPECIFIC PARAMS ---
      # [OFFICE] Use 0.25 if mapping the open office
      # xy_goal_tolerance: 0.25
      
      # [WAREHOUSE] Use 0.10 for the cluttered warehouse to prevent infinite loops
      xy_goal_tolerance: 0.10
      # -----------------------------------
   ```
   *Similarly for the Inflation Radius (controls how wide the "danger zone" around walls is):*
   ```yaml
      # --- ENVIRONMENT SPECIFIC PARAMS ---
      # [OFFICE] Use 0.35 for the open office
      # inflation_radius: 0.35
      
      # [WAREHOUSE] Use 0.15 for the cluttered warehouse corridors
      inflation_radius: 0.15
      # -----------------------------------
   ```
   *And for the Laser Blind Spot in `nav2_params_slam.yaml` (search for `raytrace_min_range` in the costmaps):*
   ```yaml
          # --- ENVIRONMENT SPECIFIC PARAMS ---
          # [OFFICE] Use 0.35 to ignore robot footprint
          # raytrace_min_range: 0.35
          # obstacle_min_range: 0.35
          
          # [WAREHOUSE] Use 0.12 so robot isn't blind to nearby boxes
          raytrace_min_range: 0.12
          obstacle_min_range: 0.12
          # -----------------------------------
   ```

4. You can also tune the **Frontier Size** in `config/explore.yaml` if the robot refuses to enter tight spaces:
   ```yaml
    # --- ENVIRONMENT SPECIFIC PARAMS ---
    # [OFFICE] Use 0.25 for large open spaces
    min_frontier_size: 0.25
    # -----------------------------------
   ```

5. Finally, if the robot hallucinates obstacles in front of itself when driving over bumps, tune the **Laser Ignorance Height** (search for `min_obstacle_height` in the costmaps):
   ```yaml
          # --- ENVIRONMENT SPECIFIC PARAMS ---
          # [OFFICE] Use 0.10 as default for flat surfaces
          min_obstacle_height: 0.10
          # -----------------------------------
   ```

6. To prevent the robot from crashing or rolling over, ensure the `desired_linear_vel` and `regulated_linear_scaling_min_radius` in `FollowPath` are set to safe values:
   ```yaml
      # --- ENVIRONMENT SPECIFIC PARAMS ---
      # [OFFICE] Use 0.20 for safe navigation
      desired_linear_vel: 0.20
      # -----------------------------------
   ```

7. Rebuild your workspace with `colcon build --packages-select autonomous_navigation` after making any changes.