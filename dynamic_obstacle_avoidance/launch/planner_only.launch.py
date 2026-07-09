import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import SetRemap, Node

def generate_launch_description():
    nav2_bringup_dir = FindPackageShare('nav2_bringup').find('nav2_bringup')
    my_pkg_dir = FindPackageShare('dynamic_obstacle_avoidance').find('dynamic_obstacle_avoidance')
    
    params_file = os.path.join(my_pkg_dir, 'config', 'nav2_params.yaml')

    return LaunchDescription([
        # Group Nav2 and remap its output cmd_vel to cmd_vel_nav
        GroupAction(
            actions=[
                SetRemap(src='/cmd_vel', dst='/cmd_vel_nav'),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(
                        os.path.join(nav2_bringup_dir, 'launch', 'navigation_launch.py')
                    ),
                    launch_arguments={
                        'params_file': params_file,
                        'use_sim_time': 'true',
                        'use_velocity_smoother': 'true',
                        'use_collision_monitor': 'true'
                    }.items()
                )
            ]
        ),
        
        # Launch our dynamic obstacle detector node to act as gatekeeper
        Node(
            package='dynamic_obstacle_avoidance',
            executable='dynamic_obstacle_detector_node',
            name='dynamic_obstacle_detector_node',
            output='screen',
            parameters=[
                {'danger_zone_x_min': 0.1},
                {'danger_zone_x_max': 1.0},
                {'danger_zone_y_width': 0.7},
                {'dynamic_velocity_threshold': 0.15},
                {'cluster_tolerance': 0.3}
            ]
        )
    ])
