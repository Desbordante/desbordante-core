#pragma once

#include <filesystem>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd/fd_algorithm.h"
#include "config/error/type.h"
#include "config/names.h"
#include "datasets.h"

template <typename T>
class AlgorithmTest : public ::testing::Test {
    static config::InputTable MakeCsvParser(std::string const& path, char separator,
                                            bool has_header) {
        return std::make_shared<CSVParser>(path, separator, has_header);
    }

protected:
    static std::unique_ptr<algos::FDAlgorithm> CreateAndConfToLoad(std::string const& path,
                                                                   char separator = ',',
                                                                   bool has_header = true) {
        using config::InputTable, algos::ConfigureFromMap, algos::StdParamsMap;
        std::unique_ptr<algos::FDAlgorithm> algorithm = std::make_unique<T>();
        auto parser = MakeCsvParser(path, separator, has_header);
        ConfigureFromMap(*algorithm, StdParamsMap{{config::names::kTable, parser}});
        return algorithm;
    }

    static algos::StdParamsMap GetParamMap(const std::filesystem::path& path, char separator = ',',
                                           bool has_header = true) {
        using namespace config::names;
        return {
                {kTable, MakeCsvParser(path, separator, has_header)},
                {kError, config::ErrorType{0.0}},
                {kSeed, decltype(pyro::Parameters::seed){0}},
        };
    }

    static std::unique_ptr<algos::FDAlgorithm> CreateAlgorithmInstance(const std::string& filename,
                                                                       char separator = ',',
                                                                       bool has_header = true) {
        return algos::CreateAndLoadAlgorithm<T>(
                GetParamMap(test_data_dir / filename, separator, has_header));
    }
};
