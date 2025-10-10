#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/od/fastod/fastod.h"
#include "algorithms/od/fastod/hashing/hashing.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "csv_config_util.h"

namespace tests {

namespace {

size_t RunFastod(algos::StdParamsMap const& params) {
    std::unique_ptr<algos::Fastod> fastod = algos::CreateAndLoadAlgorithm<algos::Fastod>(params);

    fastod->Execute();

    std::vector<algos::fastod::AscCanonicalOD> ods_asc_sorted = fastod->GetAscendingDependencies();
    std::vector<algos::fastod::DescCanonicalOD> ods_desc_sorted =
            fastod->GetDescendingDependencies();
    std::vector<algos::fastod::SimpleCanonicalOD> ods_simple_sorted =
            fastod->GetSimpleDependencies();

    std::sort(ods_asc_sorted.begin(), ods_asc_sorted.end());
    std::sort(ods_desc_sorted.begin(), ods_desc_sorted.end());
    std::sort(ods_simple_sorted.begin(), ods_simple_sorted.end());

    size_t ods_asc_sorted_hash = algos::fastod::hashing::CombineHashes(ods_asc_sorted);
    size_t ods_desc_sorted_hash = algos::fastod::hashing::CombineHashes(ods_desc_sorted);
    size_t ods_simple_sorted_hash = algos::fastod::hashing::CombineHashes(ods_simple_sorted);

    std::vector<size_t> od_hashes = {ods_asc_sorted_hash, ods_desc_sorted_hash,
                                     ods_simple_sorted_hash};
    size_t result_hash = algos::fastod::hashing::CombineHashes(od_hashes);

    return result_hash;
}

class ExactFastodResultHashTest : public ::testing::TestWithParam<CSVConfigHash> {};

class ApproximateFastodResultHashTest : public ::testing::TestWithParam<CSVConfigHash> {};

}  // namespace

TEST_P(ExactFastodResultHashTest, CorrectnessTest) {
    using namespace config::names;
    CSVConfigHash csv_config_hash = GetParam();
    algos::StdParamsMap params{{kCsvConfig, csv_config_hash.config}};
    size_t actual_hash = RunFastod(params);
    EXPECT_EQ(actual_hash, csv_config_hash.hash);
}

TEST_P(ApproximateFastodResultHashTest, CorrectnessTest) {
    using namespace config::names;
    CSVConfigHash csv_config_hash = GetParam();
    algos::StdParamsMap params{{kCsvConfig, csv_config_hash.config}, {kError, 0.1}};
    size_t actual_hash = RunFastod(params);
    EXPECT_EQ(actual_hash, csv_config_hash.hash);
}

INSTANTIATE_TEST_SUITE_P(
        TestFastodSuite, ExactFastodResultHashTest,
        ::testing::Values(CSVConfigHash{kOdTestNormOd, 8741296102670149192ULL},
                          CSVConfigHash{kOdTestNormSmall2x3, 14827049072319306073ULL},
                          CSVConfigHash{kOdTestNormSmall3x3, 66466490561337ULL},
                          CSVConfigHash{kOdTestNormAbalone, 14398696798633970055ULL},
                          CSVConfigHash{kOdTestNormBalanceScale, 11093822414574ULL},
                          CSVConfigHash{kOdTestNormBreastCancerWisconsin, 4334402279000540119ULL},
                          CSVConfigHash{kOdTestNormEchocardiogram, 2243402441338221665ULL},
                          CSVConfigHash{kOdTestNormHorse10c, 1462534374501425106ULL},
                          CSVConfigHash{kOdTestNormIris, 11093822414574ULL},
                          CSVConfigHash{kBernoulliRelation, 6518269127574092257ULL},
                          CSVConfigHash{kTestFD, 15333753345229147120ULL},
                          CSVConfigHash{kWdcAstrology, 723643032648123806ULL},
                          CSVConfigHash{kWdcGame, 3164616462792843131ULL},
                          CSVConfigHash{kWdcPlanetz, 11920309231858256338ULL},
                          CSVConfigHash{kWdcSymbols, 2211268401046792ULL},
                          CSVConfigHash{kNeighbors10k, 11706974185824900569ULL},
                          CSVConfigHash{kNeighbors100k, 11706974185824900569ULL},
                          CSVConfigHash{kAbalone, 13440043079221534278ULL},
                          CSVConfigHash{kIris, 386492228314919716ULL},
                          CSVConfigHash{kBreastCancer, 10457518087798149718ULL}));

INSTANTIATE_TEST_SUITE_P(
        TestFastodSuite, ApproximateFastodResultHashTest,
        ::testing::Values(CSVConfigHash{kOdTestNormOd, 8741296102670149192ULL},
                          CSVConfigHash{kOdTestNormSmall2x3, 14827049072319306073ULL},
                          CSVConfigHash{kOdTestNormSmall3x3, 66466490561337ULL},
                          CSVConfigHash{kOdTestNormAbalone, 13281507877791866837ULL},
                          CSVConfigHash{kOdTestNormBalanceScale, 11093822414574ULL},
                          CSVConfigHash{kOdTestNormBreastCancerWisconsin, 1988797490908697206ULL},
                          CSVConfigHash{kOdTestNormEchocardiogram, 2257561609024862593ULL},
                          CSVConfigHash{kOdTestNormHorse10c, 3742587070580858968ULL},
                          CSVConfigHash{kOdTestNormIris, 10544375522104677703ULL},
                          CSVConfigHash{kBernoulliRelation, 6518269127574092257ULL},
                          CSVConfigHash{kTestFD, 6669867546111180267ULL},
                          CSVConfigHash{kWdcAstrology, 1164174148302232767ULL},
                          CSVConfigHash{kWdcGame, 3164616462792843131ULL},
                          CSVConfigHash{kWdcPlanetz, 17122866665397837554ULL},
                          CSVConfigHash{kWdcSymbols, 6010811148591257019ULL},
                          CSVConfigHash{kAbalone, 10493577215165807658ULL},
                          CSVConfigHash{kIris, 3611430059132203437ULL},
                          CSVConfigHash{kBreastCancer, 17733420313134925921ULL}));

}  // namespace tests
