#!/usr/bin/env python3

"""
Launch the traversability_node standalone.

Launches the traversability_node independently (e.g., for physical hardware deployment
or custom multi-robot setups) with configurable params_file and use_sim_time arguments.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_share = get_package_share_directory('traversability_mapping')
    default_params_file = os.path.join(pkg_share, 'config', 'params.yaml')

    params_file_arg = DeclareLaunchArgument(
        'params_file',
        default_value=default_params_file,
        description='Full path to the ROS 2 parameters YAML file'
    )

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation clock (/clock) if true'
    )

    traversability_node = Node(
        package='traversability_mapping',
        executable='traversability_node',
        name='traversability_node',
        output='screen',
        parameters=[
            LaunchConfiguration('params_file'),
            {'use_sim_time': LaunchConfiguration('use_sim_time')}
        ],
        emulate_tty=True
    )

    return LaunchDescription([
        params_file_arg,
        use_sim_time_arg,
        traversability_node
    ])
