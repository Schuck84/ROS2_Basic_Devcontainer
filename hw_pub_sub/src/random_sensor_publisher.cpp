#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "hw_pub_sub/msg/sensor_data.hpp"
#include "hw_pub_sub/utils/random_generator.hpp"

using namespace std::chrono_literals;

class RandomSensorPublisher : public rclcpp::Node
{
public:
  RandomSensorPublisher()
  : Node("random_sensor_publisher"), 
    count_(0),
    generator_(std::random_device{}())
  {
    // Declare parameters
    this->declare_parameter("publish_rate_hz", 10.0);
    this->declare_parameter("sensor_name", "imu_sensor");
    this->declare_parameter("noise_level", 0.1);
    this->declare_parameter("temperature_base", 25.0);
    this->declare_parameter("humidity_base", 60.0);
    this->declare_parameter("pressure_base", 1013.25);
    
    // Get parameters
    double rate = this->get_parameter("publish_rate_hz").as_double();
    sensor_name_ = this->get_parameter("sensor_name").as_string();
    noise_level_ = this->get_parameter("noise_level").as_double();
    temperature_base_ = this->get_parameter("temperature_base").as_double();
    humidity_base_ = this->get_parameter("humidity_base").as_double();
    pressure_base_ = this->get_parameter("pressure_base").as_double();
    
    // Create publisher with QoS for sensor data
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::BestEffort);
    qos.history(rclcpp::HistoryPolicy::KeepLast);
    
    publisher_ = this->create_publisher<hw_pub_sub::msg::SensorData>("/random_sensors", qos);
    
    // Create timer
    auto timer_period = std::chrono::duration<double>(1.0 / rate);
    timer_ = this->create_wall_timer(
      timer_period, 
      std::bind(&RandomSensorPublisher::timer_callback, this)
    );
    
    RCLCPP_INFO(this->get_logger(), 
      "RandomSensorPublisher started - Sensor: %s, Rate: %.1f Hz", 
      sensor_name_.c_str(), rate);
  }

private:
  void timer_callback()
  {
    auto message = hw_pub_sub::msg::SensorData();
    message.header.stamp = this->get_clock()->now();
    message.header.frame_id = sensor_name_;
    
    message.sensor_name = sensor_name_;
    
    // Generate environmental readings with noise
    message.temperature = generator_.generateSensorReading(temperature_base_, noise_level_);
    message.humidity = generator_.generateSensorReading(humidity_base_, noise_level_ * 10);
    message.pressure = generator_.generateSensorReading(pressure_base_, noise_level_ * 5);
    
    // Generate IMU readings (accelerometer and gyroscope)
    message.x_acceleration = generator_.generateSensorReading(0.0, noise_level_);
    message.y_acceleration = generator_.generateSensorReading(0.0, noise_level_);
    message.z_acceleration = generator_.generateSensorReading(9.81, noise_level_); // gravity
    
    message.x_gyroscope = generator_.generateSensorReading(0.0, noise_level_ * 0.1);
    message.y_gyroscope = generator_.generateSensorReading(0.0, noise_level_ * 0.1);
    message.z_gyroscope = generator_.generateSensorReading(0.0, noise_level_ * 0.1);
    
    // Generate battery level (slowly decreasing)
    static double battery_level = 100.0;
    battery_level -= generator_.generateUniform(0.0, 0.01); // Drain slowly
    if (battery_level < 0.0) battery_level = 100.0; // Reset when empty
    message.battery_level = battery_level;
    
    // Calibration status (randomly changes occasionally)
    message.is_calibrated = generator_.generateInteger(0, 100) > 5; // 95% calibrated
    
    // Status message
    if (message.is_calibrated && message.battery_level > 20.0) {
      message.status_message = "OK";
    } else if (!message.is_calibrated) {
      message.status_message = "NEEDS_CALIBRATION";
    } else if (message.battery_level <= 20.0) {
      message.status_message = "LOW_BATTERY";
    } else {
      message.status_message = "WARNING";
    }
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing SensorData: T=%.1f°C, H=%.1f%%, P=%.1fhPa, Battery=%.1f%%, Status=%s", 
      message.temperature, message.humidity, message.pressure, 
      message.battery_level, message.status_message.c_str());
    
    publisher_->publish(message);
    count_++;
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<hw_pub_sub::msg::SensorData>::SharedPtr publisher_;
  hw_pub_sub::utils::RandomGenerator generator_;
  size_t count_;
  std::string sensor_name_;
  double noise_level_;
  double temperature_base_;
  double humidity_base_;
  double pressure_base_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RandomSensorPublisher>());
  rclcpp::shutdown();
  return 0;
}