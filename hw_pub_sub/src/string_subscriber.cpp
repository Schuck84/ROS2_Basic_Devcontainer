#include <memory>
#include <string>
#include <regex>
#include <algorithm>
#include <sstream>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "hw_pub_sub/msg/string_analysis.hpp"

class StringSubscriber : public rclcpp::Node
{
public:
  StringSubscriber()
  : Node("string_subscriber"), message_count_(0)
  {
    // Declare parameters
    this->declare_parameter("enable_analysis_publishing", true);
    this->declare_parameter("analysis_rate_hz", 1.0);
    
    // Get parameters
    enable_analysis_publishing_ = this->get_parameter("enable_analysis_publishing").as_bool();
    double analysis_rate = this->get_parameter("analysis_rate_hz").as_double();
    
    // Create subscriber with QoS
    rclcpp::QoS qos(10);
    qos.reliability(rclcpp::ReliabilityPolicy::Reliable);
    
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      "/random_strings", qos,
      std::bind(&StringSubscriber::topic_callback, this, std::placeholders::_1));
    
    // Create analysis publisher if enabled
    if (enable_analysis_publishing_) {
      analysis_publisher_ = this->create_publisher<hw_pub_sub::msg::StringAnalysis>(
        "/string_analysis", qos);
      
      // Create timer for periodic analysis
      auto timer_period = std::chrono::duration<double>(1.0 / analysis_rate);
      timer_ = this->create_wall_timer(
        timer_period, 
        std::bind(&StringSubscriber::analysis_timer_callback, this)
      );
    }
    
    RCLCPP_INFO(this->get_logger(), "StringSubscriber started");
  }

private:
  void topic_callback(const std_msgs::msg::String::SharedPtr msg)
  {
    RCLCPP_INFO(this->get_logger(), "Received: '%s'", msg->data.c_str());
    
    // Store for analysis
    recent_strings_.push_back(msg->data);
    if (recent_strings_.size() > 100) { // Keep only last 100 strings
      recent_strings_.erase(recent_strings_.begin());
    }
    
    // Perform immediate analysis
    analyzeString(msg->data);
    message_count_++;
  }
  
  void analysis_timer_callback()
  {
    if (recent_strings_.empty()) return;
    
    // Analyze the most recent string
    auto message = hw_pub_sub::msg::StringAnalysis();
    message.header.stamp = this->get_clock()->now();
    message.header.frame_id = "string_analysis";
    
    const std::string& text = recent_strings_.back();
    performDetailedAnalysis(text, message);
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Publishing StringAnalysis: words=%d, chars=%d, type=%s", 
      message.word_count, message.character_count, message.text_type.c_str());
    
    analysis_publisher_->publish(message);
  }
  
  void analyzeString(const std::string& text)
  {
    // Basic analysis for logging
    int word_count = countWords(text);
    int char_count = text.length();
    bool has_numbers = std::regex_search(text, std::regex(R"(\d)"));
    bool has_special = std::regex_search(text, std::regex(R"([^a-zA-Z0-9\s])"));
    
    RCLCPP_DEBUG(this->get_logger(), 
      "Analysis: %d chars, %d words, numbers: %s, special: %s",
      char_count, word_count, has_numbers ? "yes" : "no", has_special ? "yes" : "no");
  }
  
  void performDetailedAnalysis(const std::string& text, hw_pub_sub::msg::StringAnalysis& analysis)
  {
    analysis.original_text = text;
    analysis.character_count = text.length();
    analysis.word_count = countWords(text);
    analysis.sentence_count = countSentences(text);
    
    // Analyze words
    auto words = extractWords(text);
    analysis.unique_words = getUniqueWords(words);
    
    if (!words.empty()) {
      double total_length = 0.0;
      for (const auto& word : words) {
        total_length += word.length();
      }
      analysis.average_word_length = total_length / words.size();
      
      // Find most common word
      auto word_freq = getWordFrequency(words);
      auto most_common = std::max_element(word_freq.begin(), word_freq.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
      
      if (most_common != word_freq.end()) {
        analysis.most_common_word = most_common->first;
        analysis.most_common_word_count = most_common->second;
      }
    }
    
    // Check for patterns
    analysis.contains_numbers = std::regex_search(text, std::regex(R"(\d)"));
    analysis.contains_special_chars = std::regex_search(text, std::regex(R"([^a-zA-Z0-9\s])"));
    
    // Determine text type
    if (std::regex_search(text, std::regex(R"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})"))) {
      analysis.text_type = "UUID";
    } else if (analysis.sentence_count > 0 && analysis.word_count > 1) {
      analysis.text_type = "SENTENCE";
    } else if (analysis.word_count == 1) {
      analysis.text_type = "WORD";
    } else {
      analysis.text_type = "UNKNOWN";
    }
    
    // Simple readability score (average word length + sentence complexity)
    double avg_words_per_sentence = analysis.sentence_count > 0 ? 
      static_cast<double>(analysis.word_count) / analysis.sentence_count : 0.0;
    analysis.readability_score = analysis.average_word_length + avg_words_per_sentence * 0.5;
  }
  
  int countWords(const std::string& text)
  {
    std::istringstream iss(text);
    int count = 0;
    std::string word;
    while (iss >> word) {
      count++;
    }
    return count;
  }
  
  int countSentences(const std::string& text)
  {
    return std::count_if(text.begin(), text.end(), 
      [](char c) { return c == '.' || c == '!' || c == '?'; });
  }
  
  std::vector<std::string> extractWords(const std::string& text)
  {
    std::vector<std::string> words;
    std::regex word_regex(R"(\b\w+\b)");
    std::sregex_iterator iter(text.begin(), text.end(), word_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
      std::string word = iter->str();
      std::transform(word.begin(), word.end(), word.begin(), ::tolower);
      words.push_back(word);
    }
    
    return words;
  }
  
  std::vector<std::string> getUniqueWords(const std::vector<std::string>& words)
  {
    std::set<std::string> unique_set(words.begin(), words.end());
    return std::vector<std::string>(unique_set.begin(), unique_set.end());
  }
  
  std::map<std::string, int> getWordFrequency(const std::vector<std::string>& words)
  {
    std::map<std::string, int> frequency;
    for (const auto& word : words) {
      frequency[word]++;
    }
    return frequency;
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  rclcpp::Publisher<hw_pub_sub::msg::StringAnalysis>::SharedPtr analysis_publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  
  std::vector<std::string> recent_strings_;
  size_t message_count_;
  bool enable_analysis_publishing_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<StringSubscriber>());
  rclcpp::shutdown();
  return 0;
}