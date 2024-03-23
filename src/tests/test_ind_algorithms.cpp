#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "config/equal_nulls/type.h"
#include "config/names.h"
#include "config/thread_number/type.h"
#include "csv_config_util.h"
#include "test_hash_util.h"
#include "test_ind_util.h"

namespace tests {
namespace {

template <typename Algorithm>
class INDAlgorithmTest : public ::testing::Test {
    static config::ThreadNumType threads_;
    static config::EqNullsType is_null_equal_null_;

protected:
    static void SetThreadsParam(config::ThreadNumType threads) noexcept {
        assert(threads > 0);
        threads_ = threads;
    }

    static void SetEqualNulls(config::EqNullsType is_null_equal_null) noexcept {
        is_null_equal_null_ = is_null_equal_null;
    }

    static std::unique_ptr<Algorithm> CreateAlgorithmInstance(CSVConfigs const& csv_configs) {
        using namespace config::names;
        return algos::CreateAndLoadAlgorithm<Algorithm>(
                algos::StdParamsMap{{kCsvConfigs, csv_configs},
                                    {kThreads, threads_},
                                    {kEqualNulls, is_null_equal_null_}});
    }

    static void PerformConsistentHashTestOn(std::vector<CSVConfigsHash> const& configs_hashes) {
        for (auto const& [csv_configs, hash] : configs_hashes) {
            try {
                auto ind_algo = CreateAlgorithmInstance(csv_configs);
                ind_algo->Execute();
                EXPECT_EQ(HashVec(ToSortedINDTestVec(ind_algo->INDList()), HashPair), hash)
                        << "Wrong hash on datasets " << TableNamesToString(csv_configs);
            } catch (std::exception const& e) {
                std::cerr << "An exception with message: " << e.what()
                          << "\n\tis thrown on datasets " << TableNamesToString(csv_configs)
                          << '\n';
                FAIL();
            }
        }
    }

    /* hashes for light tables */
    inline static std::vector<CSVConfigsHash> const kLightConfigsHashes = {
            {{kIndTestNulls}, 170947241093786881U},
            {{kWdcAstronomical}, 1U},
            {{kWdcSymbols}, 1U},
            {{kWdcScience}, 1U},
            {{kWdcSatellites}, 1U},
            {{kWdcAppearances}, 1U},
            {{kWdcAstrology}, 13455143437649811744U},
            {{kWdcGame}, 447511263452U},
            {{kWdcKepler}, 1U},
            {{kWdcPlanetz}, 1U},
            {{kWdcAge}, 1U},
            {{kTestWide}, 7112674290840U},
            {{kAbalone}, 11213732566U},
            {{kIris}, 1U},
            {{kAdult}, 118907247627U},
            {{kBreastCancer}, 1U},
            {{kNeighbors10k}, 139579476277123U},
            {{kNeighbors100k}, 139579476277123U},
            {{kCIPublicHighway10k}, 195810426634326U},
            {{kCIPublicHighway700}, 195810426634326U}};

    /* hashes for tests with `is_null_equal_null` flag (light tables) */
    inline static std::vector<CSVConfigsHash> const kNullConfigsHashes = {
            {{kIndTestNulls}, 6131570082162402642U},
            {{kCIPublicHighway10k}, 3501995834407208U},
            {{kCIPublicHighway700}, 6532935312084701U}};

    /* hashes for heavy tables */
    inline static std::vector<CSVConfigsHash> const kHeavyConfigsHashes = {
            {{kEpicVitals}, 8662177202540121819U},
            {{kEpicMeds}, 5352642523966732252U},
            {{kIowa1kk}, 232519218595U}};
};

template <typename Algorithm>
config::ThreadNumType INDAlgorithmTest<Algorithm>::threads_ = 1;

template <typename Algorithm>
config::EqNullsType INDAlgorithmTest<Algorithm>::is_null_equal_null_ = false;

}  // namespace

TYPED_TEST_SUITE_P(INDAlgorithmTest);

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnLightTables) {
    TestFixture::SetThreadsParam(1);
    TestFixture::SetEqualNulls(false);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kLightConfigsHashes);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnHeavyTables) {
    TestFixture::SetThreadsParam(1);
    TestFixture::SetEqualNulls(false);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kHeavyConfigsHashes);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnNullTables) {
    TestFixture::SetThreadsParam(1);
    TestFixture::SetEqualNulls(true);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kNullConfigsHashes);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnLightTablesParallel) {
    TestFixture::SetThreadsParam(4);
    TestFixture::SetEqualNulls(false);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kLightConfigsHashes);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnHeavyTablesParallel) {
    TestFixture::SetThreadsParam(4);
    TestFixture::SetEqualNulls(false);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kHeavyConfigsHashes);
}

REGISTER_TYPED_TEST_SUITE_P(INDAlgorithmTest, ConsistentHashOnLightTables,
                            ConsistentHashOnHeavyTables, ConsistentHashOnNullTables,
                            ConsistentHashOnLightTablesParallel,
                            ConsistentHashOnHeavyTablesParallel);

using Algorithms = ::testing::Types<algos::Spider>;

INSTANTIATE_TYPED_TEST_SUITE_P(INDAlgorithmTest, INDAlgorithmTest, Algorithms);

}  // namespace tests
