import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Find your package directory
    pkg_dir = get_package_share_directory('swarm_slam')
    
    # Path to your configuration file
    config_file = os.path.join(pkg_dir, 'config', 'slam_toolbox_mapper.yaml')
    
    # Define the SLAM Toolbox Node
    slam_node = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            {'use_sim_time': True},
            config_file
        ],
        remappings=[
            ('/tf', '/tb3_1/tf'),
            ('/tf_static', '/tb3_1/tf_static'),
            ('/scan', '/tb3_1/scan'),
            ('/map', '/map')
        ]
    )

    # 2. The Auto-Wakeup Manager (Now with bonds disabled!)
    lifecycle_manager = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_slam',
        output='screen',
        parameters=[
            {'use_sim_time': True},
            {'autostart': True},
            {'node_names': ['slam_toolbox']},
            {'bond_timeout': 0.0}
        ]
    )

    return LaunchDescription([
        slam_node,
        lifecycle_manager
    ])
