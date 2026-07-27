#!/usr/bin/env python3

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_autonomous_nav = get_package_share_directory('autonomous_navigation')
    pkg_slam_toolbox = get_package_share_directory('slam_toolbox')
    pkg_nav2_bringup = get_package_share_directory('nav2_bringup')

    # Launch Arguments
    env_arg = DeclareLaunchArgument(
        'env',
        default_value='office',
        description='Environment to launch: "office" or "warehouse"'
    )
    env_config = LaunchConfiguration('env')

    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true'
    )

    # Configuration Files
    slam_params_file = os.path.join(
        pkg_autonomous_nav, 'config', 'mapper_params_online_async.yaml')
    nav2_params_file = os.path.join(pkg_autonomous_nav, 'config', 'nav2_params_slam.yaml')
    explore_params_file = os.path.join(pkg_autonomous_nav, 'config', 'explore.yaml')
    rviz_config_file = os.path.join(pkg_autonomous_nav, 'rviz', 'auto_slam.rviz')

    # Environment Launch
    # We dynamically select office_env.launch.py or warehouse_env.launch.py
    env_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(pkg_autonomous_nav, 'launch', ''),
            env_config,
            '_env.launch.py'
        ])
    )

    # SLAM Toolbox Launch
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_slam_toolbox, 'launch', 'online_async_launch.py')
        ),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'slam_params_file': slam_params_file
        }.items()
    )

    from launch.actions import GroupAction
    from launch_ros.actions import SetRemap

    # Wrap Nav2 in a GroupAction to remap its output to cmd_vel_nav
    nav2_launch = GroupAction([
        SetRemap(src='cmd_vel', dst='cmd_vel_nav'),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_nav2_bringup, 'launch', 'navigation_launch.py')
            ),
            launch_arguments={
                'use_sim_time': use_sim_time,
                'params_file': nav2_params_file,
                'autostart': 'true'
            }.items()
        )
    ])

    # Add the Velocity Smoother node to intercept cmd_vel_nav and publish smoothed cmd_vel
    velocity_smoother_node = Node(
        package='nav2_velocity_smoother',
        executable='velocity_smoother',
        name='velocity_smoother',
        output='screen',
        parameters=[nav2_params_file, {'use_sim_time': use_sim_time}],
        remappings=[
            ('cmd_vel', 'cmd_vel_nav'),
            ('cmd_vel_smoothed', 'cmd_vel')
        ]
    )

    # Explore Lite Node
    # Wait a few seconds for Nav2 to be fully up before starting explore
    explore_node = TimerAction(
        period=10.0,
        actions=[
            Node(
                package='explore_lite',
                executable='explore',
                name='explore_node',
                output='screen',
                parameters=[explore_params_file, {'use_sim_time': use_sim_time}]
            )
        ]
    )

    # RViz Node
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen'
    )

    ld = LaunchDescription()

    ld.add_action(env_arg)
    ld.add_action(use_sim_time_arg)
    ld.add_action(env_launch)
    ld.add_action(slam_launch)
    ld.add_action(nav2_launch)
    ld.add_action(velocity_smoother_node)
    ld.add_action(explore_node)
    ld.add_action(rviz_node)

    return ld
