#pragma once

#include <iomanip>
#include <unordered_map>

#include <boost/move/algo/detail/pdqsort.hpp>

#include "benchmark_runner.h"

namespace benchmark {
/// @brief Compares results of benchmark runs
class BenchmarkComparer {
private:
    std::unordered_map<std::string, long long> old_results_;

public:
    BenchmarkComparer(std::unordered_map<std::string, long long>&& old_results)
        : old_results_(std::move(old_results)) {}

    bool Compare(std::unordered_map<std::string, BenchmarkResult> const& new_results) {
        auto all_succeeded = true;
        for (auto const& [name, new_res] : new_results) {
            std::cout << "** " << name << ": ";
            // Assume benchmark cannot run 0ms
            auto const& it = old_results_.find(name);
            if (it != old_results_.end() && it->second != 0) {
                auto const& prev_res = it->second;
                auto overhead = static_cast<double>(new_res.result - prev_res) / prev_res * 100;
                auto success = overhead <= new_res.threshold;
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
};
}  // namespace benchmark
