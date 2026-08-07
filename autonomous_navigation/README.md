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

## How to run through GUI interface
open terminal and run the follwing command
```bash
cd ~/ros2_ws

```
```bash
source install/setup.bash

```

```bash
python3 "src/robotics_development/autonomous_navigation/GUI Design/main_window.py"

```


### Saving and Loading SLAM Checkpoints (Resume Mapping)

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

For example, to load the checkpoints:
```yaml
    # --- ENVIRONMENT SPECIFIC PARAMS ---
    # [OFFICE] Office SLAM Checkpoint
    # map_file_name: "/home/deadsec/ros2_ws/src/robotics_development/autonomous_navigation/maps/office_checkpoint_1"
    # map_start_pose: [0.0, 0.0, 0.0]
    
    # [WAREHOUSE] Warehouse SLAM Checkpoint
    # map_file_name: "/home/deadsec/ros2_ws/src/robotics_development/autonomous_navigation/maps/warehouse_checkpoint_1"
    # map_start_pose: [0.0, 1.5, 0.0]
    # -----------------------------------
```

*Note: Make sure to re-comment these lines (add a `#` back in front of them) when you want to start a brand new map!*
4. Rebuild your workspace with `colcon build --packages-select autonomous_navigation`.
5. Launch `auto_slam` again! The robot will load the old map, and `explore_lite` will automatically see the remaining frontiers and resume exploring! 
*(Note: To start a brand-new map, simply comment out the `map_file_name` line in the YAML file).*

**Save the Generated Map:**
   Once the robot stops moving and the map is complete, open a new terminal, source the workspace, and run our helper script:
   ```bash
   cd ~/ros2_ws/src/robotics_development/autonomous_navigation
   ./scripts/save_map.sh my_new_map
   ```
   This will save the `.pgm` and `.yaml` map files into the `maps/` directory.

