#include <gtest/gtest.h>
#include "hw_pub_sub/utils/random_generator.hpp"
#include <thread>
#include <vector>

using namespace hw_pub_sub::utils;

class RandomGeneratorTest : public ::testing::Test
{
protected:
  void SetUp() override {
    generator_ = std::make_unique<RandomGenerator>(12345); // Fixed seed for reproducibility
  }
  
  std::unique_ptr<RandomGenerator> generator_;
};

TEST_F(RandomGeneratorTest, UniformDistribution)
{
  const double min_val = -5.0;
  const double max_val = 5.0;
  const int num_samples = 1000;
  
  std::vector<double> samples;
  for (int i = 0; i < num_samples; ++i) {
    double value = generator_->generateUniform(min_val, max_val);
    samples.push_back(value);
    EXPECT_GE(value, min_val);
    EXPECT_LE(value, max_val);
  }
  
  // Check that we have reasonable distribution
  EXPECT_GT(samples.size(), 0);
}

TEST_F(RandomGeneratorTest, NormalDistribution)
{
  const double mean = 10.0;
  const double std_dev = 2.0;
  const int num_samples = 1000;
  
  std::vector<double> samples;
  double sum = 0.0;
  
  for (int i = 0; i < num_samples; ++i) {
    double value = generator_->generateNormal(mean, std_dev);
    samples.push_back(value);
    sum += value;
  }
  
  double sample_mean = sum / num_samples;
  
  // Check that sample mean is close to expected mean (within 0.5)
  EXPECT_NEAR(sample_mean, mean, 0.5);
}

TEST_F(RandomGeneratorTest, IntegerGeneration)
{
  const int min_val = 10;
  const int max_val = 20;
  const int num_samples = 100;
  
  for (int i = 0; i < num_samples; ++i) {
    int value = generator_->generateInteger(min_val, max_val);
    EXPECT_GE(value, min_val);
    EXPECT_LE(value, max_val);
  }
}

TEST_F(RandomGeneratorTest, StringGeneration)
{
  // Test word generation
  std::string word = generator_->generateRandomWord(5);
  EXPECT_EQ(word.length(), 5);
  
  // Test sentence generation
  std::string sentence = generator_->generateRandomSentence(3);
  EXPECT_FALSE(sentence.empty());
  EXPECT_NE(sentence.find('.'), std::string::npos); // Should end with period
  
  // Test UUID generation
  std::string uuid = generator_->generateUUID();
  EXPECT_EQ(uuid.length(), 36); // Standard UUID length with hyphens
  EXPECT_NE(uuid.find('-'), std::string::npos); // Should contain hyphens
}

TEST_F(RandomGeneratorTest, GeometryGeneration)
{
  // Test 3D pose generation
  auto pose = generator_->generatePose3D();
  EXPECT_EQ(pose.size(), 6); // x, y, z, roll, pitch, yaw
  
  // Test quaternion generation
  auto quat = generator_->generateQuaternion();
  EXPECT_EQ(quat.size(), 4); // w, x, y, z
  
  // Check quaternion normalization
  double norm = std::sqrt(quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] + quat[3]*quat[3]);
  EXPECT_NEAR(norm, 1.0, 1e-6);
}

TEST_F(RandomGeneratorTest, SensorReading)
{
  const double base_value = 25.0;
  const double noise_level = 0.1;
  const int num_samples = 100;
  
  std::vector<double> readings;
  for (int i = 0; i < num_samples; ++i) {
    double reading = generator_->generateSensorReading(base_value, noise_level);
    readings.push_back(reading);
  }
  
  // Calculate mean
  double sum = 0.0;
  for (double reading : readings) {
    sum += reading;
  }
  double mean = sum / readings.size();
  
  // Mean should be close to base value
  EXPECT_NEAR(mean, base_value, noise_level * 2);
}

TEST_F(RandomGeneratorTest, ThreadSafety)
{
  const int num_threads = 4;
  const int samples_per_thread = 250;
  std::vector<std::thread> threads;
  std::vector<std::vector<double>> results(num_threads);
  
  // Launch multiple threads
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, samples_per_thread, &results]() {
      for (int i = 0; i < samples_per_thread; ++i) {
        results[t].push_back(generator_->generateUniform(0.0, 1.0));
      }
    });
  }
  
  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Verify all threads generated the expected number of samples
  for (int t = 0; t < num_threads; ++t) {
    EXPECT_EQ(results[t].size(), samples_per_thread);
  }
}

TEST_F(RandomGeneratorTest, SeedReproducibility)
{
  const uint32_t seed = 54321;
  const int num_samples = 10;
  
  // Generate first sequence
  generator_->setSeed(seed);
  std::vector<double> first_sequence;
  for (int i = 0; i < num_samples; ++i) {
    first_sequence.push_back(generator_->generateUniform(0.0, 1.0));
  }
  
  // Generate second sequence with same seed
  generator_->setSeed(seed);
  std::vector<double> second_sequence;
  for (int i = 0; i < num_samples; ++i) {
    second_sequence.push_back(generator_->generateUniform(0.0, 1.0));
  }
  
  // Sequences should be identical
  EXPECT_EQ(first_sequence.size(), second_sequence.size());
  for (size_t i = 0; i < first_sequence.size(); ++i) {
    EXPECT_DOUBLE_EQ(first_sequence[i], second_sequence[i]);
  }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}