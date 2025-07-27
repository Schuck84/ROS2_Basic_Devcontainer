#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include "hw_pub_sub/msg/sensor_data.hpp"
#include "hw_pub_sub/msg/number_statistics.hpp"
#include "hw_pub_sub/msg/string_analysis.hpp"
#include <memory>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

class IntegrationTest : public ::testing::Test
{
protected:
  void SetUp() override {
    rclcpp::init(0, nullptr);
    test_node_ = std::make_shared<rclcpp::Node>("integration_test_node");
  }
  
  void TearDown() override {
    rclcpp::shutdown();
  }
  
  std::shared_ptr<rclcpp::Node> test_node_;
};

TEST_F(IntegrationTest, StringPubSubIntegration)
{
  std::vector<std::string> received_strings;
  bool analysis_received = false;
  
  // Create subscribers
  auto string_sub = test_node_->create_subscription<std_msgs::msg::String>(
    "/random_strings", 10,
    [&](std_msgs::msg::String::UniquePtr msg) {
      received_strings.push_back(msg->data);
    });
  
  auto analysis_sub = test_node_->create_subscription<hw_pub_sub::msg::StringAnalysis>(
    "/string_analysis", 10,
    [&](hw_pub_sub::msg::StringAnalysis::UniquePtr msg) {
      analysis_received = true;
      EXPECT_GT(msg->character_count, 0);
      EXPECT_GT(msg->word_count, 0);
    });
  
  // Create string publisher (simulate random_string_publisher)
  auto string_pub = test_node_->create_publisher<std_msgs::msg::String>("/random_strings", 10);
  
  // Wait for discovery
  std::this_thread::sleep_for(200ms);
  
  // Publish test messages
  for (int i = 0; i < 5; ++i) {
    auto msg = std_msgs::msg::String();
    msg.data = "Test message " + std::to_string(i) + " with multiple words.";
    string_pub->publish(msg);
    
    // Spin to process messages
    rclcpp::spin_some(test_node_);
    std::this_thread::sleep_for(50ms);
  }
  
  EXPECT_EQ(received_strings.size(), 5);
  for (size_t i = 0; i < received_strings.size(); ++i) {
    EXPECT_TRUE(received_strings[i].find("Test message " + std::to_string(i)) != std::string::npos);
  }
}

TEST_F(IntegrationTest, SensorDataIntegration)
{
  std::vector<hw_pub_sub::msg::SensorData> received_sensor_data;
  
  auto sensor_sub = test_node_->create_subscription<hw_pub_sub::msg::SensorData>(
    "/random_sensors", 10,
    [&](hw_pub_sub::msg::SensorData::UniquePtr msg) {
      received_sensor_data.push_back(*msg);
    });
  
  auto sensor_pub = test_node_->create_publisher<hw_pub_sub::msg::SensorData>("/random_sensors", 10);
  
  std::this_thread::sleep_for(100ms);
  
  // Publish test sensor data
  for (int i = 0; i < 3; ++i) {
    auto msg = hw_pub_sub::msg::SensorData();
    msg.header.stamp = test_node_->get_clock()->now();
    msg.sensor_name = "test_sensor_" + std::to_string(i);
    msg.temperature = 25.0 + i;
    msg.humidity = 60.0 + i * 5;
    msg.pressure = 1013.25 + i * 10;
    msg.battery_level = 100.0 - i * 10;
    msg.is_calibrated = true;
    msg.status_message = "OK";
    
    sensor_pub->publish(msg);
    rclcpp::spin_some(test_node_);
    std::this_thread::sleep_for(50ms);
  }
  
  EXPECT_EQ(received_sensor_data.size(), 3);
  for (size_t i = 0; i < received_sensor_data.size(); ++i) {
    EXPECT_EQ(received_sensor_data[i].sensor_name, "test_sensor_" + std::to_string(i));
    EXPECT_NEAR(received_sensor_data[i].temperature, 25.0 + i, 0.1);
    EXPECT_TRUE(received_sensor_data[i].is_calibrated);
  }
}

TEST_F(IntegrationTest, NumberStatisticsIntegration)
{
  hw_pub_sub::msg::NumberStatistics received_stats;
  bool stats_received = false;
  
  auto stats_sub = test_node_->create_subscription<hw_pub_sub::msg::NumberStatistics>(
    "/random_numbers", 10,
    [&](hw_pub_sub::msg::NumberStatistics::UniquePtr msg) {
      received_stats = *msg;
      stats_received = true;
    });
  
  auto stats_pub = test_node_->create_publisher<hw_pub_sub::msg::NumberStatistics>("/random_numbers", 10);
  
  std::this_thread::sleep_for(100ms);
  
  // Create test statistics
  auto msg = hw_pub_sub::msg::NumberStatistics();
  msg.header.stamp = test_node_->get_clock()->now();
  msg.data_sequence = {1.0, 2.0, 3.0, 4.0, 5.0};
  msg.mean = 3.0;
  msg.median = 3.0;
  msg.standard_deviation = 1.58;
  msg.variance = 2.5;
  msg.min_value = 1.0;
  msg.max_value = 5.0;
  msg.sum = 15.0;
  msg.count = 5;
  msg.trend_slope = 1.0;
  msg.distribution_type = "test";
  
  stats_pub->publish(msg);
  
  // Wait for message processing
  auto start_time = std::chrono::steady_clock::now();
  while (!stats_received && 
         std::chrono::steady_clock::now() - start_time < 1s) {
    rclcpp::spin_some(test_node_);
    std::this_thread::sleep_for(10ms);
  }
  
  EXPECT_TRUE(stats_received);
  EXPECT_EQ(received_stats.data_sequence.size(), 5);
  EXPECT_NEAR(received_stats.mean, 3.0, 0.1);
  EXPECT_NEAR(received_stats.trend_slope, 1.0, 0.1);
  EXPECT_EQ(received_stats.distribution_type, "test");
}

TEST_F(IntegrationTest, MultipleTopicsConcurrent)
{
  int string_count = 0;
  int sensor_count = 0;
  int stats_count = 0;
  
  // Create subscribers for multiple topics
  auto string_sub = test_node_->create_subscription<std_msgs::msg::String>(
    "/random_strings", 10,
    [&](std_msgs::msg::String::UniquePtr) { string_count++; });
  
  auto sensor_sub = test_node_->create_subscription<hw_pub_sub::msg::SensorData>(
    "/random_sensors", 10,
    [&](hw_pub_sub::msg::SensorData::UniquePtr) { sensor_count++; });
  
  auto stats_sub = test_node_->create_subscription<hw_pub_sub::msg::NumberStatistics>(
    "/random_numbers", 10,
    [&](hw_pub_sub::msg::NumberStatistics::UniquePtr) { stats_count++; });
  
  // Create publishers
  auto string_pub = test_node_->create_publisher<std_msgs::msg::String>("/random_strings", 10);
  auto sensor_pub = test_node_->create_publisher<hw_pub_sub::msg::SensorData>("/random_sensors", 10);
  auto stats_pub = test_node_->create_publisher<hw_pub_sub::msg::NumberStatistics>("/random_numbers", 10);
  
  std::this_thread::sleep_for(200ms);
  
  // Publish messages concurrently
  const int num_messages = 10;
  for (int i = 0; i < num_messages; ++i) {
    // String message
    auto string_msg = std_msgs::msg::String();
    string_msg.data = "Concurrent test " + std::to_string(i);
    string_pub->publish(string_msg);
    
    // Sensor message
    auto sensor_msg = hw_pub_sub::msg::SensorData();
    sensor_msg.header.stamp = test_node_->get_clock()->now();
    sensor_msg.sensor_name = "concurrent_sensor";
    sensor_msg.temperature = 20.0 + i;
    sensor_msg.is_calibrated = true;
    sensor_pub->publish(sensor_msg);
    
    // Stats message
    auto stats_msg = hw_pub_sub::msg::NumberStatistics();
    stats_msg.header.stamp = test_node_->get_clock()->now();
    stats_msg.count = i;
    stats_msg.mean = i * 2.0;
    stats_pub->publish(stats_msg);
    
    rclcpp::spin_some(test_node_);
    std::this_thread::sleep_for(20ms);
  }
  
  // Final spin to ensure all messages are processed
  for (int i = 0; i < 10; ++i) {
    rclcpp::spin_some(test_node_);
    std::this_thread::sleep_for(50ms);
  }
  
  EXPECT_EQ(string_count, num_messages);
  EXPECT_EQ(sensor_count, num_messages);
  EXPECT_EQ(stats_count, num_messages);
}

TEST_F(IntegrationTest, QoSReliabilityTest)
{
  int reliable_count = 0;
  int best_effort_count = 0;
  
  // Reliable subscriber
  rclcpp::QoS reliable_qos(10);
  reliable_qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
  
  auto reliable_sub = test_node_->create_subscription<std_msgs::msg::String>(
    "/reliable_topic", reliable_qos,
    [&](std_msgs::msg::String::UniquePtr) { reliable_count++; });
  
  // Best effort subscriber
  rclcpp::QoS best_effort_qos(10);
  best_effort_qos.reliability(rclcpp::ReliabilityPolicy::BestEffort);
  
  auto best_effort_sub = test_node_->create_subscription<std_msgs::msg::String>(
    "/best_effort_topic", best_effort_qos,
    [&](std_msgs::msg::String::UniquePtr) { best_effort_count++; });
  
  // Publishers with matching QoS
  auto reliable_pub = test_node_->create_publisher<std_msgs::msg::String>("/reliable_topic", reliable_qos);
  auto best_effort_pub = test_node_->create_publisher<std_msgs::msg::String>("/best_effort_topic", best_effort_qos);
  
  std::this_thread::sleep_for(300ms); // Allow time for QoS negotiation
  
  // Publish messages rapidly
  const int num_messages = 20;
  for (int i = 0; i < num_messages; ++i) {
    auto msg = std_msgs::msg::String();
    msg.data = "QoS test " + std::to_string(i);
    
    reliable_pub->publish(msg);
    best_effort_pub->publish(msg);
    
    if (i % 5 == 0) { // Spin occasionally
      rclcpp::spin_some(test_node_);
    }
    std::this_thread::sleep_for(5ms);
  }
  
  // Allow processing time
  for (int i = 0; i < 20; ++i) {
    rclcpp::spin_some(test_node_);
    std::this_thread::sleep_for(50ms);
  }
  
  // Reliable should receive all messages
  EXPECT_EQ(reliable_count, num_messages);
  // Best effort might lose some messages, but should receive most
  EXPECT_GE(best_effort_count, num_messages * 0.8); // At least 80%
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}