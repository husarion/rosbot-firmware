# Copyright 2026 Husarion sp. z o.o.
# Licensed under the Apache License, Version 2.0 (the "License").

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg = get_package_share_directory("rosbot_mavlink_bridge")
    cfg = os.path.join(pkg, "config", "rosbot_xl.yaml")
    ns = LaunchConfiguration("namespace")
    return LaunchDescription([
        DeclareLaunchArgument("namespace", default_value=""),
        Node(
            # Note: no `name=` override — the bridge_node constructor sets
            # the node name to "rosbot_mcu" to preserve API parity with the
            # micro-ROS firmware (§10.1).
            package="rosbot_mavlink_bridge",
            executable="bridge_node",
            parameters=[cfg, {"ros_namespace": ns}],
            output="screen",
            emulate_tty=True,
        ),
    ])
