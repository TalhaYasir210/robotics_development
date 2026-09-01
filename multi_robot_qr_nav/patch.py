import re

with open('/home/deadsec/ros2_ws/src/robotics_development/multi_robot_qr_nav/swarm_simulation/launch/swarm_bringup.launch.py', 'r') as f:
    content = f.read()

# Remove RegisterEventHandler block
content = re.sub(r'# Deterministically start bridge ONLY after the last robot has successfully spawned\n\s+bridge_event = RegisterEventHandler\(\n\s+event_handler=OnProcessExit\(\n\s+target_action=last_spawn_node,\n\s+on_exit=\[gz_bridge\]\n\s+\)\n\s+\)\n', '', content)

# Replace bridge_event with gz_bridge in LaunchDescription
content = re.sub(r'\*spawn_actions,\n\s+bridge_event,\n', '*spawn_actions,\n        gz_bridge,\n', content)

with open('/home/deadsec/ros2_ws/src/robotics_development/multi_robot_qr_nav/swarm_simulation/launch/swarm_bringup.launch.py', 'w') as f:
    f.write(content)
