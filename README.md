TurtleBot Go-To-Pose Controller

A robust ROS 2 C++ node for turtlesim that navigates the turtle to specific (x,y) coordinates and adjusts its orientation (θ) using a multi-stage state machine.
Features

    State Machine Logic: Navigates to target coordinates first, then adjusts orientation.

    Input Validation: Automatically detects negative coordinate inputs and shuts down safely.

    Debug Telemetry: Built-in DEBUG level logging for real-time tracking of pose, distance, and angle error.

    Auto-Termination: Gracefully shuts down the node once the mission is accomplished.

Prerequisites

    ROS 2 (Humble or newer recommended)

    turtlesim package installed:
    Bash

    sudo apt install ros-<distro>-turtlesim

Build Instructions

    Navigate to your workspace root:
    Bash

    cd ~/robotics_development

    Build the package:
    Bash

    colcon build --packages-select go_to_pose_pkg

    Source your workspace:
    Bash

    source install/setup.bash

How to Run
1. Launch Turtlesim

Open a new terminal and run:
Bash

ros2 run turtlesim turtlesim_node

2. Run the Controller

Open another terminal, source your workspace, and run the controller node:

Standard Run (Silent):
Bash

ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=8.0 -p target_y:=2.0 -p target_theta:=1.57

Debug Run (To see telemetry):
If you want to see the real-time distance and angle error calculations, add the --log-level debug flag:
Bash

ros2 run go_to_pose_pkg controller_node --ros-args --log-level debug -p target_x:=8.0 -p target_y:=2.0 -p target_theta:=1.57

Parameters
Parameter	Description	Default
target_x	Goal X-coordinate	5.0
target_y	Goal Y-coordinate	5.0
target_theta	Final goal orientation (radians)	0.0
Troubleshooting

    "Package not found": Ensure you have run source install/setup.bash in the terminal where you are running the command.

    Negative Inputs: If you provide negative coordinates, the node will print an error and shut down immediately to prevent unwanted movement.

    Teleop Conflicts: If you want to use your keyboard to move the turtle, ensure you are not running this controller node, as they will compete for control of /turtle1/cmd_vel.
