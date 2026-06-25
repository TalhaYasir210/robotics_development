# TurtleBot3 Autonomous Obstacle Avoidance

### About This Repository
A robust, modular ROS 2 C++ architecture for the TurtleBot3 (Waffle model) that autonomously navigates to specific (x,y) coordinates within a Gazebo simulation while dynamically avoiding obstacles.

### Architecture Overview (How it Works)
To make the robot intelligent and easy to maintain, the code is split into four distinct components. Here is a simple breakdown of how they work together to control the robot:

#### 1. The Manager (`obstacle_avoidance_node.cpp`)
Think of this as the central dispatcher. It doesn't do the heavy lifting or complex math itself. Instead, it listens to the outside world—like the raw sensor data coming from the simulation or the destination coordinates you provide—and passes that information to the other components. It also enforces a strict "10Hz Heartbeat," meaning it ensures the robot updates its decisions and publishes data to the wheels 10 times every single second without fail.

#### 2. The Eyes (`sensor_processor.cpp`)
This component is responsible for translating raw physical data into a format the robot can easily understand. The robot's laser scanner (LiDAR) shoots out hundreds of laser beams in a 360-degree circle to measure distances. The *Sensor Processor* takes all those numbers and groups them into three simple zones: a mathematically calculated "Front" hit-box, a "Left" side, and a "Right" side. It tells the brain exactly how far away the closest walls are in each direction.

#### 3. The Brain (`autonomy_fsm.cpp`)
The *Autonomy Finite State Machine (FSM)* is the robot's decision-making center. It takes the simplified distance data from the *Eyes* and decides what the robot's current "Mode" should be at any given fraction of a second. 
- **TRACKING:** The path is clear, focus on aiming at the destination.
- **DODGING:** An obstacle is too close! Stop tracking the goal and turn away immediately to find a clear gap.
- **RECOVERING:** A gap was found! Drive straight through the gap to safely clear the obstacle before turning back to the destination.
- **ARRIVED:** The destination has been successfully reached.

#### 4. The Muscles (`motion_controller.cpp`)
Once the *Brain* decides what mode the robot should be in, the *Motion Controller* takes over to figure out the exact physical wheel speeds needed to execute that decision. If the brain says "Tracking", the muscles calculate exactly how hard to spin the wheels to smoothly pivot toward the goal and drive forward. If the brain says "Dodging", the muscles steer the wheels sharply away from the nearest wall.

---

### Prerequisites
- ROS 2 (Jazzy)
- Gazebo Simulator & TurtleBot3 Packages
- colcon build tools

### Expected Output
Phase 1: Tracking & Pivoting
After receiving target values, the TurtleBot will calculate the angle error. If the target is not directly in front of it, it will strictly pivot in place to minimize the angle error before driving forward.

Phase 2: Dodging & Recovering
If a wall or obstacle enters the front Lidar cone, the robot will automatically steer away, find a clear gap, push through it, and then re-acquire the target to continue its journey.

Phase 3: Arrived
After reaching the goal coordinates within a 0.2m threshold, the node logs a success message and stops driving.

---

### How to Run

Step by Step Execution: (Follow these commands exactly to set up and run the controller)

Open a terminal (Ctrl+Alt+T) and paste the following command to create a new ROS 2 workspace:
```bash
mkdir -p ~/ros2_ws/src && cd ~/ros2_ws/src
```

In the same terminal, paste the following command to clone the specific branch of the project repository (Make sure to replace [YOUR-BRANCH-NAME] with your actual branch name!):
```bash
git clone -b feature/lidar-obstacle-avoidance https://github.com/TalhaYasir210/robotics_development.git
```

In the same terminal, paste the following commands to compile the cloned package:
```bash
cd ..
```
```bash
colcon build --packages-select obstacle_avoidance
```

In the same terminal, paste the following command to load the environment:
```bash
source install/setup.bash
```

Ensure your Gazebo simulation is already running and unpaused in a separate terminal before proceeding for that open new terminal 
- first load the bot model
```bash
export TURTLEBOT3_MODEL=waffle
```
- secondly launch the turtlebot3 world
```bash
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

## NOTE:
If bot glitches or do unexpected moment press CTRL + C and try to relaunch your turtlebot3 world using following step by step execution of commands
- Kill any lingering ROS 2 code you wrote
```bash
killall -9 obstacle_avoidance_node
```
- Kill the Gazebo physics servers (depending on your Gazebo version)
```bash
killall -9 gzserver gzclient
```
```bash
killall -9 gz ruby
```
- Stop the hidden ROS 2 communication daemon
```bash
ros2 daemon stop
```
- Start a fresh, clean daemon
```bash
ros2 daemon start
```
- load the bot model again
```bash
export TURTLEBOT3_MODEL=waffle
```
- launch the turtlebot3 world again
```bash
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

Open a first terminal, source your workspace, and run the obstacle_avoidance_node with specified goal_x and goal_y parameters:
```bash
ros2 run obstacle_avoidance obstacle_avoidance_node --ros-args -p use_sim_time:=true -p goal_x:=3.0 -p goal_y:=0.5
```
The robot will start navigating toward the goal coordinate while actively avoiding pillars and walls.

## NOTE:
After reaching the desired goal, the terminal will give feedback confirming the mission is accomplished and the node will automatically shut down.
Safety Failsafes & Notes
- Geo-Fence Error: An error message is triggered immediately if the input coordinates exceed the safe map limits (e.g., goal_x := 5.0). The terminal will display a log message highlighting the valid Safe Zone limits and shut down without moving the robot.
- Blocked Goal Error: An error message is triggered if the robot gets close to the goal, but determines the exact coordinate is located physically inside a solid obstacle.
- Emergency Brake: Pressing Ctrl+C while the robot is moving will instantly trigger the node's destructor, publishing a strict zero-velocity command to stop the robot safely.

### Optional: Live Telemetry Debug

If you want to see the live math calculations (current pose, distance to goal, and angle error) updating in real-time, add the --log-level debug flag to the end of your run command:
```bash
ros2 run obstacle_avoidance obstacle_avoidance_node --ros-args -p use_sim_time:=true -p goal_x:=3.0 -p goal_y:=0.5 --log-level debug
```
