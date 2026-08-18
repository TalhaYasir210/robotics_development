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

# 3. Current Progress (Completed - Phase 1)

*   **swarm_interfaces**: Created custom `.srv` (SaveQR, GetQR) and `.msg` (QRDetection).
*   **swarm_simulation**: Built `maze_world.sdf` with local portable models (`maze_model_rs`, `qr_target`).
*   **Robot URDF**: Implemented `tb3_namespaced.urdf.xacro` (TurtleBot3 Waffle Pi) utilizing official meshes and Gazebo Harmonic system plugins (DiffDrive, GPU LiDAR, Camera).
*   **Bringup**: Wrote `swarm_bringup.launch.py` which bridges the simulation clock and successfully spawns both robots in the maze.

# 4. Next Steps (Action Items)

*   **Phase 2 (Current Focus)**: Implement the Backend Database server in `swarm_brain/src/database_node.cpp` (using unordered map to store poses) and the QR Vision/TF Projector in `swarm_perception`.
*   **Phase 3**: Create split navigation logic in `swarm_navigation` (slam_toolbox for Bot 1, nav2_amcl for Bot 2) subscribing to a shared global `/map` topic.
*   **Phase 4**: Write the C++ Finite State Machine controllers (`mapper_controller.cpp`, `explorer_controller.cpp`).
*   **Phase 5**: Finalize Docker multi-container orchestration.
