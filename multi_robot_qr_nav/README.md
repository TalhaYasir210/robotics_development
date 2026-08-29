# Multi-Robot Kidnapped Robot Swarm

A fully asynchronous, decentralized multi-robot system using ROS 2 Jazzy and Gazebo Harmonic. In this project, an explorer robot dynamically localizes itself within a mapper robot's dynamically generated map by visually servoing to QR codes and referencing a central in-memory database.

## Demo Video

*[Video Demo will be added here later]*

## Prerequisites

To run this project, you only need Docker installed on your system. All ROS 2 packages and dependencies are containerized.
- [Docker](https://docs.docker.com/get-docker/)
- [Docker Compose](https://docs.docker.com/compose/install/)
- [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html) (Required for Gazebo Hardware Acceleration)

## How to Run

### 1. Workspace Setup
Clone the repository and switch to the working directory:
```bash
git clone https://github.com/TalhaYasir210/robotics_development.git
cd robotics_development/multi_robot_qr_nav
# git checkout <branch-name>  # Switch to your branch if you aren't on main
```

### 2. Allow GUI Forwarding
Allow Docker to connect to your local X server for GUI applications (like Gazebo and RViz):
```bash
xhost +local:docker
```

### 3. Launch the Swarm
Because the entire architecture is fully containerized and optimized, you can build and launch the simulation, mapper, and brain containers with a single command:

```bash
docker compose up --build -d
```

### 4. Monitor Logs
If you want to view the specific logs for any container in real-time, you can use the following commands:

- **`simulation`** *(Gazebo Harmonic, TurtleBot spawning, bridge outputs)*:
  ```bash
  docker compose logs -f simulation
  ```
- **`mapper`** *(Bot 1 SLAM Toolbox and explore_lite frontier exploration)*:
  ```bash
  docker compose logs -f mapper
  ```
- **`brain_perception`** *(Central database, QR visual servoing, and Bot 2 FSM/Nav2 stack)*:
  ```bash
  docker compose logs -f brain_perception
  ```

To stop and safely shut down the environment:
```bash
docker compose down
```

## How It Works

* **Bot 1 (The Mapper):** Spawns immediately and begins exploring the unknown environment. It uses `slam_toolbox` to generate a high-performance 2D map and `explore_lite` to autonomously hunt for frontiers. As it explores, a specialized interceptor node actively looks for QR codes through Bot 1's camera, calculates their real-world coordinates, and registers them into a central, in-memory database.
* **Bot 2 (The Explorer):** It starts completely blind without any map and begins wandering the environment. It uses a custom obstacle avoidance and yielding algorithm to avoid static walls and smartly yield/escape if it detects the dynamic Bot 1 approaching it.
* **Solving the Lost Robot Problem:** While wandering, if Bot 2 visually spots a QR code through its camera, it enters a visual servoing state to align with it. It then queries the central database to check if Bot 1 has already mapped that specific QR code. If the database returns the coordinates, Bot 2 calculates the inverse kinematics to perfectly localize its exact position and orientation within Bot 1's map frame.
* **Dynamic Map Synchronization:** Once localized, Bot 2 requests the actual generated map from the database and initializes its own Nav2 stack. From that point on, it operates as a fully localized robot navigating inside the dynamically updated map provided by Bot 1.
