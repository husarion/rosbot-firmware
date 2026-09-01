^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rosbot_mavlink_bridge
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Forthcoming
-----------
* Initial release. Bridge node that translates the firmware's MAVLink
  wire protocol into the same ROS 2 API the micro-ROS agent exposes, so
  downstream consumers (e.g. rosbot_ros) see byte-identical topics and
  services.
