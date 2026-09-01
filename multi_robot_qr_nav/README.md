# Multi-Robot Swarm

A fully asynchronous, decentralized multi-robot system using ROS 2 Jazzy and Gazebo Harmonic. In this project, an explorer robot dynamically localizes itself within a mapper robot's dynamically generated map by visually servoing to QR codes and referencing a central in-memory database.
After the bot2 has successfully initialized the pose , nav2 action server activates, and then can give 2d goal poses to bot2 via Rviz

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
git switch multi-bot-ai-navigation
cd robotics_development/multi_robot_qr_nav
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
### 4. Rviz configuration
- change /map topic to tb3_2/map , in bot2 rviz window

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
* **Interactive RViz Navigation:**  With the map fully synchronized and Bot 2's initial pose accurately locked in, the system unlocks operator control. You can now use the "2D Goal Pose" tool directly in RViz to issue target destinations to Bot 2, allowing it to leverage its newly initialized Nav2 stack to autonomously plan and execute complex paths through the shared environment.




<img width="2048" height="1152" alt="continuously recieving updated map from bot1(2)" src="https://github.com/user-attachments/assets/f9fdb13c-96a7-4b4d-906b-26c12a2717d7" />
<img width="2048" height="1152" alt="continuously recieving updated map from bot1(1)" src="https://github.com/user-attachments/assets/7b1e00fe-7358-47cc-8c79-b5d82d3aacee" />
<img width="2048" height="1152" alt="continuously recieving updated map from bot1" src="https://github.com/user-attachments/assets/470ed876-bd23-4ef6-807c-8dd38a0f1b27" />


## Flow Charts:
<img width="1510" height="1699" alt="docker_containarization" src="https://github.com/user-attachments/assets/f2bbc451-b78e-486b-9f46-ebe53db6910b" />
<img width="2342" height="1594" alt="topics_communications_flowchart" src="https://github.com/user-attachments/assets/7a2b9de7-c9b3-45c0-a7cc-829810fd0c77" />

