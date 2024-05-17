#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "config/equal_nulls/type.h"
#include "config/max_arity/type.h"
#include "config/names.h"
#include "config/thread_number/type.h"
#include "csv_config_util.h"
#include "max_arity/type.h"
#include "test_hash_util.h"
#include "test_ind_util.h"

namespace tests {
namespace {

struct INDTestConfig {
    config::ThreadNumType threads;
    config::EqNullsType is_null_equal_null;
    config::MaxArityType max_arity;
};

template <typename Algorithm>
class INDAlgorithmTest : public ::testing::Test {
public:
    using Config = INDTestConfig;

protected:
    static std::unique_ptr<Algorithm> CreateAlgorithmInstance(CSVConfigs const& csv_configs,
                                                              Config const& test_config) {
        using namespace config::names;
        return algos::CreateAndLoadAlgorithm<Algorithm>(algos::StdParamsMap{
                {kCsvConfigs, csv_configs},
                {kThreads, test_config.threads},
                {kEqualNulls, test_config.is_null_equal_null},
                {kMaximumArity, test_config.max_arity},
        });
    }

    static void PerformConsistentHashTestOn(std::vector<CSVConfigsHash> const& configs_hashes,
                                            Config const& test_config) {
        for (auto const& [csv_configs, hash] : configs_hashes) {
            try {
                auto ind_algo = CreateAlgorithmInstance(csv_configs, test_config);
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

    static void PerformEqualityTestOn(
            std::vector<INDEqualityTestConfig> const& equality_test_configs,
            Config const& test_config) {
        for (auto const& [csv_configs, expected_inds] : equality_test_configs) {
            CheckINDsListsEqualityTest(CreateAlgorithmInstance(csv_configs, test_config),
                                       expected_inds);
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

static INDTestConfig const kUnaryTestConfigNotEqualNull{
        .threads = 1,
        .is_null_equal_null = false,
        .max_arity = 1,
};

static INDTestConfig const kUnaryTestConfigEqualNull{
        .threads = 1,
        .is_null_equal_null = true,
        .max_arity = 1,
};

static INDTestConfig const kUnaryTestConfigNotEqualNullParallel{
        .threads = 4,
        .is_null_equal_null = false,
        .max_arity = 1,
};

}  // namespace

TYPED_TEST_SUITE_P(INDAlgorithmTest);

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnLightTables) {
    TestFixture::PerformConsistentHashTestOn(TestFixture::kLightConfigsHashes,
                                             kUnaryTestConfigNotEqualNull);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnHeavyTables) {
    TestFixture::PerformConsistentHashTestOn(TestFixture::kHeavyConfigsHashes,
                                             kUnaryTestConfigNotEqualNull);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnNullTables) {
    TestFixture::PerformConsistentHashTestOn(TestFixture::kNullConfigsHashes,
                                             kUnaryTestConfigEqualNull);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnLightTablesParallel) {
    TestFixture::PerformConsistentHashTestOn(TestFixture::kLightConfigsHashes,
                                             kUnaryTestConfigNotEqualNullParallel);
}

TYPED_TEST_P(INDAlgorithmTest, ConsistentHashOnHeavyTablesParallel) {
    TestFixture::PerformConsistentHashTestOn(TestFixture::kHeavyConfigsHashes,
                                             kUnaryTestConfigNotEqualNullParallel);
}

REGISTER_TYPED_TEST_SUITE_P(INDAlgorithmTest, ConsistentHashOnLightTables,
                            ConsistentHashOnHeavyTables, ConsistentHashOnNullTables,
                            ConsistentHashOnLightTablesParallel,
                            ConsistentHashOnHeavyTablesParallel);

/* We don't add Mind here, because Mind uses Spider to mine unary dependencies. */
using UnaryTestAlgorithms = ::testing::Types<algos::Spider>;

INSTANTIATE_TYPED_TEST_SUITE_P(INDAlgorithmTest, INDAlgorithmTest, UnaryTestAlgorithms);

namespace {

template <typename Algorithm>
class GeneralINDAlgorithmTest : public ::testing::Test {
protected:
    static std::unique_ptr<Algorithm> CreateAlgorithmInstance(CSVConfigs const& csv_configs) {
        using namespace config::names;
        return algos::CreateAndLoadAlgorithm<Algorithm>(algos::StdParamsMap{
                {kCsvConfigs, csv_configs},
        });
    }
};

}  // namespace

/* General tests for all IND algorithms */
using GeneralTestAlgorithms = ::testing::Types<algos::Spider, algos::Mind, algos::Faida>;

TYPED_TEST_SUITE(GeneralINDAlgorithmTest, GeneralTestAlgorithms);

TYPED_TEST(GeneralINDAlgorithmTest, TestEmptyTable) {
    ASSERT_THROW(TestFixture::CreateAlgorithmInstance({kIndTestEmpty}), std::runtime_error);
}

TYPED_TEST(GeneralINDAlgorithmTest, TestEmptyTablesCollection) {
    ASSERT_THROW(TestFixture::CreateAlgorithmInstance({}), config::ConfigurationError);
}

namespace {

template <typename Algorithm>
class NaryINDAlgorithmTest : public ::testing::Test {
protected:
    static std::unique_ptr<Algorithm> CreateAlgorithmInstance(CSVConfigs const& csv_configs) {
        using namespace config::names;
        return algos::CreateAndLoadAlgorithm<Algorithm>(algos::StdParamsMap{
                {kCsvConfigs, csv_configs},
        });
    }
};

}  // namespace

/* We don't add Faida, since Faida is an approximate algorithm. */
using NaryTestAlgorithms = ::testing::Types<algos::Mind>;

TYPED_TEST_SUITE(NaryINDAlgorithmTest, NaryTestAlgorithms);

TYPED_TEST(NaryINDAlgorithmTest, EqualityTest) {
    for (auto& [csv_configs, expected_inds] : kINDEqualityTestConfigs) {
        CheckINDsListsEqualityTest(TestFixture::CreateAlgorithmInstance(csv_configs),
                                   expected_inds);
    }
}

}  // namespace tests
