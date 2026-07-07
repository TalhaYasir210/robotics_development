import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import SetRemap

def generate_launch_description():
    nav2_bringup_dir = FindPackageShare('nav2_bringup').find('nav2_bringup')
    my_pkg_dir = FindPackageShare('dynamic_obstacle_avoidance').find('dynamic_obstacle_avoidance')
    
    params_file = os.path.join(my_pkg_dir, 'config', 'nav2_params.yaml')

    return LaunchDescription([
        # This is the magic! It intercepts Nav2's velocity commands and sends them
        # to a dummy topic so they NEVER reach the robot's wheels. This guarantees 
        # that only your custom Velocity Controller handles the driving!
        SetRemap(src='/cmd_vel', dst='/cmd_vel_dummy_nav2'),
        
        # Launch the standard Nav2 stack with our custom parameters
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(nav2_bringup_dir, 'launch', 'navigation_launch.py')
            ),
            launch_arguments={
                'params_file': params_file,
                'use_sim_time': 'true'
            }.items()
        )
    ])
