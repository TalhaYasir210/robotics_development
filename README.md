# TurtleBot Go-To-Pose Controller
### About This Repository
A robust ROS 2 C++ node for turtlesim that navigates the turtle to specific (x,y) coordinates and adjusts its orientation (θ)

### Prerequisites
1. ROS 2 (Humble, Jazzy, or newer)
2. turtlesim package
3. colcon build tools

### Using this Repository
#### Step by Step Execution:
Follow these commands exactly to set up and run the controller
##### (Note: All commands below should be executed in your terminal)

| Step No | Task | Description | Command |
| --- | --- | --- |  --- |
| 1 | Setup | Create the ROS 2 workspace |  mkdir -p ~/ros2_ws/src && cd ~/ros2_ws |
| 2| Clone| Clone project repository | git clone --single-branch --branch gotopose  --depth  https://github.com/TalhaYasir210/robotics_development.git |
| 3 | Build | Compile the package |  colcon build --packages-select go_to_pose_pkg |
| 4| Source | Load environment |  source install/setup.bash |
| 5| Launch | Open New Terminal & Start simulator |  ros2 run turtlesim turtlesim_node |
| 6 | Run | Open First Terminal & Run the controller |  ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=6.0 -p target_y:=1.0 -p target_theta:=1.57 |
| 7 | (Optional) Debug | If want to see Telementry only then neccessary |  ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=6.0 -p target_y:=3.0 -p target_theta:=1.57 --log-level debug |

### Expected Behavior
| Phase | Action |
| --- | --- |
| Navigating | After Recieving Input Target Values, Turtle will start moving toward the goal cordinates  | 
| Angle Adjustment | After Reaching the goal cordinates , Turtle will start to adjust its angle according to the target theta provided in the input |
| Finished | the node logs a success message and shuts down.  | 

### Project Gallery
1. Initial position of turtle in turtlesim before providing any input
<img width="500" height="500" alt="Initial position of turtle" src="https://github.com/user-attachments/assets/8fddb4e9-6598-4bb7-b47e-95446a176dfb" />


2. Running the controller_node with specified target_x, target_y, and target_theta parameters
<img width="500" height="500" alt="2 1st run cammand " src="https://github.com/user-attachments/assets/93fcbcb4-ec2e-4909-98aa-db9834fcf67c" />

3.The turtle moving toward the goal coordinate while tracing its path.
<img width="500" height="500" alt="3 mov_to_goal" src="https://github.com/user-attachments/assets/4a760cca-e52a-4e15-bc1e-434fe2430013" />


4. Turtle has reached the goal coordinate and is now adjusting its heading to match target_theta.
<img width="500" height="500" alt="4 reachedGoal_adjust_GoalAngle" src="https://github.com/user-attachments/assets/ad56096b-92de-44cc-89b0-1fa079eefba3" />


5. Terminal feedback confirming the mission is accomplished and the node has shut down.
<img width="1272" height="135" alt="5 infoMsg_mission Acomplished" src="https://github.com/user-attachments/assets/a273c6a5-7238-4c82-90d6-6cd5f8abe8bf" />

6. Running the command again with new coordinates to perform a different mission.
<img width="1272" height="135" alt="6 change goal cordinate in run cammand" src="https://github.com/user-attachments/assets/aa3993a0-e4ad-469c-b84b-ab7e96bb8993" />

7. The turtle calculating a new path based on the updated coordinates.
<img width="500" height="500" alt="7 again going to goal" src="https://github.com/user-attachments/assets/9e698059-5dfe-4739-9afc-804c6705a17c" />

8. The turtle reaching the new coordinates and fine-tuning its final orientation.
<img width="500" height="500" alt="8 again adjustting the angle after reached goal" src="https://github.com/user-attachments/assets/7b64fa78-0d99-462f-b28c-f99644cdb3ff" />

9. (A) Error message triggered when providing a negative coordinate value (target_y := -4.0) or beyond the limit, Log msg is also displyaed to highlight the limits of input values.
<img width="1275" height="219" alt="9 invalid input error" src="https://github.com/user-attachments/assets/4e10309f-7b74-4cf8-8690-52009a785fba" />

   (B) Error message triggered when providing a coordinate value exceeding the map limit (target_y := 14.0).
<img width="1275" height="219" alt="10 invalid input error 2" src="https://github.com/user-attachments/assets/bdad06f6-0ca4-42a9-ac65-f2c074766ab1" />




