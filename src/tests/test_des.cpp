#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/nar/des/des.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {

class DESTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, double minsup,
                                           double minconf, unsigned int popSize,
                                           unsigned int evalNum, double crossProb, double diffScale,
                                           algos::des::DifferentialStrategy diffStrategy) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},          {kMinimumSupport, minsup},
                {kMinimumConfidence, minconf},     {kPopulationSize, popSize},
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
    std::vector<std::string> expected = {"0.634851", "0.566887", "0.549872", "0.520035",
                                         "0.518598", "0.481561", "0.460124", "0.407567",
                                         "0.333972", "0.313191", "0.274753", "0.190558",
                                         "0.187335", "0.161869", "0.113770", "0.111297"};
    ASSERT_EQ(result, expected);
}

TEST_F(DESTest, LaunchTest2) {
    auto algorithm = CreateAlgorithmInstance(kAbalone, 0.2, 0.6, 200u, 300u, 0.9, 0.5,
                                             algos::des::DifferentialStrategy::rand1Bin);
    algorithm->Execute();
    auto result = ExtractFitnessValues(algorithm->GetNARVector());
    std::vector<std::string> expected = {"0.735697", "0.622020", "0.606939", "0.564184"};
    ASSERT_EQ(result, expected);
}

}  // namespace tests
