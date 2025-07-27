cmake_minimum_required(VERSION 3.8)
project(hw_pub_sub)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -std=c++17)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Generate custom messages and services
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/SensorData.msg"
  "msg/NumberStatistics.msg"
  "msg/StringAnalysis.msg"
  "msg/SystemHealth.msg"
  "msg/GeneratorConfig.msg"
  "srv/GetSystemInfo.srv"
  DEPENDENCIES std_msgs geometry_msgs sensor_msgs
)

# Include directories
include_directories(include)

# Random data generator utility library
add_library(random_generator src/utils/random_generator.cpp)
target_include_directories(random_generator PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

# Original examples (maintained for compatibility)
add_executable(talker src/publisher_lambda_function.cpp)
ament_target_dependencies(talker rclcpp std_msgs)

add_executable(listener src/subscriber_lambda_function.cpp)
ament_target_dependencies(listener rclcpp std_msgs)

# Random publishers
add_executable(random_string_publisher src/random_string_publisher.cpp)
target_link_libraries(random_string_publisher random_generator)
ament_target_dependencies(random_string_publisher rclcpp std_msgs)

add_executable(random_number_publisher src/random_number_publisher.cpp)
target_link_libraries(random_number_publisher random_generator)
ament_target_dependencies(random_number_publisher rclcpp std_msgs geometry_msgs)

add_executable(random_sensor_publisher src/random_sensor_publisher.cpp)
target_link_libraries(random_sensor_publisher random_generator)
ament_target_dependencies(random_sensor_publisher rclcpp std_msgs sensor_msgs)

add_executable(random_geometry_publisher src/random_geometry_publisher.cpp)
target_link_libraries(random_geometry_publisher random_generator)
ament_target_dependencies(random_geometry_publisher rclcpp geometry_msgs)

add_executable(enhanced_publisher src/enhanced_publisher.cpp)
target_link_libraries(enhanced_publisher random_generator)
ament_target_dependencies(enhanced_publisher rclcpp std_msgs)

add_executable(system_info_service src/system_info_service.cpp)
ament_target_dependencies(system_info_service rclcpp std_msgs)

# Subscribers
add_executable(string_subscriber src/string_subscriber.cpp)
ament_target_dependencies(string_subscriber rclcpp std_msgs)

add_executable(number_subscriber src/number_subscriber.cpp)
ament_target_dependencies(number_subscriber rclcpp std_msgs)

add_executable(sensor_subscriber src/sensor_subscriber.cpp)
ament_target_dependencies(sensor_subscriber rclcpp std_msgs sensor_msgs)

add_executable(geometry_subscriber src/geometry_subscriber.cpp)
ament_target_dependencies(geometry_subscriber rclcpp geometry_msgs)

# Make sure the custom messages are built before the executables that use them
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

target_link_libraries(random_number_publisher "${cpp_typesupport_target}")
target_link_libraries(random_sensor_publisher "${cpp_typesupport_target}")
target_link_libraries(enhanced_publisher "${cpp_typesupport_target}")
target_link_libraries(system_info_service "${cpp_typesupport_target}")
target_link_libraries(string_subscriber "${cpp_typesupport_target}")
target_link_libraries(number_subscriber "${cpp_typesupport_target}")
target_link_libraries(sensor_subscriber "${cpp_typesupport_target}")
target_link_libraries(geometry_subscriber "${cpp_typesupport_target}")

# Install executables
install(TARGETS
  talker
  listener
  random_string_publisher
  random_number_publisher
  random_sensor_publisher
  random_geometry_publisher
  enhanced_publisher
  system_info_service
  string_subscriber
  number_subscriber
  sensor_subscriber
  geometry_subscriber
  random_generator
  DESTINATION lib/${PROJECT_NAME})

# Install include directory
install(DIRECTORY include/
  DESTINATION include/)

# Install launch files
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch/
  FILES_MATCHING PATTERN "*.launch.py")

# Install config files
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config/
  FILES_MATCHING PATTERN "*.yaml")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_cmake_gtest REQUIRED)
  
  # Linting
  ament_lint_auto_find_test_dependencies()
  
  # Unit tests
  ament_add_gtest(test_random_data test/test_random_data.cpp)
  target_link_libraries(test_random_data random_generator)
  
  ament_add_gtest(test_pub_sub test/test_pub_sub.cpp)
  ament_target_dependencies(test_pub_sub rclcpp std_msgs)
  target_link_libraries(test_pub_sub "${cpp_typesupport_target}")
  
  ament_add_gtest(test_integration test/test_integration.cpp)
  ament_target_dependencies(test_integration rclcpp std_msgs geometry_msgs sensor_msgs)
  target_link_libraries(test_integration "${cpp_typesupport_target}")
endif()

ament_export_dependencies(rosidl_default_runtime)
ament_package()
