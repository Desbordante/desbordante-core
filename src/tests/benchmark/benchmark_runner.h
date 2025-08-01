#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/core/demangle.hpp>

#include "algorithms/algo_factory.h"
#include "config/names.h"
#include "parser/csv_parser/csv_parser.h"

namespace benchmark {
struct Benchmark {
    std::function<void()> body;
    unsigned char threshold;
};

struct BenchmarkResult {
    long long result;
    unsigned char threshold;
};

/// @brief Runs benchmars and tracks their execution time
class BenchmarkRunner {
private:
    std::unordered_map<std::string, Benchmark> benchmarks_;
    std::unordered_map<std::string, BenchmarkResult> bm_results_;

    /// @brief Demangle name and take only class name (without namespaces).
    std::string GetAlgoName(std::string const& mangled_name) {
        auto demangled_name = boost::core::demangle(mangled_name.c_str());
        auto colon_pos = demangled_name.rfind(':');
        if (colon_pos == std::string::npos) {
            return demangled_name;
        }
        return demangled_name.substr(colon_pos + 1);
    }

public:
    /// @brief Register a benchmark that simply calls and measures Algo.Execute().
    /// @param csv -- @c CSVConfig to run algo on
    /// @param other_params -- @c StdParamsMap with other algo options.
    /// It may or may not contain @c CSVConfig -- @c csv will be used anyway
    /// @param name_suffix -- suffix that will be added to benchmark name.
    /// Can be used to distinguish different runs of a single algo
    /// @param threshold -- threshold (per cent)
    template <typename Algo>
    void RegisterSimpleBenchmark(CSVConfig const& csv, algos::StdParamsMap&& other_params = {},
                                 std::string const& name_suffix = "",
                                 unsigned char threshold = 10) {
        std::ostringstream name;
        name << GetAlgoName(typeid(Algo).name()) << ", " << csv.path.stem().string();
        if (!name_suffix.empty()) {
            name << ", " << name_suffix;
        }

        other_params[config::names::kCsvConfig] = csv;

        auto bm_body = [other_params] {
            auto algo = algos::CreateAndLoadAlgorithm<Algo>(other_params);
            algo->Execute();
        };

        RegisterBenchmark(name.str(), bm_body, threshold);
    }

    /// @brief Register a custom benchmark
    void RegisterBenchmark(std::string&& name, std::function<void()>&& body,
                           unsigned char const threshold = 10) {
        benchmarks_.emplace(std::move(name), Benchmark{std::move(body), threshold});
    }

    /// @brief Removes benchmark from execution list
    void RemoveBenchmark(std::string const& bm_name) {
        benchmarks_.erase(bm_name);
    }

    /// @brief Run all registered benchmarks
    void ExecuteAll() {
        for (auto& [name, benchmark] : benchmarks_) {
            std::cout << "** " << name << "... **\n";
            auto start = std::chrono::steady_clock::now();
            benchmark.body();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - start)
                                   .count();
            std::cout << "** " << name << ": " << elapsed / 1000 << "s **\n";
            bm_results_.emplace(name, BenchmarkResult{elapsed, benchmark.threshold});
        }
    }

    /// @brief Get results of all benchmarks that've been already executed
    std::unordered_map<std::string, BenchmarkResult> const& BenchmarkResults() const {
        return bm_results_;
    }
};
}  // namespace benchmark
