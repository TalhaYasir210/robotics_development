TurtleBot3 Autonomous Obstacle Avoidance
About This Repository

A robust ROS 2 C++ node for the TurtleBot3 (Waffle model) that autonomously navigates to specific (x,y) coordinates within a Gazebo simulation. This node uses a 5-state Finite State Machine (FSM) to process live Lidar (/scan) and Odometry (/odom) data, allowing the robot to dynamically dodge obstacles, recover its path, and respect strict physical geo-fence limits.
Prerequisites

    ROS 2 (Jazzy)

    Gazebo Simulator & TurtleBot3 Packages

    colcon build tools

Expected Output

    Phase 1: Tracking & Pivoting
    After receiving target values, the TurtleBot will calculate the angle error. If the target is behind it, it will smartly pivot in place like a tank before driving forward.

    Phase 2: Dodging & Recovering
    If a wall or obstacle enters the front Lidar cone, the robot will automatically steer away, find a clear gap, push through it, and re-acquire the target.

    Phase 3: Arrived
    After reaching the goal coordinates within a 0.2m threshold, the node logs a success message, slams on the brakes, and safely shuts down.

How to Run

Step by Step Execution: (Follow these commands exactly to set up and run the controller)

Open a terminal (Ctrl+Alt+T) and paste the following command to create a new ROS 2 workspace:
Bash

mkdir -p ~/ros2_ws/src && cd ~/ros2_ws/src

In the same terminal, paste the following command to clone the specific branch of the project repository (Make sure to replace [YOUR-BRANCH-NAME] with your actual branch name!):
Bash

git clone -b [YOUR-BRANCH-NAME] https://github.com/TalhaYasir210/robotics_development.git

In the same terminal, paste the following commands to compile the cloned package:
Bash

cd ~/ros2_ws
colcon build --packages-select obstacle_avoidance

In the same terminal, paste the following command to load the environment:
Bash

source install/setup.bash

(Ensure your Gazebo simulation is already running and unpaused in a separate terminal before proceeding).

Open a new terminal, source your workspace, and run the obstacle_avoidance_node with specified goal_x and goal_y parameters:
Bash

ros2 run obstacle_avoidance obstacle_avoidance_node --ros-args -p use_sim_time:=true -p goal_x:=3.0 -p goal_y:=0.5

The robot will start navigating toward the goal coordinate while actively avoiding pillars and walls.

After reaching the desired goal, the terminal will give feedback confirming the mission is accomplished and the node will automatically shut down.
Safety Failsafes & Notes

    (A) Geo-Fence Error: An error message is triggered immediately if the input coordinates exceed the safe map limits (e.g., goal_x := 5.0). The terminal will display a log message highlighting the valid Safe Zone limits and shut down without moving the robot.

    (B) Blocked Goal Error: An error message is triggered if the robot gets close to the goal, but determines the exact coordinate is located physically inside a solid obstacle.

    (C) Emergency Brake: Pressing Ctrl+C while the robot is moving will instantly trigger the node's destructor, publishing a strict zero-velocity command to stop the robot safely.

Optional: Live Telemetry Debug

If you want to see the live math calculations (current pose, distance to goal, and angle error) updating in real-time, add the --log-level debug flag to the end of your run command:
Bash

ros2 run obstacle_avoidance obstacle_avoidance_node --ros-args -p use_sim_time:=true -p goal_x:=3.0 -p goal_y:=0.5 --log-level debug