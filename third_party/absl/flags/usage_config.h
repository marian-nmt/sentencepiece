// Lightweight Abseil flag usage configuration compatibility.
#ifndef ABSL_FLAGS_USAGE_CONFIG_H_
#define ABSL_FLAGS_USAGE_CONFIG_H_

#include <functional>
#include <string>

namespace absl {

struct FlagsUsageConfig {
  std::function<std::string()> version_string;
};

inline void SetFlagsUsageConfig(const FlagsUsageConfig&) {}

}  // namespace absl

#endif  // ABSL_FLAGS_USAGE_CONFIG_H_
