#pragma once

#include "datasets.h"
#include "fd_algorithm.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "algorithms/options/names.h"

template <typename T>
class AlgorithmTest : public LightDatasets, public HeavyDatasets, public ::testing::Test {
protected:
    std::unique_ptr<FDAlgorithm> CreateAlgorithmInstance(
        std::filesystem::path const& path, char separator = ',', bool has_header = true) {
        namespace onam = algos::config::names;

        FDAlgorithm::Config c{ .data = path, .separator = separator, .has_header = has_header };
        c.special_params[onam::kError] = 0.0;
        c.special_params[onam::kSeed] = 0;
        return std::make_unique<T>(c);
    }
};
