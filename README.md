# TurtleBot Go-To-Pose Controller
### About This Repository
A robust ROS 2 C++ node for turtlesim that navigates the turtle to specific (x,y) coordinates and adjusts its orientation (θ)

### Prerequisites
1. ROS 2 (Jazzy)
2. turtlesim package
3. colcon build tools


### Expected Output
| Phase | Action |
| --- | --- |
| Navigating | After Recieving Input Target Values, Turtle will start moving toward the goal cordinates  | 
| Angle Adjustment | After Reaching the goal cordinates , Turtle will start to adjust its angle according to the target theta provided in the input |
| Finished | the node logs a success message and shuts down.  | 

### How to Run
#### Step by Step Execution: (Follow these commands exactly to set up and run the controller)
1. open terminal by pressing (ctrl+Alt+T) and paste the following command to create a new ROS 2 workspace
```bash
mkdir -p ~/ros2_ws/src && cd ~/ros2_ws/src
```
2. In the same terminal paste the following command to clone the project repository
```bash
git clone https://github.com/TalhaYasir210/robotics_development.git
```
3. In the same terminal paste the following command to compile the cloned pacakge
```bash
cd ../..
colcon build --packages-select go_to_pose_pkg
```
4. in the same terminal paste the following command to Load environment
```bash
source install/setup.bash
```
5. To Start turtlesim simulator paste following code in your new terminal window
```bash
ros2 run turtlesim turtlesim_node
```
After turtlesim launches the Initial position of turtle in turtlesim before providing any input will look something like this
<img width="500" height="500" alt="Initial position of turtle" src="https://github.com/user-attachments/assets/8fddb4e9-6598-4bb7-b47e-95446a176dfb" />


6. (A) Open First Terminal & Run the controller_node with specified target_x, target_y, and target_theta parameters using following line of code
```bash
ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=6.0 -p target_y:=1.0 -p target_theta:=1.57
```
The turtle will start moving toward the goal coordinate while tracing its path like this.
<img width="500" height="500" alt="3 mov_to_goal" src="https://github.com/user-attachments/assets/4a760cca-e52a-4e15-bc1e-434fe2430013" />


 (B) After reaching the goal coordinates, turtle will start adjusting its head to match target_theta like this.
<img width="500" height="500" alt="4 reachedGoal_adjust_GoalAngle" src="https://github.com/user-attachments/assets/ad56096b-92de-44cc-89b0-1fa079eefba3" />


(C) After Reaching the desired goal and angle Terminal will give feedback confirming the mission is accomplished and the node has shut down.

***NOTE (A) Error message triggered when providing a negative coordinate value (target_y := -4.0) or beyond the limit, Log msg is also displyaed to highlight the limits of input values.
   (B) Error message triggered when providing a coordinate value exceeding the map limit (target_y := 14.0).***

7. ***Optional*** if want to see dubug logger as well , add < --log-level debug > as below along with the run command
```bash
ros2 run go_to_pose_pkg controller_node --ros-args -p target_x:=6.0 -p target_y:=3.0 -p target_theta:=1.57 --log-level debug
```
