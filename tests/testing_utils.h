#pragma once

#include <filesystem>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd_algorithm.h"
#include "algorithms/options/error/type.h"
#include "algorithms/options/names.h"
#include "datasets.h"

template <typename T>
class AlgorithmTest : public ::testing::Test {
protected:
    CSVParser MakeCsvParser(std::string const& path, char separator = ',',
                            bool has_header = true) {
        return {path, separator, has_header};
    }

    std::unique_ptr<algos::FDAlgorithm> CreateAndConfToFit() {
        std::unique_ptr<algos::FDAlgorithm> algorithm = std::make_unique<T>();
        algos::ConfigureFromMap(*algorithm, algos::StdParamsMap{});
        return algorithm;
    }

    algos::StdParamsMap GetParamMap(const std::filesystem::path& path, char separator = ',',
                                    bool has_header = true) {
        using namespace algos::config::names;
        return {
                {kData, path},
                {kSeparator, separator},
                {kHasHeader, has_header},
                {kError, algos::config::ErrorType{0.0}},
                {kSeed, decltype(Configuration::seed){0}},
        };
    }

    std::unique_ptr<algos::FDAlgorithm> CreateAlgorithmInstance(const std::string& filename,
                                                                char separator = ',',
                                                                bool has_header = true) {
        return algos::CreateAndLoadAlgorithm<T>(
                GetParamMap(test_data_dir / filename, separator, has_header));
    }
};
