#pragma once

#include <filesystem>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd/fd_algorithm.h"
#include "all_tables_config.h"
#include "config/error/type.h"
#include "config/names.h"
#include "table_config.h"

namespace tests {
template <typename T>
class AlgorithmTest : public ::testing::Test {
protected:
    static std::unique_ptr<algos::FDAlgorithm> CreateAndConfToLoad(
            tests::TableConfig const& config) {
        using config::InputTable, algos::ConfigureFromMap, algos::StdParamsMap;
        std::unique_ptr<algos::FDAlgorithm> algorithm = std::make_unique<T>();
        ConfigureFromMap(*algorithm,
                         StdParamsMap{{config::names::kTable, config.MakeInputTable()}});
        return algorithm;
    }

    static algos::StdParamsMap GetParamMap(tests::TableConfig const& config) {
        using namespace config::names;
        return {
                {kTable, config.MakeInputTable()},
                {kError, config::ErrorType{0.0}},
                {kSeed, decltype(pyro::Parameters::seed){0}},
        };
    }

    static void PerformConsistentHashTestOn(std::vector<TableConfigHash> const& datasets) {
        try {
            for (auto const& [config, hash] : datasets) {
                auto algorithm = CreateAlgorithmInstance(config);
                algorithm->Execute();
                EXPECT_EQ(algorithm->Fletcher16(), hash)
                        << "FD collection hash changed for " << config.name;
            }
        } catch (std::runtime_error& e) {
            std::cout << "Exception raised in test: " << e.what() << std::endl;
            FAIL();
        }
        SUCCEED();
    }

public:
    static std::unique_ptr<algos::FDAlgorithm> CreateAlgorithmInstance(
            tests::TableConfig const& config) {
        return algos::CreateAndLoadAlgorithm<T>(GetParamMap(config));
    }

    inline static std::vector<tests::TableConfigHash> const light_datasets_ = {
            {{tests::kCIPublicHighway10k, 33398},
             {tests::kneighbors10k, 43368},
             {tests::kWDC_astronomical, 22281},
             {tests::kWDC_age, 19620},
             {tests::kWDC_appearances, 25827},
             {tests::kWDC_astrology, 40815},
             {tests::kWDC_game, 6418},
             {tests::kWDC_science, 19620},
             {tests::kWDC_symbols, 28289},
             {tests::kbreast_cancer, 15121},
             {tests::kWDC_kepler, 63730}}};

    inline static std::vector<tests::TableConfigHash> const heavy_datasets_ = {
            {{tests::kadult, 23075},
             {tests::kCIPublicHighway, 13035},
             {tests::kEpicMeds, 50218},
             {tests::kEpicVitals, 2083},
             {tests::kiowa1kk, 28573},
             {tests::kLegacyPayors, 43612}}};
};
}  // namespace tests
