#!/bin/bash
set -e

# Source the base ROS 2 installation
source /opt/ros/jazzy/setup.bash

# Source the local workspace if it has been built
if [ -f /ros2_ws/install/setup.bash ]; then
    source /ros2_ws/install/setup.bash
fi

echo "=========================================================="
echo "          ROS 2 Docker Diagnostic Information             "
echo "=========================================================="
echo "ROS_DOMAIN_ID:      ${ROS_DOMAIN_ID:-'Not set (defaults to 0)'}"
echo "ROS_LOCALHOST_ONLY: ${ROS_LOCALHOST_ONLY:-'0'}"

HOST_IP=$(hostname -I 2>/dev/null || true)
echo "Network IPs:        ${HOST_IP:-'None (Offline)'}"

if [ -z "$(echo -n "$HOST_IP" | tr -d ' ')" ] && [ "${ROS_LOCALHOST_ONLY}" != "1" ]; then
    echo ""
    echo "⚠️ WARNING: No active network IP address detected!"
    echo "If you are running offline, ROS 2 containers will likely fail to"
    echo "communicate. Please set 'ROS_LOCALHOST_ONLY=1' in docker-compose.yml."
fi
echo "=========================================================="

# Execute the command passed in
exec "$@"
