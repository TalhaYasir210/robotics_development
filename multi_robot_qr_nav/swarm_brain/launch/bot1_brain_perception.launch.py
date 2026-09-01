import os
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # 1. Backend Central Database Node
    database_node = Node(
        package='swarm_brain',
        executable='database_node',
        name='swarm_database_node',
        output='screen'
    )

    # 2. Perception Node for Bot 1 (Mapper)
    # Role: mapper -> intercepts QR and drives exactly to it, then calls /save_qr
    bot1_perception_node = Node(
        package='swarm_perception',
        executable='bot1_interceptor',
        name='qr_interceptor_tb3_1',
        namespace='tb3_1',
        parameters=[{
            'robot_name': 'tb3_1',
            'global_frame': 'map',
            'target_distance': 0.20,
            'qr_size': 0.28
        }],
        remappings=[
            ('/tf', '/tb3_1/tf'),
            ('/tf_static', '/tb3_1/tf_static')
        ],
        output='screen'
    )

    # 3. Map Saver Client for Bot 1 (Mapper)
    # Role: Subscribes to /map and sends it to the database every 15s
    map_saver_node = Node(
        package='swarm_brain',
        executable='map_saver_client',
        name='map_saver_tb3_1',
        output='screen'
    )

    return LaunchDescription([
        database_node,
        bot1_perception_node,
        map_saver_node
    ])
