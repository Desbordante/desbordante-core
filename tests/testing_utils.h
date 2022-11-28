#pragma once

#include "datasets.h"
#include "fd_algorithm.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "algorithms/algo_factory.h"
#include "algorithms/options/names.h"

template <typename T>
class AlgorithmTest : public LightDatasets, public HeavyDatasets, public ::testing::Test {
protected:
    CSVParser MakeCsvParser(std::string const& path, char separator = ',',
                            bool has_header = true) {
        return {path, separator, has_header};
    }

    std::unique_ptr<algos::FDAlgorithm> CreateAndConfToFit() {
        namespace onam = algos::config::names;
        std::unique_ptr<algos::FDAlgorithm> prim = std::make_unique<T>();
        algos::ConfigureFromMap(*prim, algos::StdParamsMap{});
        return prim;
    }

    std::unique_ptr<algos::FDAlgorithm> CreateAlgorithmInstance(
            std::string const& path, char separator = ',', bool has_header = true) {
        namespace onam = algos::config::names;
        algos::StdParamsMap option_map = {
                {onam::kData, path},
                {onam::kSeparator, separator},
                {onam::kHasHeader, has_header},
                {onam::kError, algos::config::ErrorType{0.0}},
                {onam::kSeed, decltype(Configuration::seed){0}},
        };
        return algos::CreateAndLoadPrimitive<T>(option_map);
    }
};
