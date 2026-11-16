# Real-Time 3D Elevation & Traversability Mapping (`my_traversability`)

A high-performance, zero-heap real-time 3D elevation and ground traversability mapping package for autonomous mobile robots running **ROS 2 Jazzy** and **Gazebo Harmonic**.

This package processes dense 3D depth point clouds at **30 Hz** (76,800 points/frame), classifies terrain step height ($\Delta Z$), and generates:
1. **Nav2-Compatible 2D OccupancyGrid** (`/traversability_map`) for global and local path planners.
2. **Vivid 3D RViz2 Visual Markers** (`/traversability_markers`) color-coding terrain into:
   - 🟢 **Safe Floor / 4 cm Step (Cost 0):** Flat floor traversable smoothly.
   - 🟡 **Rough Slope / 8 cm Ramp (Cost 1–99):** Linear cost interpolation for inclined terrain.
   - 🔴 **Lethal Obstacle / 15 cm Box (Cost 100):** Impassable walls, boxes, and cliffs.

---


## System Requirements

* **OS:** Ubuntu 24.04 LTS (Noble Numbat)
* **ROS 2 Distribution:** ROS 2 Jazzy Jalisco
* **Simulator:** Gazebo Harmonic (gz-sim 8)
* **Compiler:** GCC 13+ / Clang 17+ with C++17 support

---

## Step-by-Step Installation & Setup

### 1. Source ROS 2 Environment
```bash
source /opt/ros/jazzy/setup.bash
```

### 2. Create Workspace & Clone Repository
```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src

# Clone the repository
git clone https://github.com/TalhaYasir210/robotics_development.git
```

### 3. Install Dependencies
```bash
cd ~/ros2_ws
rosdep update
rosdep install --from-paths src --ignore-src -r -y
```

Make sure the ROS 2 Gazebo vendor packages are installed:
```bash
sudo apt install -y \
  ros-jazzy-ros-gz-sim \
  ros-jazzy-ros-gz-bridge \
  ros-jazzy-ros-gz-interfaces \
  ros-jazzy-turtlebot3-teleop
```

### 4. Build the Package
```bash
cd ~/ros2_ws
colcon build --packages-select my_traversability
```

### 5. Source the Workspace
```bash
source ~/ros2_ws/install/setup.bash
```
---

## How to Run

### Launch Simulation + Perception + RViz (All-in-One)
This single command brings up Gazebo Harmonic with the TurtleBot3 Waffle robot, the test world containing 4cm, 8cm, and 15cm obstacles, the perception node, and pre-configured RViz2:

```bash
source /opt/ros/jazzy/setup.bash
source ~/ros2_ws/install/setup.bash

ros2 launch my_traversability traversability_sim.launch.py
```

###  Drive the Robot using Teleop
In a second terminal, launch the keyboard teleoperation node:

```bash
source /opt/ros/jazzy/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```
Use the `i`, `j`, `k`, `l`, `,` keys to drive the robot towards the obstacles and watch the grid update live!

---

## Running Automated Tests & Benchmarks

The package includes **50 automated unit tests** and a **1,000-frame real-time profiling benchmark** that verifies zero memory leaks:

```bash
cd ~/ros2_ws
source install/setup.bash

# Run all 50 unit tests
colcon test --packages-select my_traversability && colcon test-result --verbose
```

### Run the Standalone 1,000-Frame Latency Benchmark:
```bash
./build/my_traversability/benchmark_traversability
```

**Expected Output:**
```text
===================================================================
 [BENCHMARK] Executing 1,000 Frames Across Full Perception Pipeline
 Cloud Size: 76,800 points (320x240)
 Target Frequency Budget: 30.0 Hz (33.33 ms/frame)
===================================================================
 Frames Processed: 1000
 Average Latency:  0.617 ms
 Median (P50):     0.392 ms
 Max Throughput:   1619.8 FPS
 Real-Time Margin: 32.7 ms headroom
===================================================================
 [HEAP PROFILER] Verification Across 1000 Iterations
 Total Intercepted Heap Allocations: 0 calls
 Total Intercepted Bytes Allocated:  0 bytes
 glibc mallinfo2 Heap Growth:        0 bytes
 Memory Leaks Detected:              0 bytes
===================================================================
```

---

## ROS 2 Topics & Interfaces

### Subscribed Topics
| Topic Name | Type | Description |
| :--- | :--- | :--- |
| `/camera/depth/points` | `sensor_msgs/msg/PointCloud2` | Raw 30 Hz organized depth cloud from RGB-D camera. |

### Published Topics
| Topic Name | Type | Description |
| :--- | :--- | :--- |
| `/traversability_map` | `nav_msgs/msg/OccupancyGrid` | Standard 2D costmap (80×80 cells) for Nav2 planners. |
| `/traversability_markers` | `visualization_msgs/msg/MarkerArray` | 3D Green/Yellow/Red cubes & wireframe grid for RViz2. |

---

## Runtime Configurable Parameters

Parameters can be adjusted live while the robot is driving without restarting:

```bash
# Example: Change obstacle threshold to 15 cm live
ros2 param set /traversability_node obstacle_threshold 0.15

# Example: Toggle live performance profiling logs
ros2 param set /traversability_node enable_profiling true
```

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `grid_length` | `float` | `4.0` | Forward map length in meters $[0, X]$. |
| `grid_width` | `float` | `4.0` | Lateral map width in meters $[-Y/2, +Y/2]$. |
| `resolution` | `float` | `0.05` | Cell resolution in meters ($5\text{ cm} \times 5\text{ cm}$). |
| `safe_step_threshold` | `float` | `0.06` | Max height step ($\le 6\text{ cm}$) classified as Safe (Cost 0). |
| `obstacle_threshold` | `float` | `0.12` | Min height step ($\ge 12\text{ cm}$) classified as Lethal (Cost 100). |
| `min_z_cutoff` | `float` | `-0.15` | Minimum Z threshold to filter underground noise. |
| `max_z_cutoff` | `float` | `1.05` | Maximum Z threshold to filter overhead ceilings. |
| `enable_profiling` | `bool` | `true` | Prints rolling latency statistics every 100 frames. |

