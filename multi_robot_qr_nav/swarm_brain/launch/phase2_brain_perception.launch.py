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
    # Role: mapper -> Computes global TF and calls /save_qr
    bot1_perception_node = Node(
        package='swarm_perception',
        executable='qr_tf_projector',
        name='qr_projector_tb3_1',
        parameters=[{'robot_role': 'mapper'}],
        remappings=[
            ('camera/image_raw', '/tb3_1/camera/image_raw')
        ],
        output='screen'
    )

    # 3. Perception Node for Bot 2 (Explorer)
    # Role: explorer -> Queries /get_qr and publishes /tb3_2/initialpose
    bot2_perception_node = Node(
        package='swarm_perception',
        executable='qr_tf_projector',
        name='qr_projector_tb3_2',
        parameters=[{'robot_role': 'explorer'}],
        remappings=[
            ('camera/image_raw', '/tb3_2/camera/image_raw'),
            ('initialpose', '/tb3_2/initialpose')
        ],
        output='screen'
    )

    return LaunchDescription([
        database_node,
        bot1_perception_node,
        bot2_perception_node
    ])
