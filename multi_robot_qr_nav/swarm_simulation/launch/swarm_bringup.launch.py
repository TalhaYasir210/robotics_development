import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command
from launch_ros.actions import Node

def generate_launch_description():
    pkg_swarm_sim = get_package_share_directory('swarm_simulation')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')

    # Automatically set the Gazebo resource path so it finds custom models and worlds
    # We point it to the models directory, and the parent share directory for mesh resolution
    gazebo_resource_path = SetEnvironmentVariable(
        name='GZ_SIM_RESOURCE_PATH',
        value=f"{os.path.join(pkg_swarm_sim, 'models')}:{os.path.join(pkg_swarm_sim, '..')}"
    )

    world_file = os.path.join(pkg_swarm_sim, 'worlds', 'multi_room.sdf')
    xacro_file = os.path.join(pkg_swarm_sim, 'urdf', 'tb3_namespaced.urdf.xacro')

    # 1. Launch Gazebo Harmonic with the multi-room world
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': f'-r {world_file}'}.items(),
    )

    # 3. Define spawn locations for Bot 1 (Mapper) and Bot 2 (Lost Explorer)
    robots = [
        {'name': 'tb3_1', 'x': '0.0', 'y': '0.0', 'z': '0.03'},  # Spawned at Origin (Center)
        {'name': 'tb3_2', 'x': '-3.0', 'y': '3.0', 'z': '0.03'}   # Spawned inside Room 1 (Top-Left)
    ]

    # 2. Bridge the simulation clock AND robot topics
    bridge_args = ['/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock']
    
    # Dynamically generate bridge arguments for both robots
    for robot in robots:
        name = robot['name']
        bridge_args.extend([
            # cmd_vel (ROS to Gazebo)
            f'/{name}/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
            # Odometry (Gazebo to ROS)
            f'/{name}/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
            # LiDAR (Gazebo to ROS)
            f'/{name}/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            # Camera (Gazebo to ROS)
            f'/{name}/camera/image_raw@sensor_msgs/msg/Image[gz.msgs.Image',
            f'/{name}/camera/camera_info@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo',
        ])

    gz_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=bridge_args,
        parameters=[{'use_sim_time': True}],
        output='screen'
    )

    spawn_actions = []
    last_spawn_node = None

    for robot in robots:
        name = robot['name']
        
        robot_desc_command = Command(['xacro ', xacro_file, ' robot_name:=', name])
        
        rsp_node = Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            namespace=name,
            output='screen',
            parameters=[{
                'robot_description': robot_desc_command,
                'use_sim_time': True
            }],
            remappings=[
                ('/clock', '/clock'),
                ('/tf', f'/{name}/tf'),
                ('/tf_static', f'/{name}/tf_static')
            ]
        )
        
        spawn_node = Node(
            package='ros_gz_sim',
            executable='create',
            arguments=[
                '-name', name,
                '-topic', f'/{name}/robot_description',
                '-x', robot['x'],
                '-y', robot['y'],
                '-z', robot['z']
            ],
            output='screen'
        )
        
        spawn_actions.append(rsp_node)
        spawn_actions.append(spawn_node)
        last_spawn_node = spawn_node

    # Deterministically start bridge ONLY after the last robot has successfully spawned
    bridge_event = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=last_spawn_node,
            on_exit=[gz_bridge]
        )
    )

    return LaunchDescription([
        gazebo_resource_path,
        gazebo,
        *spawn_actions,
        bridge_event
    ])