#!/usr/bin/env python3

import launch
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # Get package directory
    package_dir = get_package_share_directory('hw_pub_sub')
    
    # Launch arguments
    log_level_arg = DeclareLaunchArgument(
        'log_level',
        default_value='info',
        description='Log level'
    )
    
    # Original talker and listener
    talker = Node(
        package='hw_pub_sub',
        executable='talker',
        name='talker',
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    listener = Node(
        package='hw_pub_sub',
        executable='listener',
        name='listener',
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        output='screen'
    )
    
    return LaunchDescription([
        log_level_arg,
        LogInfo(msg=['Starting Basic Talker/Listener Example']),
        talker,
        listener,
    ])