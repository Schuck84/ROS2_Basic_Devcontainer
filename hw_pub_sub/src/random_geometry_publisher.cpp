#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "hw_pub_sub/utils/random_generator.hpp"

using namespace std::chrono_literals;

class RandomGeometryPublisher : public rclcpp::Node
{
public:
  RandomGeometryPublisher()
  : Node("random_geometry_publisher"), 
    count_(0),
    generator_(std::random_device{}())
  {
    // Declare parameters
    this->declare_parameter("publish_rate_hz", 5.0);
    this->declare_parameter("motion_type", "random_walk"); // random_walk, circular, linear
    this->declare_parameter("motion_scale", 1.0);
    this->declare_parameter("frame_id", "base_link");
    
    // Get parameters
    double rate = this->get_parameter("publish_rate_hz").as_double();
    motion_type_ = this->get_parameter("motion_type").as_string();
    motion_scale_ = this->get_parameter("motion_scale").as_double();
    frame_id_ = this->get_parameter("frame_id").as_string();
    
    // Create publishers with QoS
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    
    pose_publisher_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/random_poses", qos);
    twist_publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/random_velocities", qos);
    
    // Create timer
    auto timer_period = std::chrono::duration<double>(1.0 / rate);
    timer_ = this->create_wall_timer(
      timer_period, 
      std::bind(&RandomGeometryPublisher::timer_callback, this)
    );
    
    // Initialize position
    current_x_ = 0.0;
    current_y_ = 0.0;
    current_z_ = 0.0;
    current_yaw_ = 0.0;
    
    RCLCPP_INFO(this->get_logger(), 
      "RandomGeometryPublisher started - Motion: %s, Rate: %.1f Hz", 
      motion_type_.c_str(), rate);
  }

private:
  void timer_callback()
  {
    // Generate pose
    auto pose_msg = geometry_msgs::msg::PoseStamped();
    pose_msg.header.stamp = this->get_clock()->now();
    pose_msg.header.frame_id = frame_id_;
    
    // Generate twist (velocity)
    auto twist_msg = geometry_msgs::msg::Twist();
    
    if (motion_type_ == "random_walk") {
      generateRandomWalk(pose_msg, twist_msg);
    } else if (motion_type_ == "circular") {
      generateCircularMotion(pose_msg, twist_msg);
    } else if (motion_type_ == "linear") {
      generateLinearMotion(pose_msg, twist_msg);
    } else {
      generateRandomPose(pose_msg, twist_msg);
    }
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing Pose: [%.2f, %.2f, %.2f], Twist: [%.2f, %.2f, %.2f]", 
      pose_msg.pose.position.x, pose_msg.pose.position.y, pose_msg.pose.position.z,
      twist_msg.linear.x, twist_msg.linear.y, twist_msg.angular.z);
    
    pose_publisher_->publish(pose_msg);
    twist_publisher_->publish(twist_msg);
    count_++;
  }
  
  void generateRandomWalk(geometry_msgs::msg::PoseStamped& pose, geometry_msgs::msg::Twist& twist)
  {
    // Random walk motion
    double dx = generator_.generateNormal(0.0, 0.1) * motion_scale_;
    double dy = generator_.generateNormal(0.0, 0.1) * motion_scale_;
    double dz = generator_.generateNormal(0.0, 0.05) * motion_scale_;
    double dyaw = generator_.generateNormal(0.0, 0.1) * motion_scale_;
    
    current_x_ += dx;
    current_y_ += dy;
    current_z_ += dz;
    current_yaw_ += dyaw;
    
    // Keep z above ground
    if (current_z_ < 0.0) current_z_ = 0.0;
    
    setPoseFromState(pose);
    
    // Set velocities
    twist.linear.x = dx * 10.0; // Scale for velocity
    twist.linear.y = dy * 10.0;
    twist.linear.z = dz * 10.0;
    twist.angular.z = dyaw * 10.0;
  }
  
  void generateCircularMotion(geometry_msgs::msg::PoseStamped& pose, geometry_msgs::msg::Twist& twist)
  {
    // Circular motion
    double time = count_ * 0.1; // Assume 10Hz for time base
    double radius = 2.0 * motion_scale_;
    
    current_x_ = radius * std::cos(time);
    current_y_ = radius * std::sin(time);
    current_z_ = 0.5 + 0.3 * std::sin(time * 2); // Oscillate in z
    current_yaw_ = time;
    
    setPoseFromState(pose);
    
    // Set velocities for circular motion
    twist.linear.x = -radius * std::sin(time) * 0.1;
    twist.linear.y = radius * std::cos(time) * 0.1;
    twist.linear.z = 0.6 * std::cos(time * 2) * 0.1;
    twist.angular.z = 0.1;
  }
  
  void generateLinearMotion(geometry_msgs::msg::PoseStamped& pose, geometry_msgs::msg::Twist& twist)
  {
    // Linear motion with direction changes
    static int direction = 1;
    static int step_count = 0;
    
    current_x_ += direction * 0.05 * motion_scale_;
    
    if (step_count++ > 100) { // Change direction every 100 steps
      direction *= -1;
      step_count = 0;
      current_yaw_ += M_PI; // Turn around
    }
    
    setPoseFromState(pose);
    
    twist.linear.x = direction * 0.5 * motion_scale_;
    twist.linear.y = 0.0;
    twist.linear.z = 0.0;
    twist.angular.z = 0.0;
  }
  
  void generateRandomPose(geometry_msgs::msg::PoseStamped& pose, geometry_msgs::msg::Twist& twist)
  {
    // Completely random pose
    auto pose_data = generator_.generatePose3D();
    current_x_ = pose_data[0] * motion_scale_;
    current_y_ = pose_data[1] * motion_scale_;
    current_z_ = std::abs(pose_data[2]) * motion_scale_; // Keep positive
    current_yaw_ = pose_data[5];
    
    setPoseFromState(pose);
    
    // Random velocities
    twist.linear.x = generator_.generateNormal(0.0, 0.5);
    twist.linear.y = generator_.generateNormal(0.0, 0.5);
    twist.linear.z = generator_.generateNormal(0.0, 0.2);
    twist.angular.z = generator_.generateNormal(0.0, 0.3);
  }
  
  void setPoseFromState(geometry_msgs::msg::PoseStamped& pose)
  {
    pose.pose.position.x = current_x_;
    pose.pose.position.y = current_y_;
    pose.pose.position.z = current_z_;
    
    // Convert yaw to quaternion
    double cy = std::cos(current_yaw_ * 0.5);
    double sy = std::sin(current_yaw_ * 0.5);
    
    pose.pose.orientation.w = cy;
    pose.pose.orientation.x = 0.0;
    pose.pose.orientation.y = 0.0;
    pose.pose.orientation.z = sy;
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr twist_publisher_;
  hw_pub_sub::utils::RandomGenerator generator_;
  size_t count_;
  std::string motion_type_;
  double motion_scale_;
  std::string frame_id_;
  
  // Current state
  double current_x_, current_y_, current_z_, current_yaw_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RandomGeometryPublisher>());
  rclcpp::shutdown();
  return 0;
}