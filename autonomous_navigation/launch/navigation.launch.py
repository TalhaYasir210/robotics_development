#!/usr/bin/env python3

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.actions import EmitEvent
from launch.events import Shutdown
from launch_ros.actions import Node


def generate_launch_description():
    pkg_autonomous_nav = get_package_share_directory('autonomous_navigation')
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

    map_yaml_file = LaunchConfiguration('map')
    map_arg = DeclareLaunchArgument(
        'map',
        description='Full path to map yaml file to load'
    )

    nav2_params_file = os.path.join(pkg_autonomous_nav, 'config', 'nav2_params_slam.yaml')
    rviz_config_file = os.path.join(pkg_autonomous_nav, 'rviz', 'auto_slam.rviz') # Reusing rviz config

    # Environment Launch removed (Gazebo is launched separately)

    # Nav2 Bringup (Includes AMCL and Map Server)
    nav2_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_nav2_bringup, 'launch', 'bringup_launch.py')
        ),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'map': map_yaml_file,
            'params_file': nav2_params_file,
            'autostart': 'true'
        }.items()
    )

    # RViz Node
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen',
        on_exit=EmitEvent(event=Shutdown())
    )

    ld = LaunchDescription()

    ld.add_action(env_arg)
    ld.add_action(use_sim_time_arg)
    ld.add_action(map_arg)
    ld.add_action(nav2_launch)
    ld.add_action(rviz_node)

    return ld
