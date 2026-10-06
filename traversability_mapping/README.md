# (`traversability_mapping`)

A modular, production-grade 2.5D elevation and traversability mapping ROS 2 Jazzy C++ package designed for simulated and physical mobile robots (TurtleBot3 Waffle / Waffle Pi) equipped with an Intel RealSense depth camera.

<img width="19340" height="2820" alt="traversability depth camera" src="https://github.com/user-attachments/assets/2fa17119-c3fa-4219-bcd1-c21ee8e3545f" />


---

##  Installation & Build Instructions
> **Note:**  make a directory , change the directory and clone the repository
```bash
git clone https://github.com/TalhaYasir210/robotics_development.git
```
> **Note:**  after cloning change the branch
```bash
cd <robotics development>
git switch traversability_with_depth_camera
cd ..
```
> **Note:**  always build from the root of your ROS 2 workspace 

### 1. Install Dependencies
```bash

# Install rosdep dependencies
cd <root of workspace>
rosdep update
rosdep install --from-paths src --ignore-src -r -y

# (Optional) Install Python tooling dependencies
cd <robotics_development> 
pip install -r traversability_mapping/requirements.txt
```

### Build the Package
```bash
cd <root of workspace>
colcon build --packages-select traversability_mapping
```

### Source the Workspace
```bash
source install/setup.bash
```

---

## Launching & Visualization

### Option A: Complete Standalone Simulation (Headless Gazebo + RViz + RealSense Depth Camera)
*All robot models, meshes, URDF, and worlds are fully bundled inside this package (zero external TurtleBot3 dependency required).*
*Gazebo runs in headless mode (`-s -r`), saving GPU/CPU resources, while RViz provides a unified visualizer with fixed camera view and live camera feed.*

```bash
# terminal 1 Launch complete self-contained simulation pipeline 
export TURTLEBOT3_MODEL=waffle
ros2 launch traversability_mapping tb3_simulation.launch.py
```

```bash
# terminal 2 teleop keys
source install/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```






https://github.com/user-attachments/assets/54e46442-f8cc-40d2-9a81-44c98fcf9ac4





https://github.com/user-attachments/assets/dee9b5fd-6465-4045-943f-9002fcc825e2






### Option B: Standalone Node (with existing simulation or real robot)
```bash
ros2 launch traversability_mapping traversability.launch.py
```

### Option C: Standalone Executable with Custom Parameters
```bash
ros2 run traversability_mapping traversability_node --ros-args \
  -p resolution:=0.05 \
  -p grid_width:=4.0 \
  -p grid_length:=4.0 \
  -p safe_step_threshold:=0.06 \
  -p obstacle_threshold:=0.12
```
---
## Configuration Parameters (`config/params.yaml`)

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `resolution` | float | `0.05` | Grid cell size (5 cm per cell) |
| `grid_width` | float | `4.0` | Lateral coverage in meters (-2.0m to +2.0m) |
| `grid_length` | float | `4.0` | Forward coverage in meters (0.0m to +4.0m) |
| `safe_step_threshold` | float | `0.06` | Height delta $\le 6\text{cm}$ is classified as flat ground (cost 0) |
| `obstacle_threshold` | float | `0.12` | Height delta $\ge 12\text{cm}$ is non-traversable (cost 100) |
| `min_z_cutoff` | float | `-0.15` | Floor noise filter cutoff in `base_link` frame |
| `max_z_cutoff` | float | `1.50` | Ceiling / overhang filter cutoff in `base_link` frame |
| `base_frame` | string | `"base_link"` | Robot reference coordinate frame |
| `input_topic` | string | `"/camera/depth/points"` | Subscribed point cloud topic (Sensor QoS) |
| `output_topic` | string | `"/traversability_map"` | Published OccupancyGrid topic |



