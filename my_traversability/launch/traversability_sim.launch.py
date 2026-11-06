#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import AppendEnvironmentVariable, DeclareLaunchArgument, IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    pkg_traversability = get_package_share_directory('my_traversability')
    pkg_tb3_gazebo = get_package_share_directory('turtlebot3_gazebo')
    ros_gz_sim = get_package_share_directory('ros_gz_sim')

    # Ensure TurtleBot3 Waffle model (equipped with RealSense camera) is selected
    set_tb3_model = SetEnvironmentVariable('TURTLEBOT3_MODEL', 'waffle')

    # Path to our self-contained house world (no roof, with rooms, poles, and test obstacles)
    world = os.path.join(pkg_traversability, 'worlds', 'house_world.world')

    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    x_pose = LaunchConfiguration('x_pose', default='0.0')
    y_pose = LaunchConfiguration('y_pose', default='0.0')

    # 1. Gazebo Harmonic Physics Server
    gzserver_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': ['-r -s -v2 ', world], 'on_exit_shutdown': 'true'}.items()
    )

    # 2. Gazebo Harmonic GUI Client (3D Window)
    gzclient_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': '-g -v2 '}.items()
    )

    # 3. Robot State Publisher for TurtleBot3 Waffle
    robot_state_publisher_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_tb3_gazebo, 'launch', 'robot_state_publisher.launch.py')
        ),
        launch_arguments={'use_sim_time': use_sim_time}.items()
    )

    # 4. Spawn TurtleBot3 Waffle at center (0.0, 0.0) facing the test obstacles at X = 1.8
    spawn_turtlebot_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_tb3_gazebo, 'launch', 'spawn_turtlebot3.launch.py')
        ),
        launch_arguments={
            'x_pose': x_pose,
            'y_pose': y_pose
        }.items()
    )

    # 5. Bridge simulation clock and depth camera point cloud from Gazebo Harmonic to ROS 2
    bridge_clock_points = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='bridge_clock_points',
        output='screen',
        arguments=[
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
            '/camera/points@sensor_msgs/msg/PointCloud2[gz.msgs.PointCloudPacked',
        ],
        remappings=[
            ('/camera/points', '/camera/depth/points'),
        ]
    )

    # 6. Launch our Traversability Perception Node
    traversability_node = Node(
        package='my_traversability',
        executable='traversability_node',
        name='traversability_node',
        output='screen',
        parameters=[{
            'use_sim_time': use_sim_time,
            'target_frame': 'base_link',
            'pointcloud_topic': '/camera/depth/points',
            'map_topic': '/traversability_map',
            'grid_width': 4.0,
            'grid_length': 4.0,
            'resolution': 0.05,
            'safe_step_threshold': 0.06,
            'obstacle_threshold': 0.12,
            'min_z_cutoff': -0.15,
            'max_z_cutoff': 1.05
        }]
    )

    # Resource paths for TurtleBot3 robot models & local models
    set_env_resources = AppendEnvironmentVariable(
        'GZ_SIM_RESOURCE_PATH',
        f"{os.path.join(pkg_traversability, 'models')}:{os.path.join(pkg_tb3_gazebo, 'models')}"
    )

    return LaunchDescription([
        set_tb3_model,
        set_env_resources,
        gzserver_cmd,
        gzclient_cmd,
        robot_state_publisher_cmd,
        spawn_turtlebot_cmd,
        bridge_clock_points,
        traversability_node
    ])
