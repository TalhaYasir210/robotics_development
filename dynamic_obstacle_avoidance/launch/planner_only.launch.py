import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import SetRemap, Node

import tempfile
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Generate a perfectly stable custom diff-drive box for the obstacle bot
    sdf_content = """<?xml version="1.0" ?>
<sdf version="1.8">
  <model name="obstacle_bot">
    <pose>0 0 0.1 0 0 0</pose>
    <link name="base_link">
      <inertial>
        <mass>2.0</mass>
        <inertia><ixx>0.01</ixx><ixy>0</ixy><ixz>0</ixz><iyy>0.01</iyy><iyz>0</iyz><izz>0.01</izz></inertia>
      </inertial>
      <collision name="collision">
        <geometry><box><size>0.4 0.4 0.2</size></box></geometry>
      </collision>
      <visual name="visual">
        <geometry><box><size>0.4 0.4 0.2</size></box></geometry>
        <material><ambient>1 0 0 1</ambient><diffuse>1 0 0 1</diffuse><specular>0.1 0.1 0.1 1</specular></material>
      </visual>
      <!-- Frictionless Caster -->
      <collision name="caster_collision">
        <pose>-0.15 0 -0.1 0 0 0</pose>
        <geometry><sphere><radius>0.05</radius></sphere></geometry>
        <surface><friction><ode><mu>0.0</mu><mu2>0.0</mu2></ode></friction></surface>
      </collision>
      <visual name="caster_visual">
        <pose>-0.15 0 -0.1 0 0 0</pose>
        <geometry><sphere><radius>0.05</radius></sphere></geometry>
        <material><ambient>0 1 0 1</ambient><diffuse>0 1 0 1</diffuse></material>
      </visual>
    </link>

    <link name="left_wheel">
      <pose>0.1 0.23 -0.05 -1.5707 0 0</pose>
      <inertial>
        <mass>0.5</mass>
        <inertia><ixx>0.001</ixx><ixy>0</ixy><ixz>0</ixz><iyy>0.001</iyy><iyz>0</iyz><izz>0.001</izz></inertia>
      </inertial>
      <collision name="collision">
        <geometry><cylinder><radius>0.1</radius><length>0.04</length></cylinder></geometry>
        <surface><friction><ode><mu>1.0</mu><mu2>1.0</mu2></ode></friction></surface>
      </collision>
      <visual name="visual">
        <geometry><cylinder><radius>0.1</radius><length>0.04</length></cylinder></geometry>
        <material><ambient>0 0 0 1</ambient><diffuse>0 0 0 1</diffuse></material>
      </visual>
    </link>

    <link name="right_wheel">
      <pose>0.1 -0.23 -0.05 -1.5707 0 0</pose>
      <inertial>
        <mass>0.5</mass>
        <inertia><ixx>0.001</ixx><ixy>0</ixy><ixz>0</ixz><iyy>0.001</iyy><iyz>0</iyz><izz>0.001</izz></inertia>
      </inertial>
      <collision name="collision">
        <geometry><cylinder><radius>0.1</radius><length>0.04</length></cylinder></geometry>
        <surface><friction><ode><mu>1.0</mu><mu2>1.0</mu2></ode></friction></surface>
      </collision>
      <visual name="visual">
        <geometry><cylinder><radius>0.1</radius><length>0.04</length></cylinder></geometry>
        <material><ambient>0 0 0 1</ambient><diffuse>0 0 0 1</diffuse></material>
      </visual>
    </link>

    <joint name="left_wheel_joint" type="revolute">
      <parent>base_link</parent><child>left_wheel</child><axis><xyz>0 0 1</xyz></axis>
    </joint>
    <joint name="right_wheel_joint" type="revolute">
      <parent>base_link</parent><child>right_wheel</child><axis><xyz>0 0 1</xyz></axis>
    </joint>

    <plugin filename="gz-sim-diff-drive-system" name="gz::sim::systems::DiffDrive">
      <left_joint>left_wheel_joint</left_joint>
      <right_joint>right_wheel_joint</right_joint>
      <wheel_separation>0.46</wheel_separation>
      <wheel_radius>0.1</wheel_radius>
      <max_linear_acceleration>2.0</max_linear_acceleration>
      <max_angular_acceleration>3.0</max_angular_acceleration>
      <topic>/obstacle_bot/cmd_vel</topic>
      <odom_topic>/obstacle_bot/odom</odom_topic>
      <frame_id>obstacle_bot/odom</frame_id>
      <child_frame_id>obstacle_bot/base_link</child_frame_id>
      <odom_publisher_frequency>30</odom_publisher_frequency>
      <tf_topic>/obstacle_bot/tf</tf_topic>
    </plugin>
  </model>
</sdf>
"""
    
    # Write to a temporary file
    tmp_sdf = tempfile.NamedTemporaryFile(delete=False, suffix='.sdf')
    tmp_sdf.write(sdf_content.encode())
    tmp_sdf.close()
    obstacle_sdf_path = tmp_sdf.name

    nav2_bringup_dir = FindPackageShare('nav2_bringup').find('nav2_bringup')
    my_pkg_dir = FindPackageShare('dynamic_obstacle_avoidance').find('dynamic_obstacle_avoidance')
    
    params_file = os.path.join(my_pkg_dir, 'config', 'nav2_params.yaml')

    return LaunchDescription([
        # Group Nav2 and remap its output cmd_vel to cmd_vel_nav
        GroupAction(
            actions=[
                SetRemap(src='/cmd_vel', dst='/cmd_vel_nav'),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(
                        os.path.join(nav2_bringup_dir, 'launch', 'navigation_launch.py')
                    ),
                    launch_arguments={
                        'params_file': params_file,
                        'use_sim_time': 'true',
                        'use_velocity_smoother': 'true',
                        'use_collision_monitor': 'true'
                    }.items()
                )
            ]
        ),
        
        # Launch our dynamic obstacle detector node to act as gatekeeper
        Node(
            package='dynamic_obstacle_avoidance',
            executable='dynamic_obstacle_detector_node',
            name='dynamic_obstacle_detector_node',
            output='screen',
            parameters=[
                {'danger_zone_x_min': 0.1},
                {'danger_zone_x_max': 1.0},
                {'danger_zone_y_width': 0.7},
                {'dynamic_velocity_threshold': 0.15},
                {'cluster_tolerance': 0.3}
            ]
        ),
        
        # Spawn the secondary dynamic obstacle bot in Gazebo Ignition
        Node(
            package='ros_gz_sim',
            executable='create',
            arguments=[
                '-name', 'obstacle_bot',
                '-file', obstacle_sdf_path,
                '-x', '0.0',
                '-y', '1.5',
                '-z', '0.1'
            ],
            output='screen'
        ),
        
        # Bridge the obstacle bot cmd_vel topic from ROS to GZ
        Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            arguments=[
                '/obstacle_bot/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
            ],
            output='screen'
        )
    ])
