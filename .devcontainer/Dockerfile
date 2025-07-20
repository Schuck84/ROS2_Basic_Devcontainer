FROM osrf/ros:jazzy-desktop

# Add ubuntu user with same UID and GID as your host system, if it doesn't already exist
# Since Ubuntu 24.04, a non-root user is created by default with the name vscode and UID=1000
ARG USERNAME=h2o
ARG USER_UID=1000
ARG USER_GID=1000
ENV RMW_IMPLEMENTATION=rmw_zenoh_cpp
ENV ROS_LOCALHOST_ONLY=1

# Remove existing users/groups with UID/GID 1000 and create h2o user
RUN \
    # Remove any existing user with UID 1000
    if id -u 1000 >/dev/null 2>&1; then \
        existing_user=$(id -nu 1000) && \
        userdel -r $existing_user 2>/dev/null || true; \
    fi && \
    # Remove any existing group with GID 1000
    if getent group 1000 >/dev/null 2>&1; then \
        existing_group=$(getent group 1000 | cut -d: -f1) && \
        groupdel $existing_group 2>/dev/null || true; \
    fi && \
    # Create new group and user
    groupadd --gid $USER_GID $USERNAME && \
    useradd -s /bin/bash --uid $USER_UID --gid $USER_GID -m $USERNAME
    
# Add sudo support for the non-root user
RUN apt-get update && \
    apt-get install -y sudo pip python3-venv

# RUN echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME && \
#     chmod 0440 /etc/sudoers.d/$USERNAME

# Add user to video group to allow access to webcam
RUN sudo usermod --append --groups video $USERNAME

# Update all packages
RUN sudo apt update && sudo apt upgrade -y

# Install Git
RUN sudo apt install -y git

################################
## ADD ANY CUSTOM SETUP BELOW ##
################################

RUN sudo apt install -y ros-jazzy-rmw-zenoh-cpp

# Switch from root to user
USER $USERNAME

# Rosdep update
RUN rosdep update

# Source the ROS setup file
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> ~/.bashrc