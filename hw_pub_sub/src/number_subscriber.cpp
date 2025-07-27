#include <memory>
#include <vector>
#include <deque>
#include <algorithm>
#include <numeric>

#include "rclcpp/rclcpp.hpp"
#include "hw_pub_sub/msg/number_statistics.hpp"
#include "hw_pub_sub/msg/system_health.hpp"

class NumberSubscriber : public rclcpp::Node
{
public:
  NumberSubscriber()
  : Node("number_subscriber"), message_count_(0)
  {
    // Declare parameters
    this->declare_parameter("enable_statistics_publishing", true);
    this->declare_parameter("statistics_rate_hz", 0.5);
    this->declare_parameter("window_size", 20);
    
    // Get parameters
    enable_statistics_publishing_ = this->get_parameter("enable_statistics_publishing").as_bool();
    double stats_rate = this->get_parameter("statistics_rate_hz").as_double();
    window_size_ = this->get_parameter("window_size").as_int();
    
    // Create subscriber with QoS
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    
    subscription_ = this->create_subscription<hw_pub_sub::msg::NumberStatistics>(
      "/random_numbers", qos,
      std::bind(&NumberSubscriber::topic_callback, this, std::placeholders::_1));
    
    // Create health publisher if enabled
    if (enable_statistics_publishing_) {
      health_publisher_ = this->create_publisher<hw_pub_sub::msg::SystemHealth>(
        "/number_analysis_health", qos);
      
      // Create timer for periodic health reporting
      auto timer_period = std::chrono::duration<double>(1.0 / stats_rate);
      timer_ = this->create_wall_timer(
        timer_period, 
        std::bind(&NumberSubscriber::health_timer_callback, this)
      );
    }
    
    start_time_ = this->get_clock()->now();
    
    RCLCPP_INFO(this->get_logger(), "NumberSubscriber started");
  }

private:
  void topic_callback(const hw_pub_sub::msg::NumberStatistics::SharedPtr msg)
  {
    RCLCPP_INFO(this->get_logger(), 
      "Received NumberStatistics: mean=%.3f, std=%.3f, trend=%.3f", 
      msg->mean, msg->standard_deviation, msg->trend_slope);
    
    // Store statistics for analysis
    recent_stats_.push_back(*msg);
    if (recent_stats_.size() > window_size_) {
      recent_stats_.pop_front();
    }
    
    // Perform running analysis
    analyzeStatistics(*msg);
    message_count_++;
    
    // Update running averages
    updateRunningAverages(*msg);
  }
  
  void health_timer_callback()
  {
    if (recent_stats_.empty()) return;
    
    auto health_msg = hw_pub_sub::msg::SystemHealth();
    health_msg.header.stamp = this->get_clock()->now();
    health_msg.header.frame_id = "number_analysis";
    health_msg.node_name = this->get_name();
    
    // Calculate uptime
    auto current_time = this->get_clock()->now();
    health_msg.uptime_seconds = (current_time - start_time_).seconds();
    
    // Calculate message rate
    health_msg.message_count = message_count_;
    health_msg.message_rate_hz = message_count_ / health_msg.uptime_seconds;
    
    // Analyze system health
    assessSystemHealth(health_msg);
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing SystemHealth: rate=%.2f Hz, healthy=%s", 
      health_msg.message_rate_hz, health_msg.is_healthy ? "yes" : "no");
    
    health_publisher_->publish(health_msg);
  }
  
  void analyzeStatistics(const hw_pub_sub::msg::NumberStatistics& stats)
  {
    // Detect anomalies in the statistics
    bool anomaly_detected = false;
    std::string anomaly_description;
    
    // Check for extreme values
    if (std::abs(stats.mean) > 100.0) {
      anomaly_detected = true;
      anomaly_description += "Extreme mean value; ";
    }
    
    if (stats.standard_deviation > 50.0) {
      anomaly_detected = true;
      anomaly_description += "High variance; ";
    }
    
    // Check trend
    if (std::abs(stats.trend_slope) > 10.0) {
      anomaly_detected = true;
      anomaly_description += "Strong trend detected; ";
    }
    
    // Check data quality
    if (stats.data_sequence.empty()) {
      anomaly_detected = true;
      anomaly_description += "No data received; ";
    }
    
    if (anomaly_detected) {
      RCLCPP_WARN(this->get_logger(), 
        "Anomaly detected in NumberStatistics: %s", 
        anomaly_description.c_str());
    }
    
    // Log interesting patterns
    if (stats.distribution_type == "normal" && stats.standard_deviation < 0.1) {
      RCLCPP_DEBUG(this->get_logger(), "Very low variance normal distribution detected");
    }
    
    if (stats.trend_slope > 1.0) {
      RCLCPP_DEBUG(this->get_logger(), "Strong positive trend detected: %.3f", stats.trend_slope);
    } else if (stats.trend_slope < -1.0) {
      RCLCPP_DEBUG(this->get_logger(), "Strong negative trend detected: %.3f", stats.trend_slope);
    }
  }
  
  void updateRunningAverages(const hw_pub_sub::msg::NumberStatistics& stats)
  {
    // Update running averages for long-term analysis
    if (recent_stats_.size() >= 2) {
      // Calculate average trend over window
      double trend_sum = 0.0;
      for (const auto& stat : recent_stats_) {
        trend_sum += stat.trend_slope;
      }
      double avg_trend = trend_sum / recent_stats_.size();
      
      // Calculate variance of means (stability measure)
      double mean_sum = 0.0;
      for (const auto& stat : recent_stats_) {
        mean_sum += stat.mean;
      }
      double avg_mean = mean_sum / recent_stats_.size();
      
      double variance_of_means = 0.0;
      for (const auto& stat : recent_stats_) {
        variance_of_means += (stat.mean - avg_mean) * (stat.mean - avg_mean);
      }
      variance_of_means /= recent_stats_.size();
      
      // Log stability metrics
      if (message_count_ % 10 == 0) { // Log every 10th message
        RCLCPP_DEBUG(this->get_logger(), 
          "Stability metrics - Avg trend: %.3f, Mean variance: %.3f", 
          avg_trend, variance_of_means);
      }
    }
  }
  
  void assessSystemHealth(hw_pub_sub::msg::SystemHealth& health)
  {
    // Simulate CPU and memory usage
    health.cpu_usage_percent = 15.0 + (message_count_ % 10) * 2.0; // Simulate varying load
    health.memory_usage_percent = 25.0 + (message_count_ % 5) * 1.0;
    
    // Calculate network latency (simulated)
    health.network_latency_ms = 5.0 + (message_count_ % 3) * 2.0;
    
    // Health assessment
    health.is_healthy = true;
    health.warnings.clear();
    health.errors.clear();
    
    // Check various health criteria
    if (health.message_rate_hz < 0.1) {
      health.warnings.push_back("Low message rate");
      if (health.message_rate_hz < 0.01) {
        health.is_healthy = false;
        health.errors.push_back("Message rate too low");
      }
    }
    
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
    
    if (health.network_latency_ms > 100.0) {
      health.warnings.push_back("High network latency");
      if (health.network_latency_ms > 500.0) {
        health.is_healthy = false;
        health.errors.push_back("Network latency critical");
      }
    }
    
    // Check for data quality issues
    if (recent_stats_.size() < window_size_ / 2) {
      health.warnings.push_back("Insufficient data history");
    }
    
    // Check for recent anomalies
    if (!recent_stats_.empty()) {
      const auto& latest = recent_stats_.back();
      if (std::abs(latest.mean) > 50.0) {
        health.warnings.push_back("Extreme mean values detected");
      }
    }
  }

  rclcpp::Subscription<hw_pub_sub::msg::NumberStatistics>::SharedPtr subscription_;
  rclcpp::Publisher<hw_pub_sub::msg::SystemHealth>::SharedPtr health_publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  
  std::deque<hw_pub_sub::msg::NumberStatistics> recent_stats_;
  size_t message_count_;
  int window_size_;
  bool enable_statistics_publishing_;
  rclcpp::Time start_time_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NumberSubscriber>());
  rclcpp::shutdown();
  return 0;
}