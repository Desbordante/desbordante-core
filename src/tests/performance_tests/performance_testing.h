#pragma once

#include <chrono>
#include <cmath>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/core/demangle.hpp>

#include "algorithms/algo_factory.h"
#include "config/names.h"
#include "parser/csv_parser/csv_parser.h"

namespace perf_tests {

/// @brief A singleton that controls execution of performance tests.
/// @note Most likely you'll need only these methods: @c Instance, @c RegisterSimpleTest and
/// @c RegisterTest.
class PerformanceTesting {
private:
    std::unordered_map<std::string, long long> prev_results_;
    std::unordered_map<std::string, long long> results_;
    std::vector<std::function<bool()>> tests_;

    PerformanceTesting() = default;

    /// @brief Demangle name and take only class name (without namespaces).
    std::string GetAlgoName(std::string mangled_name) {
        auto demangled_name = boost::core::demangle(mangled_name.c_str());
        auto colon_pos = demangled_name.rfind(':');
        if (colon_pos == std::string::npos) {
            return demangled_name;
        }
        return demangled_name.substr(colon_pos + 1);
    }

    /// @note Returned object lives no longer than this PerformanceTesting object.
    template <typename Body>
    std::function<bool()> WrapTest(Body&& test_body, std::string const& test_name,
                                   unsigned char threshold = 10 /* per cent */) {
        return [this, test_body{std::forward<Body>(test_body)}, test_name, threshold]() -> bool {
            std::cout << "** " << test_name << "... **\n";
            auto start = std::chrono::steady_clock::now();
            test_body();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - start)
                                   .count();
            results_.emplace(test_name, elapsed);
            std::cout << "** " << test_name << ": " << elapsed / 1000 << "s: ";
            // Assume that performance test cannot run 0ms
            if (prev_results_.contains(test_name) && prev_results_[test_name] != 0) {
                auto prev_res = prev_results_[test_name];
                auto overhead = static_cast<double>(elapsed - prev_res) / prev_res * 100;
                std::cout << (overhead <= threshold ? "SUCCESS" : "FAIL") << ": "
                          << std::setprecision(3) << std::abs(overhead) << "% "
                          << (overhead > 0 ? "slower" : "faster") << " than previous run **\n";
                return (overhead <= threshold);
            }
            std::cout << "WARNING : hasn't been ran before **\n";
            return true;
        };
    }

public:
    PerformanceTesting(PerformanceTesting&) = delete;
    PerformanceTesting(PerformanceTesting&&) = default;
    PerformanceTesting& operator=(PerformanceTesting&) = delete;
    PerformanceTesting& operator=(PerformanceTesting&&) = default;
    ~PerformanceTesting() = default;

    /// @brief Get an instance of @c PerformanceTesting.
    static PerformanceTesting& Instance() {
        static PerformanceTesting instance;
        return instance;
    }

    /// @brief Load results of previous run.
    /// @note It's unlikely that you'll need to call this method.
    void LoadPreviousResults(std::unordered_map<std::string, long long>&& prev_res) {
        prev_results_ = std::move(prev_res);
    }

    /// @brief Register a complex test to be executed at @c RunAllTests call.
    /// @tparam Test any callable, which returns @a truthy @a value when test passed,
    /// @a falsy otherwise. See implementation of @c WrapTest as an example.
    /// @note Use this method only if you really need precise control on which part of test
    /// is measured. <b>If you don't know which method to use to register your test, most likely
    /// it's @c RegisterTest(), not this one.</b>
    template <typename Test>
    void RegisterCustomTest(Test&& test) {
        tests_.push_back(std::forward<Test>(test));
    }

    /// @brief Register test to be executed at @c RunAllTests call.
    /// @tparam Body any callable.
    /// @param test_body the whole test.
    /// @param test_name a test name to be displayed. Must be unique. A good test name includes
    /// algorithm name and dataset name (or size).
    /// @param threshold execution time threshold, per cent.
    /// @note If you need more precise control on which part of test is measured, see @c
    /// RegisterCustomTest.
    template <typename Body>
    void RegisterTest(Body&& test_body, std::string const& test_name,
                      unsigned char threshold = 10) {
        RegisterCustomTest(WrapTest(std::forward<Body>(test_body), test_name, threshold));
    }

    /// @brief Run all registered tests.
    /// @note It's unlikely that you'll need to call this method.
    bool RunAllTests() {
        auto result = true;
        for (auto const& test : tests_) {
            result &= test();
        }
        return result;
    }

    /// @brief Create and register a simple single-algo test:
    /// 1. add @c csv to @c other_params;
    /// 2. create algorithm instance with these parameters;
    /// 3. call @c algorithm.Execute().
    /// @note @c other_params should not contain csv config (otherwise it will be overwritten).
    template <typename Algo>
    void RegisterSimpleTest(CSVConfig const& csv, algos::StdParamsMap&& other_params = {},
                            std::string const& name_appendix = "", unsigned char threshold = 10) {
        std::ostringstream name;
        name << GetAlgoName(typeid(Algo).name()) << ", " << csv.path.stem().string();
        if (!name_appendix.empty()) {
            name << ", " << name_appendix;
        }

        other_params[config::names::kCsvConfig] = csv;

        auto test_body = [other_params] {
            auto algo = algos::CreateAndLoadAlgorithm<Algo>(other_params);
            algo->Execute();
        };

        RegisterTest(std::move(test_body), name.str(), threshold);
    }

    /// @brief Get results of all tests that've been ran.
    /// @note It's unlikely that you'll need to call this method.
    std::unordered_map<std::string, long long> const& GetResults() const {
        return results_;
    }

    /// @brief Save single performance test result.
    /// @note This method should be used @b only with @c RegisterCustomTest
    void EmplaceResult(std::string const& test_name, long long result) {
        results_.emplace(test_name, result);
    }

    /// @brief View pervious results.
    /// @note This method should be used @b only with @c RegisterCustomTest
    std::unordered_map<std::string, long long> const& GetPrevResults() const {
        return prev_results_;
    }
};

}  // namespace perf_tests
