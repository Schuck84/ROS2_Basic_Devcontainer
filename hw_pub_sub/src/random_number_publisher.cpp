#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "hw_pub_sub/msg/number_statistics.hpp"
#include "hw_pub_sub/utils/random_generator.hpp"

using namespace std::chrono_literals;

class RandomNumberPublisher : public rclcpp::Node
{
public:
  RandomNumberPublisher()
  : Node("random_number_publisher"), 
    count_(0),
    generator_(std::random_device{}())
  {
    // Declare parameters
    this->declare_parameter("publish_rate_hz", 1.0);
    this->declare_parameter("distribution_type", "normal"); // uniform, normal, exponential
    this->declare_parameter("data_points", 10);
    this->declare_parameter("mean_value", 0.0);
    this->declare_parameter("std_deviation", 1.0);
    this->declare_parameter("min_value", -10.0);
    this->declare_parameter("max_value", 10.0);
    
    // Get parameters
    double rate = this->get_parameter("publish_rate_hz").as_double();
    distribution_type_ = this->get_parameter("distribution_type").as_string();
    data_points_ = this->get_parameter("data_points").as_int();
    mean_value_ = this->get_parameter("mean_value").as_double();
    std_deviation_ = this->get_parameter("std_deviation").as_double();
    min_value_ = this->get_parameter("min_value").as_double();
    max_value_ = this->get_parameter("max_value").as_double();
    
    // Create publisher with QoS
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    
    publisher_ = this->create_publisher<hw_pub_sub::msg::NumberStatistics>("/random_numbers", qos);
    
    // Create timer
    auto timer_period = std::chrono::duration<double>(1.0 / rate);
    timer_ = this->create_wall_timer(
      timer_period, 
      std::bind(&RandomNumberPublisher::timer_callback, this)
    );
    
    RCLCPP_INFO(this->get_logger(), 
      "RandomNumberPublisher started - Distribution: %s, Rate: %.1f Hz", 
      distribution_type_.c_str(), rate);
  }

private:
  void timer_callback()
  {
    auto message = hw_pub_sub::msg::NumberStatistics();
    message.header.stamp = this->get_clock()->now();
    message.header.frame_id = "base_link";
    
    // Generate data sequence
    std::vector<double> data;
    for (int i = 0; i < data_points_; ++i) {
      double value;
      if (distribution_type_ == "uniform") {
        value = generator_.generateUniform(min_value_, max_value_);
      } else if (distribution_type_ == "normal") {
        value = generator_.generateNormal(mean_value_, std_deviation_);
      } else if (distribution_type_ == "exponential") {
        value = generator_.generateExponential(1.0 / mean_value_);
      } else {
        value = generator_.generateUniform(0.0, 1.0);
      }
      data.push_back(value);
    }
    
    message.data_sequence = data;
    message.distribution_type = distribution_type_;
    message.count = count_++;
    
    // Calculate statistics
    calculateStatistics(data, message);
    
    RCLCPP_INFO(this->get_logger(), 
      "Publishing NumberStatistics: mean=%.3f, std=%.3f, count=%d", 
      message.mean, message.standard_deviation, message.count);
    
    publisher_->publish(message);
  }
  
  void calculateStatistics(const std::vector<double>& data, hw_pub_sub::msg::NumberStatistics& stats)
  {
    if (data.empty()) return;
    
    // Calculate mean
    double sum = 0.0;
    for (double value : data) {
      sum += value;
    }
    stats.mean = sum / data.size();
    stats.sum = sum;
    
    // Calculate variance and standard deviation
    double variance = 0.0;
    for (double value : data) {
      variance += (value - stats.mean) * (value - stats.mean);
    }
    variance /= data.size();
    stats.variance = variance;
    stats.standard_deviation = std::sqrt(variance);
    
    // Find min and max
    auto minmax = std::minmax_element(data.begin(), data.end());
    stats.min_value = *minmax.first;
    stats.max_value = *minmax.second;
    
    // Calculate median
    std::vector<double> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());
    size_t n = sorted_data.size();
    if (n % 2 == 0) {
      stats.median = (sorted_data[n/2 - 1] + sorted_data[n/2]) / 2.0;
    } else {
      stats.median = sorted_data[n/2];
    }
    
    // Simple trend calculation (slope of linear regression)
    double x_mean = (data.size() - 1) / 2.0;
    double numerator = 0.0, denominator = 0.0;
    for (size_t i = 0; i < data.size(); ++i) {
      numerator += (i - x_mean) * (data[i] - stats.mean);
      denominator += (i - x_mean) * (i - x_mean);
    }
    stats.trend_slope = (denominator != 0.0) ? numerator / denominator : 0.0;
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<hw_pub_sub::msg::NumberStatistics>::SharedPtr publisher_;
  hw_pub_sub::utils::RandomGenerator generator_;
  size_t count_;
  std::string distribution_type_;
  int data_points_;
  double mean_value_;
  double std_deviation_;
  double min_value_;
  double max_value_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RandomNumberPublisher>());
  rclcpp::shutdown();
  return 0;
}