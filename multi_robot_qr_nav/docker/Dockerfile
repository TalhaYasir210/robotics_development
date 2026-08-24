FROM osrf/ros:jazzy-desktop

ENV DEBIAN_FRONTEND=noninteractive

# Install basic build tools and OpenCV headers
RUN apt-get update && apt-get install -y \
    python3-colcon-common-extensions \
    libopencv-dev \
    ros-jazzy-ros-gz \
    ros-jazzy-navigation2 \
    ros-jazzy-nav2-bringup \
    && rm -rf /var/lib/apt/lists/*

# Set up the workspace
WORKDIR /ros2_ws

# Copy source code into the exact target path
COPY . /ros2_ws/src/robotics_development/multi_robot_qr_nav/

# Update rosdep and install all package dependencies
RUN rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y && \
    rm -rf /var/lib/apt/lists/*

# Auto-source ROS 2 on container entry
RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc
