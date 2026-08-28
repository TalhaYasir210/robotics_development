AGENT INSTRUCTION: Read this file before suggesting architectural changes or writing new nodes to ensure alignment with the asynchronous database workflow and Gazebo Harmonic requirements.

# 1. Project Overview & End Goal

**Name**: Multi-Robot Kidnapped Robot Swarm

**Goal**: A fully asynchronous, decentralized multi-robot system where robots do not communicate directly. Instead, they interact via an in-memory backend database.

**Workflow**: Bot 1 (Mapper) maps an enclosed maze and saves QR code coordinates to the database. Bot 2 (Explorer) wanders blindly, scans a QR code, queries the database for global coordinates, and injects that pose into AMCL to instantly localize itself within Bot 1's map.

# 2. Tech Stack & Rules

**Framework**: ROS 2 Jazzy Jalisco.

**Simulator**: Gazebo Harmonic (using ros_gz packages, NOT gazebo_ros_pkgs).

**Language**: C++17 (for nodes) and Python (for launch files).

*   **Rule 1**: Always use proper namespacing (`tb3_1`, `tb3_2`) and `tf_prefix` to prevent TF tree collisions.
*   **Rule 2**: Avoid symlink installs (`--symlink-install`); use standard colcon build to prevent caching issues.

# 3. Current Progress (Completed)

*   **swarm_interfaces**: Created custom `.srv` (SaveQR, GetQR, SaveMap, GetMap) and `.msg` (QRDetection).
*   **swarm_simulation**: Built `maze_world.sdf` with local portable models.
*   **Robot URDF**: Implemented `tb3_namespaced.urdf.xacro` (TurtleBot3 Waffle Pi) utilizing official meshes and Gazebo Harmonic system plugins.
*   **Bringup**: Wrote `swarm_bringup.launch.py` which bridges the simulation clock and successfully spawns both robots in the maze.
*   **swarm_brain**: Implemented `database_node.cpp` (in-memory storage of QR poses and the global Map with distinct child loggers) and `map_saver_client.cpp` (syncs Bot 1's map every 15s and detects exploration completion).
*   **swarm_perception**: 
    - Implemented `bot1_interceptor.cpp`: Visually servos Bot 1 to QR codes and saves their accurate global pose to the database. Obstacle avoidance is intentionally disabled during servoing for aggressive approaches.
    - Implemented `explorer_node.cpp` (Bot 2's FSM): Blindly wanders using LiDAR obstacle avoidance, visually servos to QR codes (with recovery strategies), calculates inverse kinematics for initial pose, downloads the Map from the database, and dynamically triggers AMCL. Continuous map polling stops once the database marks mapping as complete.
    - Configured `nav2_params_bot2.yaml` and `bot2_navigation.launch.py`: Fully functional AMCL and Nav2 stack for Bot 2 using `RewrittenYaml` to dynamically inject the `tb3_2` namespace.
*   **Launch Files**: Separated `bot1_brain_perception.launch.py` and `bot2_brain_perception.launch.py` for full independent control.

*   **Docker Orchestration (Phase 4)**: 
    - Implemented a 3-container production-ready deployment (`simulation`, `mapper`, `brain_perception`) via `docker-compose.yml`.
    - Automated dependency management (`rosdep` for OS, `requirements.txt` for Python, and explicitly installed `libzbar-dev`).
    - Utilized `BUILD_PACKAGES` build arguments to strictly isolate package compilation for each container.
    - Created a unified `swarm_brain_perception.launch.py` master launch file to run both Bot 1 and Bot 2's backend/perception nodes cleanly within the single brain container.
    - Cleaned up workspace paths to allow single-command `docker compose up --build` deployment from any client host directory.

# 4. Next Steps (Action Items)

*   **Phase 5 (Next Focus)**: Final system testing, parameter tuning, or potential hardware deployment scaling. The core software architecture is complete and fully containerized.
