import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

def generate_launch_description():
    pkg_swarm_brain = get_package_share_directory('swarm_brain')

    # Include Bot 1 Brain + Perception
    bot1_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_swarm_brain, 'launch', 'bot1_brain_perception.launch.py')
        )
    )

    # Include Bot 2 Brain + Perception
    bot2_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_swarm_brain, 'launch', 'bot2_brain_perception.launch.py')
        )
    )

    return LaunchDescription([
        bot1_launch,
        bot2_launch
    ])
