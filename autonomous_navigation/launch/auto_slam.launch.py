#!/usr/bin/env python3

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction
from launch.conditions import LaunchConfigurationEquals
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch.actions import EmitEvent
from launch.events import Shutdown
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

    mode_arg = DeclareLaunchArgument(
        'mode',
        default_value='auto',
        description='Mode of exploration: "auto" or "manual"'
    )
    mode_config = LaunchConfiguration('mode')

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
    rviz_auto_config = os.path.join(pkg_autonomous_nav, 'rviz', 'auto_slam.rviz')
    rviz_manual_config = os.path.join(pkg_autonomous_nav, 'rviz', 'manual_slam.rviz')

    # Environment Launch removed (Gazebo is launched separately)
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
    nav2_launch = GroupAction(
        condition=LaunchConfigurationEquals('mode', 'auto'),
        actions=[
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

    # Nav2 Velocity Smoother (only for auto mode)
    velocity_smoother_node = Node(
        condition=LaunchConfigurationEquals('mode', 'auto'),
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
        condition=LaunchConfigurationEquals('mode', 'auto'),
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

    # RViz Node (Auto)
    rviz_node_auto = Node(
        condition=LaunchConfigurationEquals('mode', 'auto'),
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_auto_config],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen',
        on_exit=EmitEvent(event=Shutdown())
    )
    
    # RViz Node (Manual)
    rviz_node_manual = Node(
        condition=LaunchConfigurationEquals('mode', 'manual'),
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_manual_config],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen',
        on_exit=EmitEvent(event=Shutdown())
    )

    # Simple Velocity Smoother (only for manual mode)
    simple_velocity_smoother_node = Node(
        condition=LaunchConfigurationEquals('mode', 'manual'),
        package='autonomous_navigation',
        executable='simple_velocity_smoother',
        name='simple_velocity_smoother',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )

    ld = LaunchDescription()

    ld.add_action(env_arg)
    ld.add_action(mode_arg)
    ld.add_action(use_sim_time_arg)
    ld.add_action(slam_launch)
    ld.add_action(nav2_launch)
    ld.add_action(velocity_smoother_node)
    ld.add_action(explore_node)
    ld.add_action(rviz_node_auto)
    ld.add_action(rviz_node_manual)
    ld.add_action(simple_velocity_smoother_node)

    return ld
