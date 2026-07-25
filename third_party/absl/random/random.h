// C++17 random compatibility for standalone SentencePiece.
#ifndef ABSL_RANDOM_RANDOM_H_
#define ABSL_RANDOM_RANDOM_H_

#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>

namespace absl {

class BitGen {
 public:
  using result_type = std::mt19937_64::result_type;

  BitGen() : engine_(std::random_device{}()) {}
  explicit BitGen(std::seed_seq&& sequence) : engine_(sequence) {}

  static constexpr result_type min() { return std::mt19937_64::min(); }
  static constexpr result_type max() { return std::mt19937_64::max(); }
  result_type operator()() { return engine_(); }

 private:
  std::mt19937_64 engine_;
};

template <class Result, class Generator>
Result Uniform(Generator& generator, Result low, Result high) {
  if (high <= low) return low;
  if constexpr (std::is_integral_v<Result>) {
    std::uniform_int_distribution<Result> distribution(low, high - 1);
    return distribution(generator);
  } else {
    std::uniform_real_distribution<Result> distribution(low, high);
    return distribution(generator);
  }
}

template <class Generator, class Probability>
bool Bernoulli(Generator& generator, Probability probability) {
  return std::bernoulli_distribution(static_cast<double>(probability))(generator);
}

template <class Result, class Generator, class Mean, class StdDev>
Result Gaussian(Generator& generator, Mean mean, StdDev stddev) {
  std::normal_distribution<Result> distribution(static_cast<Result>(mean),
                                                 static_cast<Result>(stddev));
  return distribution(generator);
}

}  // namespace absl

#endif  // ABSL_RANDOM_RANDOM_H_
