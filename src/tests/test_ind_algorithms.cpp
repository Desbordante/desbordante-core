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
    inline static std::vector<CSVConfigsHash> const light_configs_hashes_ = {
            {{kIndTestNulls}, 170947241093786881U},
            {{kWDC_astronomical}, 1U},
            {{kWDC_symbols}, 1U},
            {{kWDC_science}, 1U},
            {{kWDC_satellites}, 1U},
            {{kWDC_appearances}, 1U},
            {{kWDC_astrology}, 13455143437649811744U},
            {{kWDC_game}, 447511263452U},
            {{kWDC_kepler}, 1U},
            {{kWDC_planetz}, 1U},
            {{kWDC_age}, 1U},
            {{kTestWide}, 7112674290840U},
            {{kabalone}, 11213732566U},
            {{kiris}, 1U},
            {{kadult}, 118907247627U},
            {{kbreast_cancer}, 1U},
            {{kneighbors10k}, 139579476277123U},
            {{kneighbors100k}, 139579476277123U},
            {{kCIPublicHighway10k}, 195810426634326U},
            {{kCIPublicHighway700}, 195810426634326U}};

    /* hashes for tests with `is_null_equal_null` flag (light tables) */
    inline static std::vector<CSVConfigsHash> const null_configs_hashes_ = {
            {{kIndTestNulls}, 6131570082162402642U},
            {{kCIPublicHighway10k}, 3501995834407208U},
            {{kCIPublicHighway700}, 6532935312084701U}};

    /* hashes for heavy tables */
    inline static std::vector<CSVConfigsHash> const heavy_configs_hashes_ = {
            {{kEpicVitals}, 8662177202540121819U},
            {{kEpicMeds}, 5352642523966732252U},
            {{kiowa1kk}, 232519218595U}};
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
    TestFixture::PerformConsistentHashTestOn(TestFixture::light_configs_hashes_);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnHeavyTables) {
    TestFixture::SetThreadsParam(1);
    TestFixture::SetEqualNulls(false);
    TestFixture::PerformConsistentHashTestOn(TestFixture::heavy_configs_hashes_);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnNullTables) {
    TestFixture::SetThreadsParam(1);
    TestFixture::SetEqualNulls(true);
    TestFixture::PerformConsistentHashTestOn(TestFixture::null_configs_hashes_);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnLightTablesParallel) {
    TestFixture::SetThreadsParam(4);
    TestFixture::SetEqualNulls(false);
    TestFixture::PerformConsistentHashTestOn(TestFixture::light_configs_hashes_);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnHeavyTablesParallel) {
    TestFixture::SetThreadsParam(4);
    TestFixture::SetEqualNulls(false);
    TestFixture::PerformConsistentHashTestOn(TestFixture::heavy_configs_hashes_);
}

REGISTER_TYPED_TEST_SUITE_P(INDAlgorithmTest, ConsistentHashOnLightTables,
                            ConsistentHashOnHeavyTables, ConsistentHashOnNullTables,
                            ConsistentHashOnLightTablesParallel,
                            ConsistentHashOnHeavyTablesParallel);

using Algorithms = ::testing::Types<algos::Spider>;

INSTANTIATE_TYPED_TEST_SUITE_P(INDAlgorithmTest, INDAlgorithmTest, Algorithms);

}  // namespace tests
