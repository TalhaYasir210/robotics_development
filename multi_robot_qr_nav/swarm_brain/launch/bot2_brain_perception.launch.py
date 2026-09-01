import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    # 1. Explorer Node for Bot 2
    # Role: blindly wanders, servos to QR, downloads map, triggers AMCL
    bot2_explorer_node = Node(
        package='swarm_perception',
        executable='explorer_node',
        name='explorer_tb3_2',
        namespace='tb3_2',
        parameters=[{
            'robot_name': 'tb3_2',
            'global_frame': 'map',
            'target_distance': 0.50,
            'qr_size': 0.28
        }],
        remappings=[
            ('/tf', '/tb3_2/tf'),
            ('/tf_static', '/tb3_2/tf_static')
        ],
        output='screen'
    )

    # 2. Navigation Stack for Bot 2
    pkg_swarm_perception = get_package_share_directory('swarm_perception')
    bot2_navigation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_swarm_perception, 'launch', 'bot2_navigation.launch.py')
        )
    )

    # 3. Nav Logger for Bot 2
    bot2_nav_logger = Node(
        package='swarm_brain',
        executable='nav_logger.py',
        name='nav_logger',
        namespace='tb3_2',
        output='screen'
    )

    return LaunchDescription([
        bot2_explorer_node,
        bot2_navigation,
        bot2_nav_logger
    ])
