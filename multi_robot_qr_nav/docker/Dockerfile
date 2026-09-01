FROM osrf/ros:jazzy-desktop

ENV DEBIAN_FRONTEND=noninteractive

# Install common dependencies
RUN apt-get update && apt-get install -y \
    python3-colcon-common-extensions \
    libopencv-dev \
    ros-jazzy-ros-gz \
    ros-jazzy-navigation2 \
    ros-jazzy-nav2-bringup \
    libzbar-dev \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws

# Copy Python requirements and install
COPY requirements.txt /ros2_ws/
RUN pip3 install -r requirements.txt --break-system-packages || true

# Copy only the source code
COPY ./ /ros2_ws/src/robotics_development/multi_robot_qr_nav/

# Install ROS dependencies using rosdep
RUN rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y || true

# ARG to specify which packages to build for this specific container
ARG BUILD_PACKAGES=""
RUN if [ -n "$BUILD_PACKAGES" ]; then \
    bash -c "source /opt/ros/jazzy/setup.bash && colcon build --packages-up-to $BUILD_PACKAGES"; \
    else \
    bash -c "source /opt/ros/jazzy/setup.bash && colcon build"; \
    fi

# Copy entrypoint script
COPY docker/entrypoint.sh /
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
