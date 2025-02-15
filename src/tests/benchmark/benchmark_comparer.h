#pragma once

#include <string>
#include <unordered_map>

namespace benchmark {
/// @brief Compares results of benchmark runs
class BenchmarkComparer {
private:
    using Results = std::unordered_map<std::string, long long>;

    constexpr static unsigned char kDefaultThreshold = 10;

    std::unordered_map<std::string, unsigned char> thresholds_;

public:
    void SetThreshold(std::string const& name, unsigned char threshold) {
        thresholds_.emplace(name, threshold);
    }

    bool Compare(Results const& old_results, Results const& new_results) const;
};
}  // namespace benchmark
