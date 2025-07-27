#include "hw_pub_sub/utils/random_generator.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace hw_pub_sub
{
namespace utils
{

const std::vector<std::string> RandomGenerator::word_list_ = {
  "robot", "sensor", "data", "message", "topic", "node", "publisher", "subscriber",
  "system", "analysis", "statistics", "random", "generator", "communication",
  "network", "protocol", "interface", "service", "client", "server", "process",
  "thread", "algorithm", "function", "method", "class", "object", "instance",
  "parameter", "configuration", "setup", "initialization", "execution", "result"
};

RandomGenerator::RandomGenerator(uint32_t seed)
  : generator_(seed), seed_(seed), uniform_dist_(0.0, 1.0), 
    normal_dist_(0.0, 1.0), exp_dist_(1.0)
{
}

double RandomGenerator::generateUniform(double min, double max)
{
  std::lock_guard<std::mutex> lock(mutex_);
  std::uniform_real_distribution<double> dist(min, max);
  return dist(generator_);
}

double RandomGenerator::generateNormal(double mean, double std_dev)
{
  std::lock_guard<std::mutex> lock(mutex_);
  std::normal_distribution<double> dist(mean, std_dev);
  return dist(generator_);
}

double RandomGenerator::generateExponential(double lambda)
{
  std::lock_guard<std::mutex> lock(mutex_);
  std::exponential_distribution<double> dist(lambda);
  return dist(generator_);
}

int RandomGenerator::generateInteger(int min, int max)
{
  std::lock_guard<std::mutex> lock(mutex_);
  std::uniform_int_distribution<int> dist(min, max);
  return dist(generator_);
}

std::string RandomGenerator::generateRandomWord(size_t length)
{
  if (length == 0) {
    // Return a word from the predefined list
    std::lock_guard<std::mutex> lock(mutex_);
    std::uniform_int_distribution<size_t> dist(0, word_list_.size() - 1);
    return word_list_[dist(generator_)];
  }
  
  std::lock_guard<std::mutex> lock(mutex_);
  std::string word;
  std::uniform_int_distribution<int> char_dist('a', 'z');
  
  for (size_t i = 0; i < length; ++i) {
    word += static_cast<char>(char_dist(generator_));
  }
  
  return word;
}

std::string RandomGenerator::generateRandomSentence(size_t word_count)
{
  std::string sentence;
  for (size_t i = 0; i < word_count; ++i) {
    if (i > 0) sentence += " ";
    sentence += generateRandomWord(0); // Use predefined words
  }
  sentence += ".";
  
  // Capitalize first letter
  if (!sentence.empty()) {
    sentence[0] = std::toupper(sentence[0]);
  }
  
  return sentence;
}

std::string RandomGenerator::generateUUID()
{
  std::lock_guard<std::mutex> lock(mutex_);
  std::stringstream ss;
  std::uniform_int_distribution<int> hex_dist(0, 15);
  
  for (int i = 0; i < 32; ++i) {
    if (i == 8 || i == 12 || i == 16 || i == 20) {
      ss << "-";
    }
    ss << std::hex << hex_dist(generator_);
  }
  
  return ss.str();
}

std::vector<double> RandomGenerator::generatePose3D()
{
  return {
    generateUniform(-10.0, 10.0), // x
    generateUniform(-10.0, 10.0), // y
    generateUniform(-10.0, 10.0), // z
    generateUniform(-M_PI, M_PI), // roll
    generateUniform(-M_PI, M_PI), // pitch
    generateUniform(-M_PI, M_PI)  // yaw
  };
}

std::vector<double> RandomGenerator::generateQuaternion()
{
  // Generate a normalized quaternion
  double w = generateNormal(0.0, 1.0);
  double x = generateNormal(0.0, 1.0);
  double y = generateNormal(0.0, 1.0);
  double z = generateNormal(0.0, 1.0);
  
  double norm = std::sqrt(w*w + x*x + y*y + z*z);
  
  return {w/norm, x/norm, y/norm, z/norm};
}

double RandomGenerator::generateSensorReading(double base_value, double noise_level)
{
  double noise = generateNormal(0.0, noise_level);
  return base_value + noise;
}

void RandomGenerator::setSeed(uint32_t seed)
{
  std::lock_guard<std::mutex> lock(mutex_);
  seed_ = seed;
  generator_.seed(seed);
}

} // namespace utils
} // namespace hw_pub_sub