#include <memory>
#include <ostream>
#include <string>

#include <boost/dynamic_bitset.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/ucc/hyucc/hyucc.h"
#include "core/algorithms/ucc/ucc.h"
#include "core/algorithms/ucc/ucc_algorithm.h"
#include "core/config/names.h"
#include "core/config/thread_number/type.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"
#include "tests/unit/test_hash_util.h"

std::ostream& operator<<(std::ostream& os, Vertical const& v) {
    os << v.ToString();
    return os;
}

namespace tests {

namespace {

// TODO(polyntsov): think how we should organize test code, maybe implement some classes with
// basic functionality common for all primitive mining algorithms. Without it every test class
// for specific algorithm is similar to any other (compare this class to AlgorithmTest and
// ARAlgorithmTest for example).
template <typename AlgorithmUnderTest>
class UCCAlgorithmTest : public ::testing::Test {
    static config::ThreadNumType threads_;

protected:
    static void SetThreadsParam(config::ThreadNumType threads) noexcept {
        assert(threads > 0);
        threads_ = threads;
    }

    void PerformConsistentHashTestOn(std::vector<CSVConfigHash> const& config_hashes) {
        for (auto const& [csv_config, hash] : config_hashes) {
            try {
                auto ucc_algo = CreateAlgorithmInstance(csv_config);
                ucc_algo->Execute();

                std::list<model::UCC> const& actual_list = ucc_algo->UCCList();
                std::vector<std::vector<unsigned>> actual;
                actual.reserve(actual_list.size());
                std::transform(actual_list.begin(), actual_list.end(), std::back_inserter(actual),
                               [](Vertical const& v) { return v.GetColumnIndicesAsVector(); });
                std::sort(actual.begin(), actual.end());
                EXPECT_EQ(Hash(actual), hash)
                        << "Wrong hash on dataset " << csv_config.path.filename();
            } catch (std::exception const& e) {
                std::cout << "An exception with message: " << e.what()
                          << "\n\tis thrown on dataset " << csv_config.path.filename() << '\n';
                FAIL();
            }
        }
    }

public:
    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config) {
        using namespace config::names;
        // Here we return StdParamsMap with option kThreads but some algorithms does not need this
        // option (PyroUCC for example). This does not generate errors, because when creating the
        // algorithm with function CreateAndLoadAlgorithm only the options necessary for the
        // algorithm will be used
        return {{kCsvConfig, csv_config}, {kThreads, threads_}};
    }

    static std::unique_ptr<algos::UCCAlgorithm> CreateAlgorithmInstance(
            CSVConfig const& csv_config) {
        return algos::CreateAndLoadAlgorithm<AlgorithmUnderTest>(GetParamMap(csv_config));
    }

    inline static std::vector<CSVConfigHash> const kLightDatasets = {
        {kWdcAstronomical, 2089541732445U},
        {kWdcSymbols, 1},
        {kWdcScience, 2658842082150U},
        {kWdcSatellites, 5208443370856032U},
        {kWdcAppearances, 82369238361U},
        {kWdcAstrology, 79554241843163108U},
        {kWdcGame, 2555214540772530U},
        {kWdcKepler, 82426217315737U},
        {kWdcPlanetz, 2555214540772530U},
        {kWdcAge, 2658842082150U},
        {kTestWide, 2555250373874U},
        {kAbalone, 16581571148699134255U},
        {kIris, 1},
        {kAdult, 1},
        {kBreastCancer, 16854900230774656828U},
        // Possibly heavy datasets, if another less efficient algorithm than HyUCC is not
        // able to process these move them to kHeavyDatasets
        {kNeighbors10k, 170971924188219U},
#if 0
        {kNeighbors50k, 1},
#endif
        {kNeighbors100k, 170971924188219U},
        {kCIPublicHighway10k, 82369238361U},
        {kCIPublicHighway700, 82369238361U},
    };

    inline static std::vector<CSVConfigHash> const kHeavyDatasets = {
        {kEpicVitals, 1},
        {kEpicMeds, 59037771758954037U},
        {kIowa1kk, 2654435863U},
#if 0
        {kFdReduced30, 275990379954778425U},
        {kFlight1k, 2512091017708538662U},
        {kPlista1k, 1},
        {kLetter, 1},
#endif
    };
};

template <typename AlgorithmUnderTest>
config::ThreadNumType UCCAlgorithmTest<AlgorithmUnderTest>::threads_ = 1;

}  // namespace

TYPED_TEST_SUITE_P(UCCAlgorithmTest);

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnLightDatasets) {
    TestFixture::SetThreadsParam(1);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kLightDatasets);
}

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnHeavyDatasets) {
    TestFixture::SetThreadsParam(1);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kHeavyDatasets);
}

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnLightDatasetsParallel) {
    TestFixture::SetThreadsParam(4);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kLightDatasets);
}

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnHeavyDatasetsParallel) {
    TestFixture::SetThreadsParam(4);
    TestFixture::PerformConsistentHashTestOn(TestFixture::kHeavyDatasets);
}

REGISTER_TYPED_TEST_SUITE_P(UCCAlgorithmTest, ConsistentHashOnLightDatasets,
                            ConsistentHashOnHeavyDatasets, ConsistentHashOnLightDatasetsParallel,
                            ConsistentHashOnHeavyDatasetsParallel);

using Algorithms = ::testing::Types<algos::HyUCC, algos::PyroUCC, algos::HPIValid>;
INSTANTIATE_TYPED_TEST_SUITE_P(UCCAlgorithmTest, UCCAlgorithmTest, Algorithms);

}  // namespace tests
