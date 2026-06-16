TurtleBot Go-To-Pose Controller
Prerequisites
Before building, ensure the following tools and libraries are installed on your system:

    ROS 2: (Humble, Jazzy, or newer)
    
    turtlesim: Required for simulation.
    
    colcon: The ROS 2 build tool (python3-colcon-common-extensions).
    
    rosdep: For automatic dependency resolution.
    
    python3-pip and python3-setuptools: Required for Python-based ROS package compilation.
    
Step 3: Project Installation
Now, follow these commands exactly to set up the project folder:
    
    mkdir -p ~/go_to_pose_turtlesim_project
    
    git clone --branch p1_gotopose https://github.com/TalhaYasir210/robotics_development.git  go_to_pose_turtlesim_project
    
    cd go_to_pose_turtlesim_project
    
    colcon build --packages-select go_to_pose_pkg
    
    source install/setup.bash
    
    open turtlesim:open new terminal -> paste the cammand -> ros2 run turtlesim turtlesim_node
    
    To run the code -> open first terminal -> paste this cammand (note: Value of target_x, target_y & target_theta can be set according to your own desires)-> ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=8.0 -p target_y:=2.0 -p target_theta:=1.57

Debug Run (To see telemetry):
If you want to see the real-time distance and angle error calculations, add the --log-level debug flag:
Bash

    ros2 run go_to_pose_pkg controller_node --ros-args --log-level debug -p target_x:=8.0 -p target_y:=2.0 -p target_theta:=1.57


Parameters:
Parameter	Description	Default
target_x	Goal X-coordinate	5.0
target_y	Goal Y-coordinate	5.0
target_theta	Final goal orientation (radians)	0.0

Troubleshooting:

    "Package not found": Ensure you have run source install/setup.bash in the terminal where you are running the command.

    Negative Inputs: If you provide negative coordinates, the node will print an error and shut down immediately to prevent unwanted movement.

    Teleop Conflicts: If you want to use your keyboard to move the turtle, ensure you are not running this controller node, as they will compete for control of /turtle1/cmd_vel.

  
