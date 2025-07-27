#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "hw_pub_sub/msg/generator_config.hpp"
#include "hw_pub_sub/msg/system_health.hpp"
#include "hw_pub_sub/utils/random_generator.hpp"

using namespace std::chrono_literals;

class EnhancedPublisher : public rclcpp::Node
{
public:
  EnhancedPublisher()
  : Node("enhanced_publisher"), 
    count_(0),
    generator_(std::random_device{}())
  {
    // Declare parameters
    this->declare_parameter("publish_rate_hz", 1.0);
    this->declare_parameter("topic_name", "/enhanced_data");
    this->declare_parameter("generator_type", "mixed"); // mixed, config_based
    this->declare_parameter("enable_health_monitoring", true);
    this->declare_parameter("auto_adjust_rate", false);
    
    // Get parameters
    double rate = this->get_parameter("publish_rate_hz").as_double();
    topic_name_ = this->get_parameter("topic_name").as_string();
    generator_type_ = this->get_parameter("generator_type").as_string();
    enable_health_monitoring_ = this->get_parameter("enable_health_monitoring").as_bool();
    auto_adjust_rate_ = this->get_parameter("auto_adjust_rate").as_bool();
    
    // Create publishers with enhanced QoS
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    qos.durability(rclcpp::DurabilityPolicy::TransientLocal);
    qos.history(rclcpp::HistoryPolicy::KeepLast);
    
    config_publisher_ = this->create_publisher<hw_pub_sub::msg::GeneratorConfig>(
      topic_name_, qos);
    
    if (enable_health_monitoring_) {
      health_publisher_ = this->create_publisher<hw_pub_sub::msg::SystemHealth>(
        "/enhanced_publisher_health", qos);
        
      // Health monitoring timer
      health_timer_ = this->create_wall_timer(
        std::chrono::seconds(3),
        std::bind(&EnhancedPublisher::health_timer_callback, this)
      );
    }
    
    // Configuration subscriber for dynamic reconfiguration
    config_subscription_ = this->create_subscription<hw_pub_sub::msg::GeneratorConfig>(
      "/enhanced_config_updates", qos,
      std::bind(&EnhancedPublisher::config_callback, this, std::placeholders::_1));
    
    // Main publishing timer
    updateTimer(rate);
    
    start_time_ = this->get_clock()->now();
    
    RCLCPP_INFO(this->get_logger(), 
      "EnhancedPublisher started - Type: %s, Rate: %.1f Hz, Topic: %s", 
      generator_type_.c_str(), rate, topic_name_.c_str());
  }

private:
  void timer_callback()
  {
    auto message = hw_pub_sub::msg::GeneratorConfig();
    
    if (generator_type_ == "config_based") {
      generateConfigBasedMessage(message);
    } else {
      generateMixedMessage(message);
    }
    
    // Add metadata
    message.generator_type = generator_type_;
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing GeneratorConfig: type=%s, rate=%.1f, seed=%d", 
      message.generator_type.c_str(), message.publish_rate_hz, message.seed_value);
    
    config_publisher_->publish(message);
    count_++;
    last_publish_time_ = this->get_clock()->now();
    
    // Auto-adjust rate if enabled
    if (auto_adjust_rate_ && count_ % 20 == 0) {
      adjustPublishRate();
    }
  }
  
  void generateConfigBasedMessage(hw_pub_sub::msg::GeneratorConfig& config)
  {
    // Generate configuration based on current system state
    config.publish_rate_hz = this->get_parameter("publish_rate_hz").as_double();
    config.data_range_min = generator_.generateInteger(-100, 0);
    config.data_range_max = generator_.generateInteger(1, 100);
    config.noise_level = generator_.generateUniform(0.01, 0.5);
    config.enable_logging = generator_.generateInteger(0, 1) == 1;
    config.output_format = generator_.generateInteger(0, 1) ? "binary" : "text";
    config.seed_value = generator_.generateInteger(1000, 9999);
    config.mean_value = generator_.generateNormal(0.0, 10.0);
    config.std_deviation = generator_.generateUniform(0.5, 5.0);
  }
  
  void generateMixedMessage(hw_pub_sub::msg::GeneratorConfig& config)
  {
    // Generate mixed configuration with varying parameters
    static int cycle = 0;
    cycle++;
    
    switch (cycle % 4) {
      case 0: // Low noise, high rate
        config.publish_rate_hz = 5.0;
        config.noise_level = 0.05;
        config.output_format = "precise";
        break;
      case 1: // High noise, medium rate
        config.publish_rate_hz = 2.0;
        config.noise_level = 0.3;
        config.output_format = "noisy";
        break;
      case 2: // Variable rate
        config.publish_rate_hz = generator_.generateUniform(0.5, 3.0);
        config.noise_level = 0.1;
        config.output_format = "variable";
        break;
      case 3: // Custom configuration
        config.publish_rate_hz = 1.0;
        config.noise_level = generator_.generateExponential(0.2);
        config.output_format = "custom";
        break;
    }
    
    config.data_range_min = -50;
    config.data_range_max = 50;
    config.enable_logging = true;
    config.seed_value = count_;
    config.mean_value = generator_.generateSensorReading(0.0, config.noise_level);
    config.std_deviation = config.noise_level * 5.0;
  }
  
  void config_callback(const hw_pub_sub::msg::GeneratorConfig::SharedPtr msg)
  {
    RCLCPP_INFO(this->get_logger(), 
      "Received configuration update: rate=%.1f, type=%s", 
      msg->publish_rate_hz, msg->generator_type.c_str());
    
    // Apply configuration changes
    if (msg->publish_rate_hz > 0.1 && msg->publish_rate_hz <= 20.0) {
      updateTimer(msg->publish_rate_hz);
      RCLCPP_INFO(this->get_logger(), "Updated publish rate to %.1f Hz", msg->publish_rate_hz);
    }
    
    if (!msg->generator_type.empty()) {
      generator_type_ = msg->generator_type;
      RCLCPP_INFO(this->get_logger(), "Updated generator type to %s", generator_type_.c_str());
    }
    
    if (msg->seed_value > 0) {
      generator_.setSeed(msg->seed_value);
      RCLCPP_INFO(this->get_logger(), "Updated generator seed to %d", msg->seed_value);
    }
  }
  
  void health_timer_callback()
  {
    auto health_msg = hw_pub_sub::msg::SystemHealth();
    health_msg.header.stamp = this->get_clock()->now();
    health_msg.header.frame_id = "enhanced_publisher";
    health_msg.node_name = this->get_name();
    
    // Calculate uptime and message statistics
    auto current_time = this->get_clock()->now();
    health_msg.uptime_seconds = (current_time - start_time_).seconds();
    health_msg.message_count = count_;
    health_msg.message_rate_hz = count_ / health_msg.uptime_seconds;
    
    // Assess publisher health
    assessPublisherHealth(health_msg);
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing EnhancedPublisherHealth: rate=%.2f Hz, healthy=%s", 
      health_msg.message_rate_hz, health_msg.is_healthy ? "yes" : "no");
    
    health_publisher_->publish(health_msg);
  }
  
  void assessPublisherHealth(hw_pub_sub::msg::SystemHealth& health)
  {
    // Simulate system resource usage
    health.cpu_usage_percent = 8.0 + (count_ % 12) * 1.5;
    health.memory_usage_percent = 15.0 + (count_ % 8) * 1.0;
    health.network_latency_ms = 1.5 + (count_ % 5) * 0.5;
    
    // Initialize health status
    health.is_healthy = true;
    health.warnings.clear();
    health.errors.clear();
    
    // Check publishing rate consistency
    double expected_rate = this->get_parameter("publish_rate_hz").as_double();
    double actual_rate = health.message_rate_hz;
    double rate_deviation = std::abs(actual_rate - expected_rate) / expected_rate;
    
    if (rate_deviation > 0.2) { // 20% deviation
      health.warnings.push_back("Publishing rate deviation: " + 
        std::to_string(rate_deviation * 100) + "%");
      
      if (rate_deviation > 0.5) { // 50% deviation
        health.is_healthy = false;
        health.errors.push_back("Critical publishing rate deviation");
      }
    }
    
    // Check for publishing gaps
    if (last_publish_time_.nanoseconds() > 0) {
      auto current_time = this->get_clock()->now();
      double time_since_publish = (current_time - last_publish_time_).seconds();
      double expected_interval = 1.0 / expected_rate;
      
      if (time_since_publish > expected_interval * 3) {
        health.warnings.push_back("Long gap since last publish");
        if (time_since_publish > expected_interval * 10) {
          health.is_healthy = false;
          health.errors.push_back("Publishing appears to have stopped");
        }
      }
    }
    
    // Check system resources
    if (health.cpu_usage_percent > 75.0) {
      health.warnings.push_back("High CPU usage");
      if (health.cpu_usage_percent > 90.0) {
        health.is_healthy = false;
        health.errors.push_back("CPU usage critical");
      }
    }
    
    if (health.memory_usage_percent > 80.0) {
      health.warnings.push_back("High memory usage");
      if (health.memory_usage_percent > 95.0) {
        health.is_healthy = false;
        health.errors.push_back("Memory usage critical");
      }
    }
    
    // Check configuration consistency
    if (generator_type_ != "mixed" && generator_type_ != "config_based") {
      health.warnings.push_back("Unknown generator type: " + generator_type_);
    }
    
    // Performance metrics
    double message_efficiency = (health.message_count > 0) ? 
      health.uptime_seconds / health.message_count : 0.0;
      
    if (message_efficiency > 10.0) { // More than 10 seconds per message
      health.warnings.push_back("Low message efficiency");
    }
  }
  
  void adjustPublishRate()
  {
    if (!auto_adjust_rate_) return;
    
    // Simple adaptive rate adjustment based on system load
    double current_rate = this->get_parameter("publish_rate_hz").as_double();
    double cpu_usage = 8.0 + (count_ % 12) * 1.5; // Simulated
    
    double new_rate = current_rate;
    
    if (cpu_usage > 80.0) {
      // Reduce rate if high CPU
      new_rate = current_rate * 0.8;
    } else if (cpu_usage < 30.0) {
      // Increase rate if low CPU
      new_rate = current_rate * 1.1;
    }
    
    // Clamp rate to reasonable bounds
    new_rate = std::max(0.1, std::min(10.0, new_rate));
    
    if (std::abs(new_rate - current_rate) > 0.1) {
      updateTimer(new_rate);
      RCLCPP_INFO(this->get_logger(), 
        "Auto-adjusted publish rate from %.1f to %.1f Hz (CPU: %.1f%%)",
        current_rate, new_rate, cpu_usage);
    }
  }
  
  void updateTimer(double rate)
  {
    // Update the parameter
    this->set_parameter(rclcpp::Parameter("publish_rate_hz", rate));
    
    // Recreate timer with new rate
    if (timer_) {
      timer_->cancel();
    }
    
    auto timer_period = std::chrono::duration<double>(1.0 / rate);
    timer_ = this->create_wall_timer(
      timer_period, 
      std::bind(&EnhancedPublisher::timer_callback, this)
    );
  }

  // Publishers and subscribers
  rclcpp::Publisher<hw_pub_sub::msg::GeneratorConfig>::SharedPtr config_publisher_;
  rclcpp::Publisher<hw_pub_sub::msg::SystemHealth>::SharedPtr health_publisher_;
  rclcpp::Subscription<hw_pub_sub::msg::GeneratorConfig>::SharedPtr config_subscription_;
  
  // Timers
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr health_timer_;
  
  // State
  hw_pub_sub::utils::RandomGenerator generator_;
  size_t count_;
  rclcpp::Time start_time_;
  rclcpp::Time last_publish_time_;
  
  // Configuration
  std::string topic_name_;
  std::string generator_type_;
  bool enable_health_monitoring_;
  bool auto_adjust_rate_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<EnhancedPublisher>());
  rclcpp::shutdown();
  return 0;
}