import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node, PushRosNamespace
from launch.substitutions import LaunchConfiguration
from nav2_common.launch import RewrittenYaml

def generate_launch_description():
    pkg_swarm_perception = get_package_share_directory('swarm_perception')
    pkg_nav2_bringup = get_package_share_directory('nav2_bringup')

    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    namespace = 'tb3_2'
    nav2_config = os.path.join(pkg_swarm_perception, 'config', 'nav2_params_bot2.yaml')

    configured_params = RewrittenYaml(
        source_file=nav2_config,
        root_key=namespace,
        param_rewrites={},
        convert_types=True
    )

    # 1. Robot Localization (EKF)
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

    # 2. AMCL Node
    start_amcl = Node(
        package='nav2_amcl',
        executable='amcl',
        name='amcl',
        namespace=namespace,
        output='screen',
        parameters=[configured_params, {'use_sim_time': use_sim_time}],
        remappings=[
            ('/tf', f'/{namespace}/tf'),
            ('/tf_static', f'/{namespace}/tf_static'),
            ('/map', f'/{namespace}/map')
        ]
    )

    # 3. Lifecycle Manager for AMCL
    start_amcl_lifecycle = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_localization',
        namespace=namespace,
        output='screen',
        parameters=[
            {'use_sim_time': use_sim_time},
            {'autostart': False},
            {'node_names': ['amcl']},
            {'bond_timeout': 0.0}
        ]
    )

    # 4. Nav2 Navigation Stack
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
                    'autostart': 'false'
                }.items()
            )
        ]
    )

    return LaunchDescription([
        start_ekf,
        start_amcl,
        start_amcl_lifecycle,
        start_nav2
    ])
