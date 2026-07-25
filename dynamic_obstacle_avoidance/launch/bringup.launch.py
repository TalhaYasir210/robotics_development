import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    # Directories
    pkg_dynamic_obstacle = FindPackageShare('dynamic_obstacle_avoidance').find('dynamic_obstacle_avoidance')
    pkg_turtlebot3_gazebo = FindPackageShare('turtlebot3_gazebo').find('turtlebot3_gazebo')
    pkg_turtlebot3_navigation2 = FindPackageShare('turtlebot3_navigation2').find('turtlebot3_navigation2')
    
    # Paths
    map_yaml_file = os.path.join(pkg_dynamic_obstacle, 'maps', 'map.yaml')
    rviz_config_file = os.path.join(pkg_turtlebot3_navigation2, 'rviz', 'tb3_navigation2.rviz')

    return LaunchDescription([
        # 1. Launch Gazebo with TurtleBot3 World
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_turtlebot3_gazebo, 'launch', 'turtlebot3_world.launch.py')
            )
        ),
        
        # 2. Map Server (to load our packaged map)
        Node(
            package='nav2_map_server',
            executable='map_server',
            name='map_server',
            output='screen',
            parameters=[{'yaml_filename': map_yaml_file}, {'use_sim_time': True}]
        ),
        
        # Lifecycle manager to activate the map_server
        Node(
            package='nav2_lifecycle_manager',
            executable='lifecycle_manager',
            name='lifecycle_manager_map',
            output='screen',
            parameters=[{'use_sim_time': True},
                        {'autostart': True},
                        {'node_names': ['map_server']}]
        ),

        # 3. Static Transform Publisher (map -> odom offset)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='static_transform_publisher',
            arguments=['-2.0', '-0.5', '0', '0', '0', '0', 'map', 'odom'],
            output='screen'
        ),

        # 4. Include our planner launch file (Nav2 + Obstacle Node + Obstacle Bot)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_dynamic_obstacle, 'launch', 'planner_only.launch.py')
            )
        ),

        # 5. Passive Tracking Logger Node
        Node(
            package='dynamic_obstacle_avoidance',
            executable='passive_tracking_logger_node',
            name='passive_tracking_logger_node',
            output='screen',
            parameters=[{'use_sim_time': True}]
        ),

        # 6. RViz2 for visualization
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', rviz_config_file],
            parameters=[{'use_sim_time': True}],
            output='screen'
        )
    ])
