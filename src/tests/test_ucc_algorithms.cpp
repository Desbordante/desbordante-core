#include <memory>
#include <ostream>
#include <string>

#include <boost/dynamic_bitset.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/ucc/hyucc/hyucc.h"
#include "algorithms/ucc/ucc.h"
#include "algorithms/ucc/ucc_algorithm.h"
#include "config/thread_number/type.h"
#include "datasets.h"

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

public:
    static algos::StdParamsMap GetParamMap(std::filesystem::path const& path, char separator = ',',
                                           bool has_header = true) {
        using namespace config::names;
        return {{kCsvPath, path},
                {kSeparator, separator},
                {kHasHeader, has_header},
                {kThreads, threads_}};
    }

    static std::unique_ptr<algos::UCCAlgorithm> CreateAlgorithmInstance(std::string const& filename,
                                                                        char separator = ',',
                                                                        bool has_header = true) {
        return algos::CreateAndLoadAlgorithm<AlgorithmUnderTest>(
                GetParamMap(test_data_dir / filename, separator, has_header));
    }

    static inline const std::vector<Dataset> light_datasets_ = {
        {"WDC_astronomical.csv", 2089541732445U, ',', true},
        {"WDC_symbols.csv", 1, ',', true},
        {"WDC_science.csv", 2658842082150U, ',', true},
        {"WDC_satellites.csv", 5208443370856032U, ',', true},
        {"WDC_appearances.csv", 82369238361U, ',', true},
        {"WDC_astrology.csv", 79554241843163108U, ',', true},
        {"WDC_game.csv", 2555214540772530U, ',', true},
        {"WDC_kepler.csv", 82426217315737U, ',', true},
        {"WDC_planetz.csv", 2555214540772530U, ',', true},
        {"WDC_age.csv", 2658842082150U, ',', true},
        {"TestWide.csv", 2555250373874U, ',', true},
        {"abalone.csv", 16581571148699134255U, ',', true},
        {"iris.csv", 1, ',', false},
        {"adult.csv", 1, ';', false},
        {"breast_cancer.csv", 16854900230774656828U, ',', true},
        // Possibly heavy datasets, if another less efficient algorithm than HyUCC is not
        // able to process these move them to heavy_datasets_
        {"neighbors10k.csv", 170971924188219U, ',', true},
#if 0
        {"neighbors50k.csv", 1, ',', true},
#endif
        {"neighbors100k.csv", 170971924188219U, ',', true},
        {"CIPublicHighway10k.csv", 82369238361U, ',', true},
        {"CIPublicHighway700.csv", 82369238361U, ',', true},
    };

    static inline const std::vector<Dataset> heavy_datasets_ = {
        {"EpicVitals.csv", 1, '|', true},
        {"EpicMeds.csv", 59037771758954037U, '|', true},
        {"iowa1kk.csv", 2654435863U, ',', true},
#if 0
        {"fd-reduced-30.csv", 275990379954778425U, ',', true},
        {"flight_1k.csv", 2512091017708538662U, ';', true},
        {"plista_1k.csv", 1, ';', false},
        {"letter.csv", 1, ',', false},
#endif
    };
};

template <typename AlgorithmUnderTest>
config::ThreadNumType UCCAlgorithmTest<AlgorithmUnderTest>::threads_ = 1;

// Implement custom hash functions since implementation of `std::hash` or `boost::hash` may change
// depending on the library version/architecture/os/whatever leading to tests failing.
// Taken from
// https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
std::size_t Hash(std::vector<unsigned> const& vec) {
    std::size_t seed = vec.size();
    for (auto x : vec) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

std::size_t Hash(std::vector<std::vector<unsigned>> const& vec) {
    size_t hash = 1;
    for (auto const& v : vec) {
        hash = 31 * hash + Hash(v);
    }
    return hash;
}

template <typename T>
void PerformConsistentHashTestOn(std::vector<Dataset> const& datasets) {
    for (Dataset const& dataset : datasets) {
        try {
            auto ucc_algo = T::CreateAlgorithmInstance(dataset.name, dataset.separator,
                                                       dataset.header_presence);
            ucc_algo->Execute();

            std::list<model::UCC> const& actual_list = ucc_algo->UCCList();
            std::vector<std::vector<unsigned>> actual;
            actual.reserve(actual_list.size());
            std::transform(actual_list.begin(), actual_list.end(), std::back_inserter(actual),
                           [](Vertical const& v) { return v.GetColumnIndicesAsVector(); });
            std::sort(actual.begin(), actual.end());
            EXPECT_EQ(Hash(actual), dataset.hash) << "Wrong hash on dataset " << dataset.name;
        } catch (std::exception const& e) {
            std::cout << "An exception with message: " << e.what() << "\n\tis thrown on dataset "
                      << dataset.name << '\n';
            FAIL();
        }
    }
}

}  // namespace

TYPED_TEST_SUITE_P(UCCAlgorithmTest);

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnLightDatasets) {
    TestFixture::SetThreadsParam(1);
    PerformConsistentHashTestOn<TestFixture>(TestFixture::light_datasets_);
}

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnHeavyDatasets) {
    TestFixture::SetThreadsParam(1);
    PerformConsistentHashTestOn<TestFixture>(TestFixture::heavy_datasets_);
}

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnLightDatasetsParallel) {
    TestFixture::SetThreadsParam(4);
    PerformConsistentHashTestOn<TestFixture>(TestFixture::light_datasets_);
}

TYPED_TEST_P(UCCAlgorithmTest, ConsistentHashOnHeavyDatasetsParallel) {
    TestFixture::SetThreadsParam(4);
    PerformConsistentHashTestOn<TestFixture>(TestFixture::heavy_datasets_);
}

REGISTER_TYPED_TEST_SUITE_P(UCCAlgorithmTest, ConsistentHashOnLightDatasets,
                            ConsistentHashOnHeavyDatasets, ConsistentHashOnLightDatasetsParallel,
                            ConsistentHashOnHeavyDatasetsParallel);

using Algorithms = ::testing::Types<algos::HyUCC>;
INSTANTIATE_TYPED_TEST_SUITE_P(UCCAlgorithmTest, UCCAlgorithmTest, Algorithms);

}  // namespace tests
