# Obstacle Avoidance Project Rules

## 1. Explicit Permission for Code Changes (CRITICAL)
- **Do not modify, add, or delete any files without explicit user permission.**
- Before executing any file edits, present the exact proposed changes (e.g., using diff blocks or a clear explanation) and wait for the user to say "proceed" or approve.

## 2. Architectural Integrity
- **Maintain Modularity:** Keep the ROS 2 API code (publishers, subscribers, timers) isolated within `obstacle_avoidance_node.cpp`. All business logic must remain decoupled in `SensorProcessor`, `AutonomyFSM`, and `MotionController`.
- **Heartbeat Pattern:** Respect the 10Hz universal heartbeat loop for data publishing. Do not bypass this loop to publish arbitrary commands elsewhere.
- **Single Source of Truth:** `ProcessedSensorData` is the sole data format to be passed between the processor, brain, and actuator.

## 3. Style and Conventions
- Follow standard ROS 2 C++ style guidelines.
- Keep visualization debug streams (e.g., split LiDAR scans for RViz) alive regardless of the current FSM state.
