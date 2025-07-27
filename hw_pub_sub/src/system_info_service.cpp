#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "hw_pub_sub/srv/get_system_info.hpp"
#include "hw_pub_sub/msg/system_health.hpp"

class SystemInfoService : public rclcpp::Node
{
public:
  SystemInfoService()
  : Node("system_info_service")
  {
    // Create service
    service_ = this->create_service<hw_pub_sub::srv::GetSystemInfo>(
      "get_system_info",
      std::bind(&SystemInfoService::handle_service_request, this,
                std::placeholders::_1, std::placeholders::_2));
    
    // Create health publisher
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    
    health_publisher_ = this->create_publisher<hw_pub_sub::msg::SystemHealth>(
      "/system_info_service_health", qos);
    
    // Health monitoring timer
    health_timer_ = this->create_wall_timer(
      std::chrono::seconds(5),
      std::bind(&SystemInfoService::health_timer_callback, this)
    );
    
    start_time_ = this->get_clock()->now();
    request_count_ = 0;
    
    RCLCPP_INFO(this->get_logger(), "SystemInfoService started and ready to serve requests");
  }

private:
  void handle_service_request(
    const std::shared_ptr<hw_pub_sub::srv::GetSystemInfo::Request> request,
    std::shared_ptr<hw_pub_sub::srv::GetSystemInfo::Response> response)
  {
    RCLCPP_INFO(this->get_logger(), 
      "Received system info request: %s", request->request_type.c_str());
    
    // Set response header
    response->header.stamp = this->get_clock()->now();
    response->header.frame_id = "system_info";
    
    // Basic system information
    response->system_name = "ROS2 Multi-Topic Communication System";
    response->ros_version = "ROS2 Jazzy";
    
    // Calculate system uptime
    auto current_time = this->get_clock()->now();
    response->system_uptime = (current_time - start_time_).seconds();
    
    // Process different request types
    if (request->request_type == "nodes") {
      response->active_nodes = getActiveNodes();
      response->success = true;
      response->message = "Active nodes retrieved successfully";
    }
    else if (request->request_type == "topics") {
      response->active_topics = getActiveTopics();
      response->success = true;
      response->message = "Active topics retrieved successfully";
    }
    else if (request->request_type == "full") {
      response->active_nodes = getActiveNodes();
      response->active_topics = getActiveTopics();
      response->success = true;
      response->message = "Full system information retrieved successfully";
    }
    else if (request->request_type == "health") {
      response->active_nodes = getHealthStatus();
      response->success = true;
      response->message = "System health status retrieved successfully";
    }
    else {
      response->success = false;
      response->message = "Unknown request type: " + request->request_type + 
                         ". Supported types: nodes, topics, full, health";
    }
    
    request_count_++;
    
    RCLCPP_INFO(this->get_logger(), 
      "Service request processed: success=%s, message='%s'",
      response->success ? "true" : "false", response->message.c_str());
  }
  
  std::vector<std::string> getActiveNodes()
  {
    // In a real implementation, this would query the ROS2 graph
    // For this demo, we return the expected nodes
    std::vector<std::string> nodes = {
      "/random_string_publisher",
      "/random_number_publisher", 
      "/random_sensor_publisher",
      "/random_geometry_publisher",
      "/enhanced_publisher",
      "/string_subscriber",
      "/number_subscriber",
      "/sensor_subscriber",
      "/geometry_subscriber",
      "/system_info_service"
    };
    
    RCLCPP_DEBUG(this->get_logger(), "Retrieved %zu active nodes", nodes.size());
    return nodes;
  }
  
  std::vector<std::string> getActiveTopics()
  {
    // In a real implementation, this would query the ROS2 graph
    // For this demo, we return the expected topics
    std::vector<std::string> topics = {
      "/random_strings",
      "/random_numbers",
      "/random_sensors", 
      "/random_poses",
      "/random_velocities",
      "/enhanced_data",
      "/string_analysis",
      "/number_analysis_health",
      "/sensor_health",
      "/geometry_tracking_health",
      "/enhanced_publisher_health",
      "/system_info_service_health"
    };
    
    RCLCPP_DEBUG(this->get_logger(), "Retrieved %zu active topics", topics.size());
    return topics;
  }
  
  std::vector<std::string> getHealthStatus()
  {
    // Return health status information
    std::vector<std::string> health_info = {
      "System Status: OPERATIONAL",
      "Uptime: " + std::to_string((this->get_clock()->now() - start_time_).seconds()) + " seconds",
      "Service Requests: " + std::to_string(request_count_),
      "Publisher Nodes: 5 active",
      "Subscriber Nodes: 4 active", 
      "Health Monitoring: ENABLED",
      "Data Generation: ACTIVE",
      "Message Flow: NOMINAL"
    };
    
    RCLCPP_DEBUG(this->get_logger(), "Generated health status with %zu items", health_info.size());
    return health_info;
  }
  
  void health_timer_callback()
  {
    auto health_msg = hw_pub_sub::msg::SystemHealth();
    health_msg.header.stamp = this->get_clock()->now();
    health_msg.header.frame_id = "system_info_service";
    health_msg.node_name = this->get_name();
    
    // Calculate uptime and service statistics
    auto current_time = this->get_clock()->now();
    health_msg.uptime_seconds = (current_time - start_time_).seconds();
    health_msg.message_count = request_count_;
    health_msg.message_rate_hz = request_count_ / health_msg.uptime_seconds;
    
    // Assess service health
    assessServiceHealth(health_msg);
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing SystemInfoServiceHealth: requests=%d, rate=%.3f req/s, healthy=%s", 
      request_count_, health_msg.message_rate_hz, health_msg.is_healthy ? "yes" : "no");
    
    health_publisher_->publish(health_msg);
  }
  
  void assessServiceHealth(hw_pub_sub::msg::SystemHealth& health)
  {
    // Simulate system resource usage
    health.cpu_usage_percent = 5.0 + (request_count_ % 10) * 0.5;
    health.memory_usage_percent = 10.0 + (request_count_ % 6) * 0.3;
    health.network_latency_ms = 1.0 + (request_count_ % 4) * 0.2;
    
    // Initialize health status
    health.is_healthy = true;
    health.warnings.clear();
    health.errors.clear();
    
    // Check service availability
    if (health.uptime_seconds < 1.0) {
      health.warnings.push_back("Service recently started");
    }
    
    // Check request rate patterns
    if (health.message_rate_hz > 10.0) {
      health.warnings.push_back("High service request rate");
      if (health.message_rate_hz > 50.0) {
        health.is_healthy = false;
        health.errors.push_back("Service request rate critical");
      }
    }
    
    // Check system resources
    if (health.cpu_usage_percent > 80.0) {
      health.warnings.push_back("High CPU usage");
      if (health.cpu_usage_percent > 95.0) {
        health.is_healthy = false;
        health.errors.push_back("CPU usage critical");
      }
    }
    
    if (health.memory_usage_percent > 85.0) {
      health.warnings.push_back("High memory usage");
      if (health.memory_usage_percent > 95.0) {
        health.is_healthy = false;
        health.errors.push_back("Memory usage critical");
      }
    }
    
    // Service-specific checks
    if (request_count_ == 0 && health.uptime_seconds > 30.0) {
      health.warnings.push_back("No service requests received");
    }
    
    // Simulate occasional warnings for demonstration
    if (request_count_ % 100 == 50) {
      health.warnings.push_back("Periodic maintenance check");
    }
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Service health assessment: %zu warnings, %zu errors",
      health.warnings.size(), health.errors.size());
  }

  rclcpp::Service<hw_pub_sub::srv::GetSystemInfo>::SharedPtr service_;
  rclcpp::Publisher<hw_pub_sub::msg::SystemHealth>::SharedPtr health_publisher_;
  rclcpp::TimerBase::SharedPtr health_timer_;
  
  rclcpp::Time start_time_;
  int request_count_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SystemInfoService>());
  rclcpp::shutdown();
  return 0;
}