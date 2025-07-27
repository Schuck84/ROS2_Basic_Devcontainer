#include <memory>
#include <vector>
#include <deque>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "hw_pub_sub/msg/system_health.hpp"

class GeometrySubscriber : public rclcpp::Node
{
public:
  GeometrySubscriber()
  : Node("geometry_subscriber"), message_count_(0), total_distance_(0.0)
  {
    // Declare parameters
    this->declare_parameter("enable_trajectory_tracking", true);
    this->declare_parameter("max_trajectory_points", 100);
    this->declare_parameter("velocity_threshold", 0.1);
    
    // Get parameters
    enable_trajectory_tracking_ = this->get_parameter("enable_trajectory_tracking").as_bool();
    max_trajectory_points_ = this->get_parameter("max_trajectory_points").as_int();
    velocity_threshold_ = this->get_parameter("velocity_threshold").as_double();
    
    // Create subscribers with QoS
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    
    pose_subscription_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
      "/random_poses", qos,
      std::bind(&GeometrySubscriber::pose_callback, this, std::placeholders::_1));
    
    twist_subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/random_velocities", qos,
      std::bind(&GeometrySubscriber::twist_callback, this, std::placeholders::_1));
    
    // Create health publisher if tracking enabled
    if (enable_trajectory_tracking_) {
      health_publisher_ = this->create_publisher<hw_pub_sub::msg::SystemHealth>(
        "/geometry_tracking_health", qos);
      
      // Create timer for periodic health reporting
      timer_ = this->create_wall_timer(
        std::chrono::seconds(2), 
        std::bind(&GeometrySubscriber::health_timer_callback, this)
      );
    }
    
    start_time_ = this->get_clock()->now();
    last_position_ = {0.0, 0.0, 0.0};
    
    RCLCPP_INFO(this->get_logger(), "GeometrySubscriber started");
  }

private:
  void pose_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
  {
    RCLCPP_DEBUG(this->get_logger(), 
      "Received Pose: [%.2f, %.2f, %.2f] in frame %s", 
      msg->pose.position.x, msg->pose.position.y, msg->pose.position.z,
      msg->header.frame_id.c_str());
    
    // Store trajectory point
    if (enable_trajectory_tracking_) {
      trajectory_points_.push_back(*msg);
      if (trajectory_points_.size() > max_trajectory_points_) {
        trajectory_points_.pop_front();
      }
      
      // Calculate movement distance
      calculateMovement(*msg);
      
      // Analyze trajectory
      analyzeTrajectory();
    }
    
    message_count_++;
    last_pose_time_ = this->get_clock()->now();
  }
  
  void twist_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    RCLCPP_DEBUG(this->get_logger(), 
      "Received Twist: linear=[%.2f, %.2f, %.2f], angular=[%.2f, %.2f, %.2f]", 
      msg->linear.x, msg->linear.y, msg->linear.z,
      msg->angular.x, msg->angular.y, msg->angular.z);
    
    // Store velocity data
    recent_velocities_.push_back(*msg);
    if (recent_velocities_.size() > 20) { // Keep last 20 velocity readings
      recent_velocities_.pop_front();
    }
    
    // Analyze velocity
    analyzeVelocity(*msg);
    
    last_twist_time_ = this->get_clock()->now();
  }
  
  void calculateMovement(const geometry_msgs::msg::PoseStamped& current_pose)
  {
    if (trajectory_points_.size() < 2) {
      last_position_ = {current_pose.pose.position.x, 
                       current_pose.pose.position.y, 
                       current_pose.pose.position.z};
      return;
    }
    
    // Calculate distance from last position
    double dx = current_pose.pose.position.x - last_position_[0];
    double dy = current_pose.pose.position.y - last_position_[1];
    double dz = current_pose.pose.position.z - last_position_[2];
    
    double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    total_distance_ += distance;
    
    // Update last position
    last_position_ = {current_pose.pose.position.x, 
                     current_pose.pose.position.y, 
                     current_pose.pose.position.z};
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Movement: distance=%.3fm, total=%.3fm", distance, total_distance_);
  }
  
  void analyzeTrajectory()
  {
    if (trajectory_points_.size() < 5) return;
    
    // Calculate trajectory metrics
    double max_x = -1000.0, min_x = 1000.0;
    double max_y = -1000.0, min_y = 1000.0;
    double max_z = -1000.0, min_z = 1000.0;
    
    for (const auto& point : trajectory_points_) {
      max_x = std::max(max_x, point.pose.position.x);
      min_x = std::min(min_x, point.pose.position.x);
      max_y = std::max(max_y, point.pose.position.y);
      min_y = std::min(min_y, point.pose.position.y);
      max_z = std::max(max_z, point.pose.position.z);
      min_z = std::min(min_z, point.pose.position.z);
    }
    
    double x_range = max_x - min_x;
    double y_range = max_y - min_y;
    double z_range = max_z - min_z;
    
    // Detect trajectory patterns
    if (x_range < 0.1 && y_range < 0.1) {
      RCLCPP_DEBUG(this->get_logger(), "Stationary trajectory detected");
    } else if (x_range > y_range * 5) {
      RCLCPP_DEBUG(this->get_logger(), "Linear trajectory (X-axis) detected");
    } else if (y_range > x_range * 5) {
      RCLCPP_DEBUG(this->get_logger(), "Linear trajectory (Y-axis) detected");
    } else if (std::abs(x_range - y_range) < 0.5 && x_range > 1.0) {
      RCLCPP_DEBUG(this->get_logger(), "Circular/elliptical trajectory detected");
    }
    
    // Log trajectory statistics every 20 messages
    if (message_count_ % 20 == 0) {
      RCLCPP_INFO(this->get_logger(), 
        "Trajectory stats: X[%.2f,%.2f], Y[%.2f,%.2f], Z[%.2f,%.2f], Total distance: %.2fm",
        min_x, max_x, min_y, max_y, min_z, max_z, total_distance_);
    }
  }
  
  void analyzeVelocity(const geometry_msgs::msg::Twist& twist)
  {
    // Calculate velocity magnitude
    double linear_speed = std::sqrt(twist.linear.x * twist.linear.x +
                                   twist.linear.y * twist.linear.y +
                                   twist.linear.z * twist.linear.z);
    
    double angular_speed = std::sqrt(twist.angular.x * twist.angular.x +
                                    twist.angular.y * twist.angular.y +
                                    twist.angular.z * twist.angular.z);
    
    // Detect motion states
    if (linear_speed < velocity_threshold_ && angular_speed < velocity_threshold_) {
      RCLCPP_DEBUG(this->get_logger(), "Robot appears stationary");
    } else if (linear_speed > angular_speed * 2) {
      RCLCPP_DEBUG(this->get_logger(), "Linear motion dominant: %.3f m/s", linear_speed);
    } else if (angular_speed > linear_speed * 2) {
      RCLCPP_DEBUG(this->get_logger(), "Rotational motion dominant: %.3f rad/s", angular_speed);
    } else {
      RCLCPP_DEBUG(this->get_logger(), "Combined motion: %.3f m/s, %.3f rad/s", 
        linear_speed, angular_speed);
    }
    
    // Store velocity statistics
    recent_linear_speeds_.push_back(linear_speed);
    recent_angular_speeds_.push_back(angular_speed);
    
    if (recent_linear_speeds_.size() > 20) {
      recent_linear_speeds_.pop_front();
      recent_angular_speeds_.pop_front();
    }
  }
  
  void health_timer_callback()
  {
    auto health_msg = hw_pub_sub::msg::SystemHealth();
    health_msg.header.stamp = this->get_clock()->now();
    health_msg.header.frame_id = "geometry_tracking";
    health_msg.node_name = this->get_name();
    
    // Calculate uptime and message statistics
    auto current_time = this->get_clock()->now();
    health_msg.uptime_seconds = (current_time - start_time_).seconds();
    health_msg.message_count = message_count_;
    health_msg.message_rate_hz = message_count_ / health_msg.uptime_seconds;
    
    // Assess tracking health
    assessTrackingHealth(health_msg);
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing GeometryHealth: rate=%.2f Hz, healthy=%s, trajectory_points=%zu", 
      health_msg.message_rate_hz, health_msg.is_healthy ? "yes" : "no",
      trajectory_points_.size());
    
    health_publisher_->publish(health_msg);
  }
  
  void assessTrackingHealth(hw_pub_sub::msg::SystemHealth& health)
  {
    // Simulate system resource usage
    health.cpu_usage_percent = 12.0 + (message_count_ % 6) * 2.0;
    health.memory_usage_percent = 20.0 + (message_count_ % 4) * 1.5;
    health.network_latency_ms = 2.0 + (message_count_ % 3) * 1.0;
    
    // Initialize health status
    health.is_healthy = true;
    health.warnings.clear();
    health.errors.clear();
    
    // Check message rate
    if (health.message_rate_hz < 0.5) {
      health.warnings.push_back("Low geometry message rate");
      if (health.message_rate_hz < 0.1) {
        health.is_healthy = false;
        health.errors.push_back("Geometry message rate critically low");
      }
    }
    
    // Check data staleness
    auto current_time = this->get_clock()->now();
    if (last_pose_time_.nanoseconds() > 0) {
      double time_since_pose = (current_time - last_pose_time_).seconds();
      if (time_since_pose > 10.0) {
        health.warnings.push_back("Stale pose data");
        if (time_since_pose > 30.0) {
          health.is_healthy = false;
          health.errors.push_back("Pose data timeout");
        }
      }
    }
    
    if (last_twist_time_.nanoseconds() > 0) {
      double time_since_twist = (current_time - last_twist_time_).seconds();
      if (time_since_twist > 10.0) {
        health.warnings.push_back("Stale velocity data");
      }
    }
    
    // Check trajectory quality
    if (trajectory_points_.size() < max_trajectory_points_ / 4) {
      health.warnings.push_back("Insufficient trajectory history");
    }
    
    // Analyze velocity patterns for anomalies
    if (!recent_linear_speeds_.empty()) {
      // Calculate average speeds
      double avg_linear = 0.0, avg_angular = 0.0;
      for (size_t i = 0; i < recent_linear_speeds_.size(); ++i) {
        avg_linear += recent_linear_speeds_[i];
        avg_angular += recent_angular_speeds_[i];
      }
      avg_linear /= recent_linear_speeds_.size();
      avg_angular /= recent_angular_speeds_.size();
      
      // Check for excessive speeds
      if (avg_linear > 10.0) {
        health.warnings.push_back("High average linear velocity");
      }
      
      if (avg_angular > 5.0) {
        health.warnings.push_back("High average angular velocity");
      }
      
      // Check for motion consistency
      double linear_variance = 0.0;
      for (double speed : recent_linear_speeds_) {
        linear_variance += (speed - avg_linear) * (speed - avg_linear);
      }
      linear_variance /= recent_linear_speeds_.size();
      
      if (linear_variance > 25.0) {
        health.warnings.push_back("Highly variable linear motion");
      }
    }
    
    // System resource checks
    if (health.cpu_usage_percent > 70.0) {
      health.warnings.push_back("High CPU usage");
    }
    
    if (health.memory_usage_percent > 80.0) {
      health.warnings.push_back("High memory usage");
    }
    
    // Overall assessment
    RCLCPP_DEBUG(this->get_logger(), 
      "Health assessment: %zu warnings, %zu errors, total_distance=%.2fm",
      health.warnings.size(), health.errors.size(), total_distance_);
  }

  // Subscriptions
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr pose_subscription_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr twist_subscription_;
  rclcpp::Publisher<hw_pub_sub::msg::SystemHealth>::SharedPtr health_publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  
  // Data storage
  std::deque<geometry_msgs::msg::PoseStamped> trajectory_points_;
  std::deque<geometry_msgs::msg::Twist> recent_velocities_;
  std::deque<double> recent_linear_speeds_;
  std::deque<double> recent_angular_speeds_;
  
  // State tracking
  size_t message_count_;
  double total_distance_;
  std::vector<double> last_position_;
  rclcpp::Time start_time_;
  rclcpp::Time last_pose_time_;
  rclcpp::Time last_twist_time_;
  
  // Parameters
  bool enable_trajectory_tracking_;
  int max_trajectory_points_;
  double velocity_threshold_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GeometrySubscriber>());
  rclcpp::shutdown();
  return 0;
}