Prerequisites;
Ensure you have the following installed:
1. ROS 2 (Humble, Jazzy, or newer)
2. turtlesim package
3. colcon build tools

Step by Step Guide:
1. create workspace -> mkdir -p ~/ros2_ws
2. go to that directory -> cd ~/ros2_ws
3. Clone the branch into your local machine -> git clone --single-branch --branch gotopose  --depth  https://github.com/TalhaYasir210/robotics_development.git
4. Build the pkg -> colcon build --packages-select go_to_pose_pkg
5. Source the workspace -> source install/setup.bash
6. open and run turtlesim in new terminal -> ros2 run turtlesim turtlesim_node
7. Run the control_node -> ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=6.0 -p target_y:=1.0 -p target_theta:=1.57
8. To see Debug as well -> ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=6.0 -p target_y:=3.0 -p target_theta:=1.57 --log-level debug

Expected Behavior:
Phase 1 (Navigating): The turtle will move linearly and angularly until it reaches the target (x,y).
Phase 2 (Adjusting): Once at the coordinate, the turtle will rotate until it matches your target target_theta.
Phase 3 (Finished): The mission will complete, and the node will automatically shut down.
