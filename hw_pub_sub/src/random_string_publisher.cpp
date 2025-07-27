#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "hw_pub_sub/utils/random_generator.hpp"

using namespace std::chrono_literals;

class RandomStringPublisher : public rclcpp::Node
{
public:
  RandomStringPublisher()
  : Node("random_string_publisher"), 
    count_(0),
    generator_(std::random_device{}())
  {
    // Declare parameters
    this->declare_parameter("publish_rate_hz", 2.0);
    this->declare_parameter("string_type", "sentence"); // word, sentence, uuid
    this->declare_parameter("word_count", 5);
    this->declare_parameter("word_length", 7);
    
    // Get parameters
    double rate = this->get_parameter("publish_rate_hz").as_double();
    string_type_ = this->get_parameter("string_type").as_string();
    word_count_ = this->get_parameter("word_count").as_int();
    word_length_ = this->get_parameter("word_length").as_int();
    
    // Create publisher with QoS
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    qos.durability(rclcpp::DurabilityPolicy::TransientLocal);
    
    publisher_ = this->create_publisher<std_msgs::msg::String>("/random_strings", qos);
    
    // Create timer
    auto timer_period = std::chrono::duration<double>(1.0 / rate);
    timer_ = this->create_wall_timer(
      timer_period, 
      std::bind(&RandomStringPublisher::timer_callback, this)
    );
    
    RCLCPP_INFO(this->get_logger(), 
      "RandomStringPublisher started - Type: %s, Rate: %.1f Hz", 
      string_type_.c_str(), rate);
  }

private:
  void timer_callback()
  {
    auto message = std_msgs::msg::String();
    
    if (string_type_ == "word") {
      message.data = generator_.generateRandomWord(word_length_);
    } else if (string_type_ == "sentence") {
      message.data = generator_.generateRandomSentence(word_count_);
    } else if (string_type_ == "uuid") {
      message.data = generator_.generateUUID();
    } else {
      message.data = "Unknown string type: " + string_type_;
    }
    
    message.data += " [" + std::to_string(count_++) + "]";
    
    RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
    publisher_->publish(message);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  hw_pub_sub::utils::RandomGenerator generator_;
  size_t count_;
  std::string string_type_;
  int word_count_;
  int word_length_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RandomStringPublisher>());
  rclcpp::shutdown();
  return 0;
}