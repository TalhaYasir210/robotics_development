#!/bin/bash
set -e

# Source the base ROS 2 installation
source /opt/ros/jazzy/setup.bash

# Source the local workspace if it has been built
if [ -f /ros2_ws/install/setup.bash ]; then
    source /ros2_ws/install/setup.bash
fi

# Execute the command passed in
exec "$@"
