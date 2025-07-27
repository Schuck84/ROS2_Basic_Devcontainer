#include <memory>
#include <vector>
#include <deque>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "hw_pub_sub/msg/sensor_data.hpp"
#include "hw_pub_sub/msg/system_health.hpp"

class SensorSubscriber : public rclcpp::Node
{
public:
  SensorSubscriber()
  : Node("sensor_subscriber"), message_count_(0)
  {
    // Declare parameters
    this->declare_parameter("enable_health_monitoring", true);
    this->declare_parameter("health_check_rate_hz", 2.0);
    this->declare_parameter("temperature_limits", std::vector<double>{-10.0, 60.0});
    this->declare_parameter("humidity_limits", std::vector<double>{0.0, 100.0});
    this->declare_parameter("pressure_limits", std::vector<double>{800.0, 1200.0});
    
    // Get parameters
    enable_health_monitoring_ = this->get_parameter("enable_health_monitoring").as_bool();
    double health_rate = this->get_parameter("health_check_rate_hz").as_double();
    temp_limits_ = this->get_parameter("temperature_limits").as_double_array();
    humidity_limits_ = this->get_parameter("humidity_limits").as_double_array();
    pressure_limits_ = this->get_parameter("pressure_limits").as_double_array();
    
    // Create subscriber with QoS for sensor data
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::BestEffort); // Sensor data typically best effort
    qos.history(rclcpp::HistoryPolicy::KeepLast);
    
    subscription_ = this->create_subscription<hw_pub_sub::msg::SensorData>(
      "/random_sensors", qos,
      std::bind(&SensorSubscriber::topic_callback, this, std::placeholders::_1));
    
    // Create health publisher if enabled
    if (enable_health_monitoring_) {
      rclcpp::QoS health_qos(10);
      health_qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
      
      health_publisher_ = this->create_publisher<hw_pub_sub::msg::SystemHealth>(
        "/sensor_health", health_qos);
      
      // Create timer for periodic health monitoring
      auto timer_period = std::chrono::duration<double>(1.0 / health_rate);
      timer_ = this->create_wall_timer(
        timer_period, 
        std::bind(&SensorSubscriber::health_timer_callback, this)
      );
    }
    
    start_time_ = this->get_clock()->now();
    
    RCLCPP_INFO(this->get_logger(), "SensorSubscriber started");
  }

private:
  void topic_callback(const hw_pub_sub::msg::SensorData::SharedPtr msg)
  {
    RCLCPP_DEBUG(this->get_logger(), 
      "Received SensorData from %s: T=%.1f°C, H=%.1f%%, P=%.1fhPa, Battery=%.1f%%", 
      msg->sensor_name.c_str(), msg->temperature, msg->humidity, 
      msg->pressure, msg->battery_level);
    
    // Store sensor data for analysis
    recent_sensor_data_.push_back(*msg);
    if (recent_sensor_data_.size() > 50) { // Keep last 50 readings
      recent_sensor_data_.pop_front();
    }
    
    // Perform immediate validation
    validateSensorData(*msg);
    
    // Detect anomalies
    detectAnomalies(*msg);
    
    message_count_++;
    last_message_time_ = this->get_clock()->now();
  }
  
  void health_timer_callback()
  {
    if (recent_sensor_data_.empty()) return;
    
    auto health_msg = hw_pub_sub::msg::SystemHealth();
    health_msg.header.stamp = this->get_clock()->now();
    health_msg.header.frame_id = "sensor_health";
    health_msg.node_name = this->get_name();
    
    // Calculate uptime and message statistics
    auto current_time = this->get_clock()->now();
    health_msg.uptime_seconds = (current_time - start_time_).seconds();
    health_msg.message_count = message_count_;
    health_msg.message_rate_hz = message_count_ / health_msg.uptime_seconds;
    
    // Assess overall sensor health
    assessSensorHealth(health_msg);
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing SensorHealth: rate=%.2f Hz, healthy=%s, warnings=%zu", 
      health_msg.message_rate_hz, health_msg.is_healthy ? "yes" : "no",
      health_msg.warnings.size());
    
    health_publisher_->publish(health_msg);
  }
  
  void validateSensorData(const hw_pub_sub::msg::SensorData& data)
  {
    std::vector<std::string> validation_errors;
    
    // Validate temperature
    if (data.temperature < temp_limits_[0] || data.temperature > temp_limits_[1]) {
      validation_errors.push_back("Temperature out of range: " + std::to_string(data.temperature));
    }
    
    // Validate humidity
    if (data.humidity < humidity_limits_[0] || data.humidity > humidity_limits_[1]) {
      validation_errors.push_back("Humidity out of range: " + std::to_string(data.humidity));
    }
    
    // Validate pressure
    if (data.pressure < pressure_limits_[0] || data.pressure > pressure_limits_[1]) {
      validation_errors.push_back("Pressure out of range: " + std::to_string(data.pressure));
    }
    
    // Validate battery level
    if (data.battery_level < 0.0 || data.battery_level > 100.0) {
      validation_errors.push_back("Battery level out of range: " + std::to_string(data.battery_level));
    }
    
    // Validate IMU data (reasonable acceleration ranges)
    double total_accel = std::sqrt(data.x_acceleration * data.x_acceleration +
                                  data.y_acceleration * data.y_acceleration +
                                  data.z_acceleration * data.z_acceleration);
    if (total_accel < 5.0 || total_accel > 20.0) { // Reasonable range including gravity
      validation_errors.push_back("Unusual acceleration magnitude: " + std::to_string(total_accel));
    }
    
    // Log validation errors
    if (!validation_errors.empty()) {
      for (const auto& error : validation_errors) {
        RCLCPP_WARN(this->get_logger(), "Validation error: %s", error.c_str());
      }
    }
  }
  
  void detectAnomalies(const hw_pub_sub::msg::SensorData& data)
  {
    if (recent_sensor_data_.size() < 5) return; // Need some history
    
    // Calculate recent averages for comparison
    double avg_temp = 0.0, avg_humidity = 0.0, avg_pressure = 0.0;
    for (const auto& sensor_data : recent_sensor_data_) {
      avg_temp += sensor_data.temperature;
      avg_humidity += sensor_data.humidity;
      avg_pressure += sensor_data.pressure;
    }
    
    size_t count = recent_sensor_data_.size();
    avg_temp /= count;
    avg_humidity /= count;
    avg_pressure /= count;
    
    // Detect sudden changes (anomalies)
    const double temp_threshold = 5.0; // 5°C sudden change
    const double humidity_threshold = 10.0; // 10% sudden change
    const double pressure_threshold = 50.0; // 50 hPa sudden change
    
    if (std::abs(data.temperature - avg_temp) > temp_threshold) {
      RCLCPP_WARN(this->get_logger(), 
        "Temperature anomaly detected: current=%.1f°C, average=%.1f°C", 
        data.temperature, avg_temp);
    }
    
    if (std::abs(data.humidity - avg_humidity) > humidity_threshold) {
      RCLCPP_WARN(this->get_logger(), 
        "Humidity anomaly detected: current=%.1f%%, average=%.1f%%", 
        data.humidity, avg_humidity);
    }
    
    if (std::abs(data.pressure - avg_pressure) > pressure_threshold) {
      RCLCPP_WARN(this->get_logger(), 
        "Pressure anomaly detected: current=%.1fhPa, average=%.1fhPa", 
        data.pressure, avg_pressure);
    }
    
    // Check for sensor status issues
    if (!data.is_calibrated) {
      RCLCPP_WARN(this->get_logger(), "Sensor %s needs calibration", data.sensor_name.c_str());
    }
    
    if (data.battery_level < 20.0) {
      RCLCPP_WARN(this->get_logger(), "Low battery on sensor %s: %.1f%%", 
        data.sensor_name.c_str(), data.battery_level);
    }
    
    if (data.status_message != "OK") {
      RCLCPP_WARN(this->get_logger(), "Sensor %s status: %s", 
        data.sensor_name.c_str(), data.status_message.c_str());
    }
  }
  
  void assessSensorHealth(hw_pub_sub::msg::SystemHealth& health)
  {
    // Simulate system resource usage
    health.cpu_usage_percent = 20.0 + (message_count_ % 8) * 3.0;
    health.memory_usage_percent = 30.0 + (message_count_ % 6) * 2.0;
    health.network_latency_ms = 3.0 + (message_count_ % 4) * 1.5;
    
    // Initialize health status
    health.is_healthy = true;
    health.warnings.clear();
    health.errors.clear();
    
    // Check message rate
    if (health.message_rate_hz < 1.0) {
      health.warnings.push_back("Low sensor data rate");
      if (health.message_rate_hz < 0.1) {
        health.is_healthy = false;
        health.errors.push_back("Sensor data rate critically low");
      }
    }
    
    // Check for data staleness
    auto current_time = this->get_clock()->now();
    if (last_message_time_.nanoseconds() > 0) {
      double time_since_last = (current_time - last_message_time_).seconds();
      if (time_since_last > 5.0) {
        health.warnings.push_back("Stale sensor data");
        if (time_since_last > 30.0) {
          health.is_healthy = false;
          health.errors.push_back("Sensor data timeout");
        }
      }
    }
    
    // Check recent sensor data quality
    if (!recent_sensor_data_.empty()) {
      const auto& latest = recent_sensor_data_.back();
      
      // Check calibration status
      int uncalibrated_count = 0;
      int low_battery_count = 0;
      for (const auto& data : recent_sensor_data_) {
        if (!data.is_calibrated) uncalibrated_count++;
        if (data.battery_level < 20.0) low_battery_count++;
      }
      
      if (uncalibrated_count > recent_sensor_data_.size() / 2) {
        health.warnings.push_back("Multiple uncalibrated sensor readings");
      }
      
      if (low_battery_count > 3) {
        health.warnings.push_back("Persistent low battery warnings");
        if (latest.battery_level < 5.0) {
          health.is_healthy = false;
          health.errors.push_back("Critical battery level");
        }
      }
      
      // Check for invalid sensor status
      if (latest.status_message == "WARNING" || latest.status_message == "ERROR") {
        health.warnings.push_back("Sensor reporting issues: " + latest.status_message);
      }
    }
    
    // System resource checks
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
    
    // Calculate data quality score (0-100)
    double quality_score = 100.0;
    quality_score -= health.warnings.size() * 10.0;
    quality_score -= health.errors.size() * 25.0;
    quality_score = std::max(0.0, quality_score);
    
    RCLCPP_DEBUG(this->get_logger(), "Sensor data quality score: %.1f", quality_score);
  }

  rclcpp::Subscription<hw_pub_sub::msg::SensorData>::SharedPtr subscription_;
  rclcpp::Publisher<hw_pub_sub::msg::SystemHealth>::SharedPtr health_publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  
  std::deque<hw_pub_sub::msg::SensorData> recent_sensor_data_;
  size_t message_count_;
  bool enable_health_monitoring_;
  rclcpp::Time start_time_;
  rclcpp::Time last_message_time_;
  
  // Parameter limits
  std::vector<double> temp_limits_;
  std::vector<double> humidity_limits_;
  std::vector<double> pressure_limits_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SensorSubscriber>());
  rclcpp::shutdown();
  return 0;
}