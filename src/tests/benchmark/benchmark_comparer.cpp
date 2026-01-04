#include "tests/benchmark/benchmark_comparer.h"

#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace benchmark {
bool BenchmarkComparer::Compare(
        std::unordered_map<std::string, long long> const& old_results,
        std::unordered_map<std::string, long long> const& new_results) const {
    auto all_succeeded = true;
    for (auto const& [name, new_res] : new_results) {
        std::cout << "** " << name << ": ";
        // Assume benchmark cannot run 0ms
        auto const it = old_results.find(name);
        if (it != old_results.end() && it->second != 0) {
            auto threshold = kDefaultThreshold;
            auto const threshold_it = thresholds_.find(name);
            if (threshold_it != thresholds_.end()) {
                threshold = threshold_it->second;
            }

            auto const prev_res = it->second;
            auto const overhead = static_cast<double>(new_res - prev_res) / prev_res * 100;
            auto const success = overhead <= threshold;
            std::cout << (success ? "SUCCESS" : "FAIL") << ": " << std::setprecision(3)
                      << std::abs(overhead) << "% " << (overhead > 0 ? "slower" : "faster")
                      << " than previous run **\n";
            all_succeeded = all_succeeded && success;
        } else {
            std::cout << "WARNING: hasn't been run before **\n";
        }
    }
    return all_succeeded;
}
}  // namespace benchmark
