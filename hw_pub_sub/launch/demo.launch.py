#!/usr/bin/env python3

import launch
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration, TextSubstitution
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # Get package directory
    package_dir = get_package_share_directory('hw_pub_sub')
    
    # Declare launch arguments
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation time'
    )
    
    log_level_arg = DeclareLaunchArgument(
        'log_level',
        default_value='info',
        description='Log level (debug, info, warn, error)'
    )
    
    config_file_arg = DeclareLaunchArgument(
        'config_file',
        default_value=os.path.join(package_dir, 'config', 'params.yaml'),
        description='Path to parameter file'
    )
    
    # Publishers
    random_string_publisher = Node(
        package='hw_pub_sub',
        executable='random_string_publisher',
        name='random_string_publisher',
        parameters=[LaunchConfiguration('config_file')],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    random_number_publisher = Node(
        package='hw_pub_sub',
        executable='random_number_publisher',
        name='random_number_publisher',
        parameters=[LaunchConfiguration('config_file')],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    random_sensor_publisher = Node(
        package='hw_pub_sub',
        executable='random_sensor_publisher',
        name='random_sensor_publisher',
        parameters=[LaunchConfiguration('config_file')],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    random_geometry_publisher = Node(
        package='hw_pub_sub',
        executable='random_geometry_publisher',
        name='random_geometry_publisher',
        parameters=[LaunchConfiguration('config_file')],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    # Subscribers
    string_subscriber = Node(
        package='hw_pub_sub',
        executable='string_subscriber',
        name='string_subscriber',
        parameters=[LaunchConfiguration('config_file')],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    number_subscriber = Node(
        package='hw_pub_sub',
        executable='number_subscriber',
        name='number_subscriber',
        parameters=[LaunchConfiguration('config_file')],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    sensor_subscriber = Node(
        package='hw_pub_sub',
        executable='sensor_subscriber',
        name='sensor_subscriber',
        parameters=[LaunchConfiguration('config_file')],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    return LaunchDescription([
        use_sim_time_arg,
        log_level_arg,
        config_file_arg,
        LogInfo(msg=['Starting ROS2 Multi-Topic Communication System']),
        random_string_publisher,
        random_number_publisher,
        random_sensor_publisher,
        random_geometry_publisher,
        string_subscriber,
        number_subscriber,
        sensor_subscriber,
        LogInfo(msg=['All nodes launched successfully'])
    ])