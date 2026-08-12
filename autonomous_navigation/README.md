# Autonomous Navigation Project

This package provides a standalone and well-optimized setup for autonomous navigation and mapping in ROS 2. It utilizes custom Gazebo environments, SLAM Toolbox, Nav2, and frontier exploration.

## Prerequisites

    ROS 2 (Jazzy)
    Gazebo Simulator & TurtleBot3 Packages
    Navigation2 (Nav2) Stack
    colcon build tools


## Project Setup & Build
## 1. Clone the Repository
First, set up your ROS 2 workspace and clone the repository directly into the `src` folder. Make sure to specify the branch you want to work on:

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone https://github.com/TalhaYasir210/robotics_development.git

```

```bash
#after cloning 
cd /robotics_development
git pull
git submodule update --init --recursive
```

## 2. To build and run the workspace:

```bash

apt-get update
apt-get install python3-venv ros-jazzy-turtlebot3 ros-jazzy-turtlebot3-gazebo -y

python3 -m venv ros_venv
source /opt/ros_venv/bin/activate

pip install -r /ros2_ws/src/robotics_development/requirements.txt

cd /ros2_ws
rosdep update
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-up-to autonomous_navigation explore_lite explore_lite_msgs multirobot_map_merge


source install/setup.bash
python3 "src/robotics_development/autonomous_navigation/GUI Design/main_window.py"

```

## 3. Project Preview:
<img width="1080" height="1080" alt="Screenshot from 2026-08-12 10-50-57" src="https://github.com/user-attachments/assets/135e770f-1b32-47c0-9212-fde86a3c26d3" />
<img width="1080" height="1080" alt="Screenshot from 2026-08-12 10-51-28" src="https://github.com/user-attachments/assets/c21deab2-b111-44bc-99fa-4ecdb136e69c" />
<img width="2126" height="1164" alt="Screenshot from 2026-08-12 10-52-08" src="https://github.com/user-attachments/assets/509657f8-d076-4e93-9dd8-de2c508f90e5" />
<img width="2126" height="1164" alt="Screenshot from 2026-08-12 10-53-38" src="https://github.com/user-attachments/assets/f1d65ea5-2aa9-4afc-ba91-21be5151db73" />





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

