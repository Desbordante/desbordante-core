#pragma once

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd/fd_algorithm.h"
#include "all_csv_configs.h"
#include "config/error/type.h"
#include "config/names.h"
#include "csv_config_util.h"

namespace tests {
template <typename T>
class AlgorithmTest : public ::testing::Test {
protected:
    static std::unique_ptr<algos::FDAlgorithm> CreateAndConfToLoad(CSVConfig const& csv_config) {
        using namespace config::names;
        using algos::ConfigureFromMap, algos::StdParamsMap;

        std::unique_ptr<algos::FDAlgorithm> algorithm = std::make_unique<T>();
        ConfigureFromMap(*algorithm, StdParamsMap{{kTable, MakeInputTable(csv_config)}});
        return algorithm;
    }

    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config) {
        using namespace config::names;
        return {
                {kCsvConfig, csv_config},
                {kError, config::ErrorType{0.0}},
                {kSeed, decltype(algos::pyro::Parameters::seed){0}},
        };
    }

    static void PerformConsistentHashTestOn(std::vector<CSVConfigHash> const& config_hashes) {
        try {
            for (auto const& [csv_config, hash] : config_hashes) {
                auto algorithm = CreateAlgorithmInstance(csv_config);
                algorithm->Execute();
                EXPECT_EQ(algorithm->Fletcher16(), hash)
                        << "FD collection hash changed for " << csv_config.path.filename();
            }
        } catch (std::runtime_error& e) {
            std::cout << "Exception raised in test: " << e.what() << std::endl;
            FAIL();
        }
        SUCCEED();
    }

public:
    static std::unique_ptr<algos::FDAlgorithm> CreateAlgorithmInstance(CSVConfig const& config) {
        return algos::CreateAndLoadAlgorithm<T>(GetParamMap(config));
    }

    inline static std::vector<CSVConfigHash> const kLightDatasets = {
            {{tests::kCIPublicHighway10k, 33398},
             {tests::kNeighbors10k, 43368},
             {tests::kWdcAstronomical, 22281},
             {tests::kWdcAge, 19620},
             {tests::kWdcAppearances, 25827},
             {tests::kWdcAstrology, 40815},
             {tests::kWdcGame, 6418},
             {tests::kWdcScience, 19620},
             {tests::kWdcSymbols, 28289},
             {tests::kBreastCancer, 15121},
             {tests::kWdcKepler, 63730}}};

    inline static std::vector<CSVConfigHash> const kHeavyDatasets = {
            {{tests::kAdult, 23075},
             {tests::kCIPublicHighway, 13035},
             {tests::kEpicMeds, 50218},
             {tests::kEpicVitals, 2083},
             {tests::kIowa1kk, 28573},
             {tests::kLegacyPayors, 43612}}};
};
}  // namespace tests
