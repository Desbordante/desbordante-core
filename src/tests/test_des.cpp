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
};

TEST_F(DESTest, LaunchTest) {
    auto algorithm = CreateAlgorithmInstance(kAbalone, 0.5, 0.6, 100u, 100u, 0.9, 0.5,
                                             algos::des::DifferentialStrategy::rand1Bin);
    algorithm->Execute();
    std::vector<std::string> result;
    for (auto i : algorithm->GetNARVector()) {
        result.push_back(std::to_string(i.GetQualities().fitness));
    }
    std::vector<std::string> expected = {
            "0.614583", "0.592672", "0.556569", "0.555795", "0.493177", "0.468389", "0.436493",
            "0.408549", "0.393278", "0.330923", "0.245074", "0.240821", "0.211013", "0.202699",
            "0.187660", "0.185542", "0.182276", "0.149640", "0.148624", "0.113370", "0.093495"};
    ASSERT_EQ(result, expected);
}

}  // namespace tests
