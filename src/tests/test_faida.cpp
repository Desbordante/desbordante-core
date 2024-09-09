#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/ind/faida/faida.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "test_ind_util.h"

namespace tests {
namespace {
// Since Faida is an approximate algorithm, its results may differ from all the other IND discovery
// algorithms, which are exact. Of course, on these small examples the results of both exact and
// approximate algos must be the same, but in the future the set of test datasets will grow and
// large tests may be added. Therefore the decision was made to move Faida tests to a separate
// class.
class FaidaINDAlgorithmTest : public ::testing::Test {
public:
    struct Config {
        int sample_size;
        double hll_accuracy;
        unsigned short num_threads;
    };

protected:
    static std::unique_ptr<algos::INDAlgorithm> CreateFaidaInstance(CSVConfigs const& csv_configs,
                                                                    Config const& config) {
        using namespace config::names;
        return algos::CreateAndLoadAlgorithm<algos::Mind>(algos::StdParamsMap{
                {kCsvConfigs, csv_configs},
                {kSampleSize, config.sample_size},
                {kHllAccuracy, config.hll_accuracy},
                {kThreads, config.num_threads},
        });
    }
};

using FaidaTestConfig = FaidaINDAlgorithmTest::Config;

static FaidaTestConfig parallel_test_config{
        .sample_size = 500,
        .hll_accuracy = 0.001,
        .num_threads = 4,
};
}  // namespace

TEST_F(FaidaINDAlgorithmTest, EqualityTest) {
    for (auto& [csv_configs, expected_inds] : kINDEqualityTestConfigs) {
        CheckINDsListsEqualityTest(CreateFaidaInstance(csv_configs, parallel_test_config),
                                   expected_inds);
    }
}

TEST_F(FaidaINDAlgorithmTest, TestTwoTables) {
    INDTestSet expected_inds_subset{{{0, {0, 1, 2, 3}}, {1, {0, 1, 3, 4}}},
                                    {{1, {0, 1, 3, 4}}, {0, {0, 1, 2, 3}}}};
    CheckINDsResultContainsINDsTest(
            CreateFaidaInstance({kIndTestTableFirst, kIndTestTableSecond}, parallel_test_config),
            expected_inds_subset, 47);
}

}  // namespace tests
