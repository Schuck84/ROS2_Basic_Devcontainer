#ifndef HW_PUB_SUB__UTILS__RANDOM_GENERATOR_HPP_
#define HW_PUB_SUB__UTILS__RANDOM_GENERATOR_HPP_

#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace hw_pub_sub
{
namespace utils
{

/**
 * @brief Thread-safe random data generator with multiple statistical distributions
 */
class RandomGenerator
{
public:
  explicit RandomGenerator(uint32_t seed = std::random_device{}());
  
  // Number generation
  double generateUniform(double min = 0.0, double max = 1.0);
  double generateNormal(double mean = 0.0, double std_dev = 1.0);
  double generateExponential(double lambda = 1.0);
  int generateInteger(int min = 0, int max = 100);
  
  // String generation
  std::string generateRandomWord(size_t length = 5);
  std::string generateRandomSentence(size_t word_count = 5);
  std::string generateUUID();
  
  // Geometry generation
  std::vector<double> generatePose3D();
  std::vector<double> generateQuaternion();
  
  // Sensor data generation
  double generateSensorReading(double base_value, double noise_level = 0.1);
  
  // Utility functions
  void setSeed(uint32_t seed);
  uint32_t getSeed() const { return seed_; }
  
private:
  std::mt19937 generator_;
  uint32_t seed_;
  mutable std::mutex mutex_;
  
  // Common distributions
  std::uniform_real_distribution<double> uniform_dist_;
  std::normal_distribution<double> normal_dist_;
  std::exponential_distribution<double> exp_dist_;
  
  // Word lists for string generation
  static const std::vector<std::string> word_list_;
};

} // namespace utils
} // namespace hw_pub_sub

#endif // HW_PUB_SUB__UTILS__RANDOM_GENERATOR_HPP_