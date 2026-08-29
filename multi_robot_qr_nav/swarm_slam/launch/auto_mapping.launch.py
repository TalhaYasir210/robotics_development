import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node, PushRosNamespace, SetRemap
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    pkg_swarm_slam = get_package_share_directory('swarm_slam')
    pkg_nav2_bringup = get_package_share_directory('nav2_bringup')

    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    namespace = 'tb3_1'

    # --- VERIFY THESE FILENAMES MATCH YOUR SAVED FILES EXACTLY ---
    slam_config = os.path.join(pkg_swarm_slam, 'config', 'mapper_param_online_async.yaml')
    nav2_config = os.path.join(pkg_swarm_slam, 'config', 'nav2_params_slam.yaml')
    explore_config = os.path.join(pkg_swarm_slam, 'config', 'explore.yaml')

    # 1. SLAM Toolbox (Explicit Node to enforce /tf remappings)
    start_slam = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}, slam_config],
        remappings=[
            ('/tf', f'/{namespace}/tf'),
            ('/tf_static', f'/{namespace}/tf_static'),
            ('/scan', f'/{namespace}/scan'),
            ('/map', '/map')
        ]
    )

    # 1b. SLAM Lifecycle Manager (Bonds Disabled)
    start_slam_lifecycle = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_slam',
        output='screen',
        parameters=[
            {'use_sim_time': use_sim_time},
            {'autostart': True},
            {'node_names': ['slam_toolbox']},
            {'bond_timeout': 0.0}
        ]
    )

    # 2. Nav2 Navigation Stack (Wrapped with PushRosNamespace)
    start_nav2 = GroupAction(
        actions=[
            PushRosNamespace(namespace),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(pkg_nav2_bringup, 'launch', 'navigation_launch.py')
                ),
                launch_arguments={
                    'use_sim_time': use_sim_time,
                    'params_file': nav2_config,
                    'namespace': namespace,
                    'use_remappings': 'true',
                    'autostart': 'true',
                    'use_collision_monitor': 'False'
                }.items()
            )
        ]
    )

    # 3. Explore Lite
    start_explore = Node(
        package='explore_lite',
        executable='explore',
        name='explore_node',
        namespace=namespace,
        output='screen',
        parameters=[explore_config, {'use_sim_time': use_sim_time}],
        remappings=[
            ('/tf', f'/{namespace}/tf'),
            ('/tf_static', f'/{namespace}/tf_static')
        ]
    )

    # 4. Robot Localization (EKF) to replace Gazebo TF bridge
    start_ekf = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        namespace=namespace,
        output='screen',
        parameters=[{
            'use_sim_time': use_sim_time,
            'frequency': 50.0,
            'two_d_mode': True,
            'publish_tf': True,
            'odom_frame': f'{namespace}/odom',
            'base_link_frame': f'{namespace}/base_footprint',
            'world_frame': f'{namespace}/odom',
            'odom0': 'odom',
            'odom0_config': [True,  True,  False,
                             False, False, True,
                             True,  True,  False,
                             False, False, True,
                             False, False, False]
        }],
        remappings=[
            ('/tf', f'/{namespace}/tf'),
            ('/tf_static', f'/{namespace}/tf_static')
        ]
    )

    delayed_explore = TimerAction(
        period=12.0,
        actions=[start_explore]
    )

    return LaunchDescription([
        start_ekf,
        start_slam,
        start_slam_lifecycle,
        start_nav2,
        delayed_explore
    ])