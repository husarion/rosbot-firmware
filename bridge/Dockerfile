# Build + run image for rosbot_mavlink_bridge. Builds from the same source
# tree the CI matrix uses (no surprise divergence). Default base is jazzy;
# pass `--build-arg ROS_DISTRO=humble` to build the matching humble image.
#
# Build context must be the repo root so the dialect headers in
# lib/mavlink/dialect/generated/ are reachable via the relative path the
# bridge's CMakeLists.txt expects.
ARG ROS_DISTRO=jazzy
FROM ros:${ROS_DISTRO}-ros-base

# Dependencies for colcon + rosdep. The base image already carries rclcpp +
# common interfaces, but we still need build tooling.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        python3-colcon-common-extensions \
        python3-rosdep \
    && rm -rf /var/lib/apt/lists/*

# Stage the bridge package and the dialect headers it includes by relative
# path. Keeping the same layout the developer uses avoids special-casing
# the build for the container.
WORKDIR /ws/src
COPY bridge/rosbot_mavlink_bridge ./rosbot_mavlink_bridge
# The bridge CMakeLists.txt resolves dialect headers via
# ../../lib/mavlink/dialect/generated. Mirror that path under /ws/.
COPY lib/mavlink/dialect /ws/lib/mavlink/dialect

WORKDIR /ws
RUN . /opt/ros/${ROS_DISTRO}/setup.sh && \
    rosdep install --from-paths src --ignore-src -y && \
    colcon build --packages-select rosbot_mavlink_bridge \
        --cmake-args -DCMAKE_BUILD_TYPE=Release

# Source overlay on every container launch so `ros2 launch ...` works
# without the caller having to set up the env.
COPY bridge/docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["ros2", "launch", "rosbot_mavlink_bridge", "rosbot_xl.launch.py", "namespace:=rosbot"]
