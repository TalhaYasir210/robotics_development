# (`traversability_mapping`)

A modular, production-grade 2.5D elevation and traversability mapping ROS 2 Jazzy C++ package designed for simulated and physical mobile robots (TurtleBot3 Waffle / Waffle Pi) equipped with an Intel RealSense depth camera.

---

##  Installation & Build Instructions

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
  -p safe_step_threshold:=0.04 \
  -p obstacle_threshold:=0.12
```
---
## Configuration Parameters (`config/params.yaml`)

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `resolution` | float | `0.05` | Grid cell size (5 cm per cell) |
| `grid_width` | float | `4.0` | Lateral coverage in meters (-2.0m to +2.0m) |
| `grid_length` | float | `4.0` | Forward coverage in meters (0.0m to +4.0m) |
| `safe_step_threshold` | float | `0.04` | Height delta $\le 4\text{cm}$ is classified as flat ground (cost 0) |
| `obstacle_threshold` | float | `0.12` | Height delta $\ge 12\text{cm}$ is non-traversable (cost 100) |
| `min_z_cutoff` | float | `-0.15` | Floor noise filter cutoff in `base_link` frame |
| `max_z_cutoff` | float | `1.50` | Ceiling / overhang filter cutoff in `base_link` frame |
| `base_frame` | string | `"base_link"` | Robot reference coordinate frame |
| `input_topic` | string | `"/camera/depth/points"` | Subscribed point cloud topic (Sensor QoS) |
| `output_topic` | string | `"/traversability_map"` | Published OccupancyGrid topic |



