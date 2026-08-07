# Autonomous Navigation Project

This package provides a standalone and well-optimized setup for autonomous navigation and mapping in ROS 2. It utilizes custom Gazebo environments, SLAM Toolbox, Nav2, and frontier exploration.

## Project Setup & Build
## 1. Clone the Repository
First, set up your ROS 2 workspace and clone the repository directly into the `src` folder. Make sure to specify the branch you want to work on:

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone -b feature/gui --single-branch https://github.com/TalhaYasir210/robotics_development.git

```

## 2. To build the workspace:

```bash

cd ~/ros2_ws

# Update rosdep and install all required system dependencies
rosdep update
rosdep install --from-paths src --ignore-src -r -y

# Build only the autonomous_navigation package
colcon build --packages-select autonomous_navigation

# Source the workspace setup
source install/setup.bash

```

## How to run through GUI interface
open terminal and run the follwing command
```bash

pip install PyQt5

```

```bash
cd ~/ros2_ws
source install/setup.bash
python3 "src/robotics_development/autonomous_navigation/GUI Design/main_window.py"

```

## Troubleshooting: Virtual Environment Build Errors

### Issue 1: The 'em' Module Error

```bash

pip install empy==3.3.4

```
### Issue 2: The 'catkin_pkg' Error

```bash
pip install catkin_pkg

```

## The Ultimate Fix (The "Glass Wall" Environment)

```bash


#1. Deactivate your current environment
deactivate

# 2. Delete the old environment folder
rm -rf /path/to/your/venv

# 3. Create a new environment using the system packages flag
python3 -m venv --system-site-packages /path/to/your/venv

# 4. Activate the new environment
source /path/to/your/venv/bin/activate

# 5. Run the build again
cd ~/ros2_ws
colcon build --packages-select autonomous_navigation

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

