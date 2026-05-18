cmake_minimum_required(VERSION 3.16)
project(rosbot_mavlink_bridge LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

# Path to the firmware's generated MAVLink dialect. Single source of truth
# (D4) — the firmware and the bridge consume the same checked-in headers.
set(MAVLINK_DIALECT_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mavlink/dialect/generated/rosbot")
if(NOT EXISTS "${MAVLINK_DIALECT_DIR}/mavlink.h")
  message(FATAL_ERROR
    "MAVLink dialect headers not found at ${MAVLINK_DIALECT_DIR}. "
    "Run `just mavgen` (or the equivalent mavgen.py command in "
    "lib/mavlink/dialect/README.md) to regenerate them.")
endif()

add_library(${PROJECT_NAME} SHARED
  src/bridge_node.cpp
  src/transport/udp_transport.cpp
  src/transport/serial_transport.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${MAVLINK_DIALECT_DIR}
)
ament_target_dependencies(${PROJECT_NAME}
  rclcpp sensor_msgs std_msgs std_srvs
)
# The dialect headers take packed-member addresses internally; silence the
# warning rather than risk turning it into an error in CI.
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-address-of-packed-member)

add_executable(bridge_node src/main.cpp)
target_link_libraries(bridge_node ${PROJECT_NAME})
ament_target_dependencies(bridge_node rclcpp)

install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS bridge_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
