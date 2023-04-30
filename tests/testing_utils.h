#pragma once

#include <filesystem>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd_algorithm.h"
#include "datasets.h"
#include "util/config/error/type.h"
#include "util/config/names.h"

template <typename T>
class AlgorithmTest : public ::testing::Test {
protected:
    algos::RelationStream MakeCsvParser(std::string const& path, char separator = ',',
                                        bool has_header = true) {
        return std::make_shared<CSVParser>(path, separator, has_header);
    }

    std::unique_ptr<algos::FDAlgorithm> CreateAndConfToLoad(algos::RelationStream parser) {
        std::unique_ptr<algos::FDAlgorithm> algorithm = std::make_unique<T>();
        algos::ConfigureFromMap(*algorithm,
                                algos::StdParamsMap{{util::config::names::kData, parser}});
        return algorithm;
    }

    algos::StdParamsMap GetParamMap(const std::filesystem::path& path, char separator = ',',
                                    bool has_header = true) {
        using namespace util::config::names;
        return {
                {kData, path},
                {kSeparator, separator},
                {kHasHeader, has_header},
                {kError, util::config::ErrorType{0.0}},
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
