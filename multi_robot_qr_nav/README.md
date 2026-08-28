# Multi-Robot Kidnapped Robot Swarm

A fully asynchronous, decentralized multi-robot system using ROS 2 Jazzy and Gazebo Harmonic. In this project, an explorer robot dynamically localizes itself within a mapper robot's dynamically generated map by visually servoing to QR codes and referencing a central in-memory database.

## Prerequisites

To run this project, you only need Docker installed on your system. All ROS 2 packages and dependencies are containerized.
- [Docker](https://docs.docker.com/get-docker/)
- [Docker Compose](https://docs.docker.com/compose/install/)
- [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html) (Required for Gazebo Hardware Acceleration)

## How to Run

Because the entire architecture is fully containerized and optimized, you can build and launch the simulation, mapper, and brain containers with a single command:

```bash
docker compose up --build
```

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
