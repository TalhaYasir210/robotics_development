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

    # 3. Perception Node for Bot 2 (Explorer)
    # Role: explorer -> Queries /get_qr and publishes /tb3_2/initialpose
    bot2_perception_node = Node(
        package='swarm_perception',
        executable='bot2_projector',
        name='qr_projector_tb3_2',
        parameters=[{'robot_role': 'explorer'}],
        remappings=[
            ('camera/image_raw', '/tb3_2/camera/image_raw'),
            ('camera/camera_info', '/tb3_2/camera/camera_info'),
            ('initialpose', '/tb3_2/initialpose'),
            ('/tf', '/tb3_2/tf'),
            ('/tf_static', '/tb3_2/tf_static')
        ],
        output='screen'
    )

    return LaunchDescription([
        database_node,
        bot1_perception_node,
        bot2_perception_node
    ])
