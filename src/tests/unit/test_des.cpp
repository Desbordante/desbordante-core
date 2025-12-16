#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/nar/des/des.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

class DESTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, double minsup,
                                           double minconf, unsigned int popSize,
                                           unsigned int evalNum, double crossProb, double diffScale,
                                           algos::des::DifferentialStrategy diffStrategy) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},          {kArMinimumSupport, minsup},
                {kArMinimumConfidence, minconf},   {kPopulationSize, popSize},
                {kMaxFitnessEvaluations, evalNum}, {kCrossoverProbability, crossProb},
                {kDifferentialScale, diffScale},   {kDifferentialStrategy, diffStrategy}};
    }

    template <typename... Args>
    static std::unique_ptr<algos::des::DES> CreateAlgorithmInstance(Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::des::DES>(
                GetParamMap(std::forward<Args>(args)...));
    }

    static std::vector<std::string> ExtractFitnessValues(
            std::vector<algos::des::NAR> const& nar_vector) {
        std::vector<std::string> fitness_values;
        fitness_values.reserve(nar_vector.size());
        for (auto const& nar : nar_vector) {
            fitness_values.push_back(std::to_string(nar.GetQualities().fitness));
        }
        return fitness_values;
    }
};

TEST_F(DESTest, LaunchTest1) {
    auto algorithm = CreateAlgorithmInstance(kAbalone, 0.0, 0.0, 100u, 100u, 0.9, 0.5,
                                             algos::des::DifferentialStrategy::rand1Bin);
    algorithm->Execute();
    auto result = ExtractFitnessValues(algorithm->GetNARVector());
    std::vector<std::string> expected = {"0.609963", "0.518598", "0.514174", "0.497616", "0.483237",
                                         "0.444604", "0.418879", "0.341235", "0.321052", "0.310425",
                                         "0.299355", "0.296755", "0.296456", "0.259754", "0.235430",
                                         "0.213631", "0.192706", "0.186377", "0.168266", "0.164281",
                                         "0.098144", "0.085561", "0.076805", "0.076043"};
    ASSERT_EQ(result, expected);
}

TEST_F(DESTest, LaunchTest2) {
    auto algorithm = CreateAlgorithmInstance(kAbalone, 0.2, 0.6, 200u, 300u, 0.9, 0.5,
                                             algos::des::DifferentialStrategy::rand1Bin);
    algorithm->Execute();
    auto result = ExtractFitnessValues(algorithm->GetNARVector());
    std::vector<std::string> expected = {"0.598929", "0.587854", "0.566134", "0.524618",
                                         "0.522095", "0.489909", "0.376434"};
    ASSERT_EQ(result, expected);
}

}  // namespace tests
