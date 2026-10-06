#!/usr/bin/env python3

"""
Launch self-contained simulation for TurtleBot3 Waffle with depth camera.

This launch file:
1. Spawns headless Gazebo Sim (gz-sim) with traversability_world.
2. Publishes TurtleBot3 robot state transforms via robot_state_publisher.
3. Bridges Gazebo topics to ROS 2 (clock, odom, tf, cmd_vel, camera points/images).
4. Launches the traversability_node with parameters from params.yaml.
5. Launches RViz2 with the customized traversability visualizer.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    AppendEnvironmentVariable,
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    SetEnvironmentVariable
)
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_traversability = get_package_share_directory('traversability_mapping')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')

    # Self-contained paths within traversability_mapping package
    models_dir = os.path.join(pkg_traversability, 'models')
    world_file = os.path.join(pkg_traversability, 'worlds', 'traversability_world.world')
    urdf_file = os.path.join(pkg_traversability, 'urdf', 'turtlebot3_waffle.urdf')
    waffle_sdf = os.path.join(models_dir, 'turtlebot3_waffle', 'model.sdf')
    params_file = os.path.join(pkg_traversability, 'config', 'params.yaml')
    rviz_config = os.path.join(pkg_traversability, 'rviz', 'traversability.rviz')

    # Launch Configurations
    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    use_rviz = LaunchConfiguration('use_rviz', default='true')
    x_pose = LaunchConfiguration('x_pose', default='-2.0')
    y_pose = LaunchConfiguration('y_pose', default='-0.5')

    # Arguments
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true'
    )

    use_rviz_arg = DeclareLaunchArgument(
        'use_rviz',
        default_value='true',
        description='Launch RViz2 if true'
    )

    x_pose_arg = DeclareLaunchArgument(
        'x_pose',
        default_value='-2.0',
        description='Initial robot X spawn pose'
    )

    y_pose_arg = DeclareLaunchArgument(
        'y_pose',
        default_value='-0.5',
        description='Initial robot Y spawn pose'
    )

    # Tell Gazebo Sim where to find our self-contained model meshes
    set_gz_resource_path = AppendEnvironmentVariable(
        name='GZ_SIM_RESOURCE_PATH',
        value=models_dir
    )

    set_tb3_model_env = SetEnvironmentVariable(
        name='TURTLEBOT3_MODEL',
        value='waffle'
    )

    # 1. Gazebo Sim Server Only (HEADLESS mode: -s -r -> NO Gazebo GUI window loaded)
    gz_server = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={
            'gz_args': ['-r -s -v2 ', world_file],
            'on_exit_shutdown': 'true'
        }.items()
    )

    # 2. Robot State Publisher (using package's self-contained URDF)
    with open(urdf_file, 'r') as f:
        robot_description_content = f.read()

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description_content,
            'use_sim_time': use_sim_time
        }]
    )

    # 3. Spawn TurtleBot3 Waffle model into Gazebo
    spawn_robot = Node(
        package='ros_gz_sim',
        executable='create',
        name='spawn_turtlebot3_waffle',
        arguments=[
            '-file', waffle_sdf,
            '-name', 'waffle',
            '-x', x_pose,
            '-y', y_pose,
            '-z', '0.01'
        ],
        output='screen'
    )

    # 4. ROS-GZ Bridge (Bridges clock, odom, TF, cmd_vel, joint_states, and Intel RealSense camera)
    gz_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='ros_gz_bridge',
        arguments=[
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
            '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
            '/tf@tf2_msgs/msg/TFMessage[gz.msgs.Pose_V',
            '/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
            '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            '/joint_states@sensor_msgs/msg/JointState[gz.msgs.Model',
            '/camera/points@sensor_msgs/msg/PointCloud2[gz.msgs.PointCloudPacked',
            '/camera/depth_image@sensor_msgs/msg/Image[gz.msgs.Image',
            '/camera/image@sensor_msgs/msg/Image[gz.msgs.Image',
        ],
        remappings=[
            ('/camera/points', '/camera/depth/points'),
            ('/camera/image', '/camera/image_raw'),
        ],
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )

    # 5. Traversability Mapping Node
    traversability_node = Node(
        package='traversability_mapping',
        executable='traversability_node',
        name='traversability_node',
        output='screen',
        parameters=[
            params_file,
            {'use_sim_time': use_sim_time}
        ],
        emulate_tty=True
    )

    # 6. RViz2 (unified visualizer with fixed camera view & direct camera feed)
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config],
        parameters=[{'use_sim_time': use_sim_time}],
        condition=IfCondition(use_rviz),
        output='screen'
    )

    return LaunchDescription([
        set_gz_resource_path,
        set_tb3_model_env,
        use_sim_time_arg,
        use_rviz_arg,
        x_pose_arg,
        y_pose_arg,
        gz_server,
        robot_state_publisher,
        spawn_robot,
        gz_bridge,
        traversability_node,
        rviz_node
    ])
