#!/bin/bash
# Source ROS 2 + the bridge overlay, then exec the command. Used by the
# bridge container.
set -e
source "/opt/ros/${ROS_DISTRO}/setup.bash"
source /ws/install/setup.bash
exec "$@"
