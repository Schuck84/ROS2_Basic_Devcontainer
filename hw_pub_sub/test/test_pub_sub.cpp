#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <memory>
#include <chrono>

using namespace std::chrono_literals;

class PubSubTest : public ::testing::Test
{
protected:
  void SetUp() override {
    rclcpp::init(0, nullptr);
    node_ = std::make_shared<rclcpp::Node>("test_node");
  }
  
  void TearDown() override {
    rclcpp::shutdown();
  }
  
  std::shared_ptr<rclcpp::Node> node_;
};

TEST_F(PubSubTest, BasicPublishSubscribe)
{
  std::string received_message;
  bool message_received = false;
  
  // Create subscriber
  auto subscription = node_->create_subscription<std_msgs::msg::String>(
    "test_topic", 10,
    [&](std_msgs::msg::String::UniquePtr msg) {
      received_message = msg->data;
      message_received = true;
    });
  
  // Create publisher
  auto publisher = node_->create_publisher<std_msgs::msg::String>("test_topic", 10);
  
  // Give some time for discovery
  rclcpp::sleep_for(100ms);
  
  // Publish a message
  auto message = std_msgs::msg::String();
  message.data = "Hello, Test!";
  publisher->publish(message);
  
  // Spin until message is received or timeout
  auto start_time = std::chrono::steady_clock::now();
  while (!message_received && 
         std::chrono::steady_clock::now() - start_time < 1s) {
    rclcpp::spin_some(node_);
    rclcpp::sleep_for(10ms);
  }
  
  EXPECT_TRUE(message_received);
  EXPECT_EQ(received_message, "Hello, Test!");
}

TEST_F(PubSubTest, MultipleMessages)
{
  std::vector<std::string> received_messages;
  const int num_messages = 5;
  
  // Create subscriber
  auto subscription = node_->create_subscription<std_msgs::msg::String>(
    "test_topic_multi", 10,
    [&](std_msgs::msg::String::UniquePtr msg) {
      received_messages.push_back(msg->data);
    });
  
  // Create publisher
  auto publisher = node_->create_publisher<std_msgs::msg::String>("test_topic_multi", 10);
  
  // Give some time for discovery
  rclcpp::sleep_for(100ms);
  
  // Publish multiple messages
  for (int i = 0; i < num_messages; ++i) {
    auto message = std_msgs::msg::String();
    message.data = "Message " + std::to_string(i);
    publisher->publish(message);
    rclcpp::sleep_for(10ms); // Small delay between messages
  }
  
  // Spin until all messages are received or timeout
  auto start_time = std::chrono::steady_clock::now();
  while (received_messages.size() < num_messages && 
         std::chrono::steady_clock::now() - start_time < 2s) {
    rclcpp::spin_some(node_);
    rclcpp::sleep_for(10ms);
  }
  
  EXPECT_EQ(received_messages.size(), num_messages);
  for (int i = 0; i < num_messages; ++i) {
    EXPECT_EQ(received_messages[i], "Message " + std::to_string(i));
  }
}

TEST_F(PubSubTest, QoSReliability)
{
  int messages_received = 0;
  
  // Create subscriber with reliable QoS
  rclcpp::QoS qos(10);
  qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
  
  auto subscription = node_->create_subscription<std_msgs::msg::String>(
    "test_topic_qos", qos,
    [&](std_msgs::msg::String::UniquePtr msg) {
      messages_received++;
    });
  
  // Create publisher with reliable QoS
  auto publisher = node_->create_publisher<std_msgs::msg::String>("test_topic_qos", qos);
  
  // Give time for QoS negotiation
  rclcpp::sleep_for(200ms);
  
  // Publish messages rapidly
  const int num_messages = 10;
  for (int i = 0; i < num_messages; ++i) {
    auto message = std_msgs::msg::String();
    message.data = "QoS Message " + std::to_string(i);
    publisher->publish(message);
  }
  
  // Spin until messages are received or timeout
  auto start_time = std::chrono::steady_clock::now();
  while (messages_received < num_messages && 
         std::chrono::steady_clock::now() - start_time < 3s) {
    rclcpp::spin_some(node_);
    rclcpp::sleep_for(10ms);
  }
  
  // With reliable QoS, we should receive all messages
  EXPECT_EQ(messages_received, num_messages);
}

class ParameterizedPubSubTest : public PubSubTest, 
                               public ::testing::WithParamInterface<std::string>
{
};

TEST_P(ParameterizedPubSubTest, DifferentTopicNames)
{
  std::string topic_name = GetParam();
  std::string received_message;
  bool message_received = false;
  
  auto subscription = node_->create_subscription<std_msgs::msg::String>(
    topic_name, 10,
    [&](std_msgs::msg::String::UniquePtr msg) {
      received_message = msg->data;
      message_received = true;
    });
  
  auto publisher = node_->create_publisher<std_msgs::msg::String>(topic_name, 10);
  
  rclcpp::sleep_for(100ms);
  
  auto message = std_msgs::msg::String();
  message.data = "Test message for " + topic_name;
  publisher->publish(message);
  
  auto start_time = std::chrono::steady_clock::now();
  while (!message_received && 
         std::chrono::steady_clock::now() - start_time < 1s) {
    rclcpp::spin_some(node_);
    rclcpp::sleep_for(10ms);
  }
  
  EXPECT_TRUE(message_received);
  EXPECT_EQ(received_message, "Test message for " + topic_name);
}

INSTANTIATE_TEST_SUITE_P(
  TopicNames,
  ParameterizedPubSubTest,
  ::testing::Values(
    "test_topic_1",
    "test_topic_2", 
    "random_strings",
    "string_analysis"
  )
);

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}