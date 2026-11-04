#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/sd/sd_miner/sd_miner.h"
#include "core/algorithms/sd/sd_verifier/sd_verifier.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

namespace {

constexpr char kMiningCsv[] =
        "X,Y\n"
        "1,1\n"
        "2,2\n"
        "3,3\n"
        "4,4\n"
        "5,5\n"
        "6,6\n"
        "7,7\n"
        "8,8\n"
        "9,9\n"
        "10,10\n"
        "11,0\n"
        "12,1\n"
        "13,2\n"
        "14,3\n"
        "15,4\n"
        "16,100\n"
        "17,5\n"
        "18,6\n"
        "19,7\n"
        "20,8\n";

constexpr char kSingleRowCsv[] =
        "X,Y\n"
        "42,100\n";

constexpr char kExactGapCsv[] =
        "X,Y\n"
        "1,0\n"
        "2,10\n"
        "3,20\n"
        "4,30\n";

constexpr char kFiniteGapWithInsertionCsv[] =
        "X,Y\n"
        "1,0\n"
        "2,5\n"
        "3,15\n"
        "4,20\n";

constexpr char kPaperFiniteGapCsv[] =
        "X,Y\n"
        "1,5\n"
        "2,9\n"
        "3,12\n"
        "4,25\n"
        "5,31\n"
        "6,30\n"
        "7,34\n"
        "8,40\n";

constexpr char kUnsortedCsv[] =
        "X,Y\n"
        "5,5\n"
        "1,1\n"
        "3,3\n"
        "2,2\n"
        "4,4\n";

constexpr char kDuplicateLhsCsv[] =
        "X,Y\n"
        "1,10\n"
        "1,5\n"
        "2,6\n"
        "3,7\n";

constexpr char kSplitDuplicateLhsCsv[] =
        "X,Y\n"
        "2,101\n"
        "1,100\n"
        "1,0\n";

constexpr char kFloatingBoundaryCsv[] =
        "X,Y\n"
        "1,0.9\n"
        "2,2.9\n";

constexpr char kNearEqualExactGapCsv[] =
        "X,Y\n"
        "1,1\n"
        "2,0\n"
        "3,1e-15\n";

constexpr char kHugeFiniteGapCsv[] =
        "X,Y\n"
        "1,0\n"
        "2,1e308\n";

constexpr char kNonFiniteLhsCsv[] =
        "X,Y\n"
        "nan,0\n"
        "2,1\n";

constexpr char kNonFiniteRhsCsv[] =
        "X,Y\n"
        "1,0\n"
        "2,inf\n";

constexpr char kMissingColumnCsv[] =
        "X,Y\n"
        "1,1\n"
        "2\n"
        "3,2\n";

constexpr char kFiniteGapBlocksCsv[] =
        "X,Y\n"
        "1,0\n"
        "2,1\n"
        "3,2\n"
        "4,10\n"
        "5,11\n"
        "6,12\n"
        "7,13\n"
        "8,30\n"
        "9,31\n"
        "10,32\n"
        "11,33\n"
        "12,34\n";

constexpr char kSingletonCandidatesCsv[] =
        "X,Y\n"
        "1,0\n"
        "2,100\n"
        "3,0\n"
        "4,100\n"
        "5,0\n";

constexpr char kOverlappingIntervalsCsv[] =
        "X,Y\n"
        "1,3\n"
        "2,4\n"
        "3,2\n"
        "4,2\n"
        "5,5\n"
        "6,1\n";

constexpr char kRedundantGreedyCsv[] =
        "X,Y\n"
        "1,10\n"
        "2,20\n"
        "3,25\n"
        "4,35\n"
        "5,45\n"
        "6,55\n"
        "7,65\n"
        "8,95\n";

constexpr char kIrreducibleGreedyCsv[] =
        "X,Y\n"
        "1,10\n"
        "2,20\n"
        "3,25\n"
        "4,35\n"
        "5,45\n"
        "6,60\n"
        "7,90\n"
        "8,105\n";

constexpr char kEmptyCsv[] = "X,Y\n";

constexpr char kNonNumericCsv[] =
        "X,Y\n"
        "1,0\n"
        "2,not-a-number\n";

struct ScopedTestFile {
    std::string filename;

    ScopedTestFile(std::string name, std::string const& content) : filename(std::move(name)) {
        std::ofstream f(filename);
        f << content;
    }

    ~ScopedTestFile() {
        std::remove(filename.c_str());
    }
};

algos::StdParamsMap BaseParams(std::string const& filename) {
    return {{config::names::kCsvConfig, CSVConfig{filename.c_str(), ',', true}},
            {config::names::kLhsIndices, config::IndicesType{0}},
            {config::names::kRhsIndices, config::IndicesType{1}},
            {config::names::kSdG1, 0.0},
            {config::names::kSdG2, -1.0},
            {config::names::kSdMinimumConfidence, 0.9},
            {config::names::kSdMinimumSupport, 1.0}};
}

struct MinerOptions {
    std::optional<algos::sd_miner::IntervalStrategy> interval_strategy = std::nullopt;
    std::optional<algos::sd_miner::AssemblyStrategy> assembly_strategy = std::nullopt;
    std::optional<double> min_confidence = std::nullopt;
    std::optional<double> min_support = std::nullopt;
    std::optional<double> g1 = std::nullopt;
    std::optional<double> g2 = std::nullopt;
    std::optional<double> delta = std::nullopt;
};

std::unique_ptr<algos::sd_miner::SDMiner> RunMiner(std::string const& filename,
                                                   MinerOptions const& options = {}) {
    auto params = BaseParams(filename);
    if (options.interval_strategy.has_value()) {
        params[config::names::kSdIntervalStrategy] = *options.interval_strategy;
    }
    if (options.assembly_strategy.has_value()) {
        params[config::names::kSdAssemblyStrategy] = *options.assembly_strategy;
    }
    if (options.min_confidence.has_value()) {
        params[config::names::kSdMinimumConfidence] = *options.min_confidence;
    }
    if (options.min_support.has_value()) {
        params[config::names::kSdMinimumSupport] = *options.min_support;
    }
    if (options.g1.has_value()) {
        params[config::names::kSdG1] = *options.g1;
    }
    if (options.g2.has_value()) {
        params[config::names::kSdG2] = *options.g2;
    }
    if (options.delta.has_value()) {
        params[config::names::kSdDelta] = *options.delta;
    }

    auto miner = algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params);
    miner->Execute();
    return miner;
}

void ExpectApproximateCoversExact(std::string const& filename, double min_confidence, double g1,
                                  double g2, double delta) {
    auto exact_miner =
            RunMiner(filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                .min_confidence = min_confidence,
                                .min_support = 0.0,
                                .g1 = g1,
                                .g2 = g2});
    auto approx_miner = RunMiner(
            filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kApproximate,
                       .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy,
                       .min_confidence = min_confidence,
                       .min_support = 0.0,
                       .g1 = g1,
                       .g2 = g2,
                       .delta = delta});

    auto const& exact_candidates = exact_miner->GetCandidates();
    auto const& approx_candidates = approx_miner->GetCandidates();
    ASSERT_FALSE(exact_candidates.empty());
    ASSERT_FALSE(approx_candidates.empty());

    double const relaxed_threshold = min_confidence * (1.0 - delta) / (1.0 + delta);
    for (auto const& exact_candidate : exact_candidates) {
        bool found_cover = false;
        for (auto const& approx_candidate : approx_candidates) {
            if (approx_candidate.left <= exact_candidate.left &&
                exact_candidate.right <= approx_candidate.right &&
                approx_candidate.confidence >= relaxed_threshold) {
                found_cover = true;
                break;
            }
        }

        EXPECT_TRUE(found_cover) << "No approximate cover for exact interval ["
                                 << exact_candidate.left << ", " << exact_candidate.right << "]";
    }
}

std::string MakeBinarySequenceCsv(size_t length, size_t bit_mask) {
    std::ostringstream csv;
    csv << "X,Y\n";
    for (size_t i = 0; i < length; ++i) {
        csv << i + 1 << ',' << ((bit_mask >> i) & 1U) << '\n';
    }
    return csv.str();
}

size_t ComputeCandidateUnionSupport(
        std::vector<algos::sd_miner::SDCandidateInterval> const& candidates, size_t subset,
        size_t data_size) {
    std::vector<bool> covered(data_size, false);
    for (size_t candidate_idx = 0; candidate_idx < candidates.size(); ++candidate_idx) {
        if ((subset & (size_t{1} << candidate_idx)) == 0) {
            continue;
        }
        for (size_t pos = candidates[candidate_idx].left; pos <= candidates[candidate_idx].right;
             ++pos) {
            covered[pos] = true;
        }
    }
    return static_cast<size_t>(std::count(covered.begin(), covered.end(), true));
}

size_t BruteForceMinimumTableauSize(
        std::vector<algos::sd_miner::SDCandidateInterval> const& candidates, size_t target_support,
        size_t data_size, size_t max_subset_size = std::numeric_limits<size_t>::max()) {
    if (candidates.size() >= std::numeric_limits<size_t>::digits) {
        ADD_FAILURE() << "Test oracle cannot encode the candidate subset.";
        return std::numeric_limits<size_t>::max();
    }
    auto covers_target = [&](auto const& self, size_t next, size_t remaining,
                             size_t subset) -> bool {
        if (remaining == 0) {
            return ComputeCandidateUnionSupport(candidates, subset, data_size) >= target_support;
        }
        for (size_t candidate_idx = next; candidate_idx + remaining <= candidates.size();
             ++candidate_idx) {
            if (self(self, candidate_idx + 1, remaining - 1,
                     subset | (size_t{1} << candidate_idx))) {
                return true;
            }
        }
        return false;
    };

    size_t const limit = std::min(candidates.size(), max_subset_size);
    for (size_t selected_count = 0; selected_count <= limit; ++selected_count) {
        if (covers_target(covers_target, 0, selected_count, 0)) {
            return selected_count;
        }
    }
    return std::numeric_limits<size_t>::max();
}

void ExpectTableauUsesCandidates(
        std::vector<algos::sd_miner::SDTableauPattern> const& tableau,
        std::vector<algos::sd_miner::SDCandidateInterval> const& candidates) {
    for (auto const& pattern : tableau) {
        EXPECT_TRUE(std::any_of(candidates.begin(), candidates.end(), [&](auto const& candidate) {
            return pattern.left_position == candidate.left &&
                   pattern.right_position == candidate.right;
        }));
    }
}

void ExpectTableauQuality(algos::sd_miner::SDMiner const& miner, size_t data_size,
                          size_t target_support, double min_confidence) {
    auto const& tableau = miner.GetTableau();
    ExpectTableauUsesCandidates(tableau, miner.GetCandidates());

    std::vector<bool> covered(data_size, false);
    for (auto const& pattern : tableau) {
        ASSERT_LE(pattern.left_position, pattern.right_position);
        ASSERT_LT(pattern.right_position, data_size);
        EXPECT_EQ(pattern.support, pattern.right_position - pattern.left_position + 1);
        EXPECT_GE(pattern.confidence, min_confidence);
        for (size_t pos = pattern.left_position; pos <= pattern.right_position; ++pos) {
            covered[pos] = true;
        }
    }

    size_t const union_support =
            static_cast<size_t>(std::count(covered.begin(), covered.end(), true));
    EXPECT_EQ(miner.GetGlobalSupport(), union_support);
    EXPECT_GE(union_support, target_support);
}

void ExpectNoRedundantPatterns(std::vector<algos::sd_miner::SDTableauPattern> const& tableau,
                               size_t data_size) {
    std::vector<size_t> coverage_counts(data_size, 0);
    for (auto const& pattern : tableau) {
        for (size_t pos = pattern.left_position; pos <= pattern.right_position; ++pos) {
            ++coverage_counts[pos];
        }
    }

    for (auto const& pattern : tableau) {
        EXPECT_TRUE(std::any_of(coverage_counts.begin() + pattern.left_position,
                                coverage_counts.begin() + pattern.right_position + 1,
                                [](size_t count) { return count == 1; }));
    }
}

}  // namespace

TEST(SDMinerTest, ExactIntervalsAndExactAssemblyBuildMinimalTableau) {
    ScopedTestFile file("sd_miner_exact_test.csv", kMiningCsv);
    auto params = BaseParams(file.filename);
    params[config::names::kSdIntervalStrategy] = algos::sd_miner::IntervalStrategy::kExact;
    params[config::names::kSdAssemblyStrategy] = algos::sd_miner::AssemblyStrategy::kExact;

    auto miner = algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params);
    miner->Execute();

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 2);
    EXPECT_EQ(miner->GetGlobalSupport(), 20);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 11.0);
    EXPECT_DOUBLE_EQ(tableau[1].left_x, 11.0);
    EXPECT_DOUBLE_EQ(tableau[1].right_x, 20.0);

    for (auto const& candidate : miner->GetCandidates()) {
        EXPECT_GE(candidate.confidence, 0.9);
    }
}

TEST(SDMinerTest, ApproximateIntervalsAndGreedyAssemblyReachSupport) {
    ScopedTestFile file("sd_miner_approx_test.csv", kMiningCsv);
    auto params = BaseParams(file.filename);
    params[config::names::kSdMinimumSupport] = 0.8;
    params[config::names::kSdDelta] = 0.1;
    params[config::names::kSdIntervalStrategy] = algos::sd_miner::IntervalStrategy::kApproximate;
    params[config::names::kSdAssemblyStrategy] = algos::sd_miner::AssemblyStrategy::kGreedy;

    auto miner = algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params);
    miner->Execute();

    EXPECT_FALSE(miner->GetCandidates().empty());
    EXPECT_FALSE(miner->GetTableau().empty());
    EXPECT_GE(miner->GetGlobalSupport(), 16);
}

TEST(SDMinerTest, EmptyDatasetProducesEmptyTableau) {
    ScopedTestFile file("sd_miner_empty_test.csv", kEmptyCsv);

    auto miner = RunMiner(file.filename);

    EXPECT_TRUE(miner->GetCandidates().empty());
    EXPECT_TRUE(miner->GetTableau().empty());
    EXPECT_EQ(miner->GetGlobalSupport(), 0);
}

TEST(SDMinerTest, SingleRowDatasetProducesSinglePattern) {
    ScopedTestFile file("sd_miner_single_row_test.csv", kSingleRowCsv);

    auto miner = RunMiner(file.filename);

    auto const& candidates = miner->GetCandidates();
    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(candidates.size(), 1);
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_EQ(candidates[0].left, 0);
    EXPECT_EQ(candidates[0].right, 0);
    EXPECT_DOUBLE_EQ(candidates[0].confidence, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 42.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 42.0);
    EXPECT_EQ(tableau[0].support, 1);
    EXPECT_EQ(miner->GetGlobalSupport(), 1);
}

TEST(SDMinerTest, SortsRowsByAntecedentBeforeMining) {
    ScopedTestFile file("sd_miner_unsorted_test.csv", kUnsortedCsv);

    auto miner = RunMiner(file.filename);

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 5.0);
    EXPECT_EQ(tableau[0].support, 5);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 1.0);
}

TEST(SDMinerTest, DuplicateAntecedentValuesUseVerifierCompatibleOrdering) {
    ScopedTestFile file("sd_miner_duplicate_lhs_test.csv", kDuplicateLhsCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0,
                                     .min_support = 1.0});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 2);
    EXPECT_EQ(miner->GetGlobalSupport(), 4);
    EXPECT_EQ(tableau[0].left_position, 0);
    EXPECT_EQ(tableau[0].right_position, 1);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 1.0);
    EXPECT_EQ(tableau[1].left_position, 2);
    EXPECT_EQ(tableau[1].right_position, 3);
    EXPECT_DOUBLE_EQ(tableau[1].left_x, 2.0);
    EXPECT_DOUBLE_EQ(tableau[1].right_x, 3.0);
}

TEST(SDMinerTest, DuplicateAntecedentPositionIntervalMatchesVerifierSubset) {
    ScopedTestFile file("sd_miner_split_duplicate_lhs_test.csv", kSplitDuplicateLhsCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0,
                                     .min_support = 2.0 / 3.0,
                                     .g1 = 1.0,
                                     .g2 = 1.0});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_EQ(tableau[0].left_position, 1);
    EXPECT_EQ(tableau[0].right_position, 2);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 2.0);
    EXPECT_EQ(tableau[0].support, 2);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 1.0);
    EXPECT_EQ(miner->GetGlobalSupport(), 2);

    auto const& sorted_row_indices = miner->GetSortedRowIndices();
    EXPECT_EQ(sorted_row_indices, (std::vector<size_t>{2, 1, 0}));
    config::IndicesType const verifier_indices{
            static_cast<config::IndexType>(sorted_row_indices[tableau[0].left_position]),
            static_cast<config::IndexType>(sorted_row_indices[tableau[0].right_position])};
    EXPECT_EQ(verifier_indices, (config::IndicesType{1, 0}));

    algos::StdParamsMap verifier_params{
            {config::names::kCsvConfig, CSVConfig{file.filename.c_str(), ',', true}},
            {config::names::kLhsIndices, config::IndicesType{0}},
            {config::names::kRhsIndices, config::IndicesType{1}},
            {config::names::kSdG1, 1.0},
            {config::names::kSdG2, 1.0}};
    auto verifier = algos::CreateAndLoadAlgorithm<algos::sd_verifier::SDVerifier>(verifier_params);
    verifier->SetOption(config::names::kSdIndices, verifier_indices);
    verifier->Execute();

    EXPECT_DOUBLE_EQ(verifier->GetConfidence(), tableau[0].confidence);
}

TEST(SDMinerTest, FloatingPointBoundaryAtMaximumGapHasNoOperations) {
    ScopedTestFile file("sd_miner_floating_boundary_test.csv", kFloatingBoundaryCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0,
                                     .min_support = 1.0,
                                     .g1 = 1.0,
                                     .g2 = 2.0});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_EQ(tableau[0].left_position, 0);
    EXPECT_EQ(tableau[0].right_position, 1);
    EXPECT_EQ(tableau[0].support, 2);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 1.0);
}

TEST(SDMinerTest, ExactGapDoesNotTreatNearEqualValuesAsOneStep) {
    ScopedTestFile file("sd_miner_near_equal_exact_gap_test.csv", kNearEqualExactGapCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0 / 3.0,
                                     .min_support = 1.0,
                                     .g1 = 0.1,
                                     .g2 = 0.1});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_EQ(tableau[0].left_position, 0);
    EXPECT_EQ(tableau[0].right_position, 2);
    EXPECT_EQ(tableau[0].support, 3);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 1.0 / 3.0);
}

TEST(SDMinerTest, HugeFiniteGapRequiresDeletionWithoutOverflow) {
    ScopedTestFile file("sd_miner_huge_finite_gap_test.csv", kHugeFiniteGapCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 0.5,
                                     .min_support = 1.0,
                                     .g1 = 0.0,
                                     .g2 = 1e-10});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_EQ(tableau[0].left_position, 0);
    EXPECT_EQ(tableau[0].right_position, 1);
    EXPECT_EQ(tableau[0].support, 2);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 0.5);
}

TEST(SDMinerTest, MissingSelectedColumnsAreSkippedLikeVerifier) {
    ScopedTestFile file("sd_miner_missing_column_test.csv", kMissingColumnCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0,
                                     .min_support = 1.0});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_EQ(tableau[0].support, 2);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 3.0);
    EXPECT_EQ(miner->GetGlobalSupport(), 2);
}

TEST(SDMinerTest, ZeroMinimumSupportProducesEmptyTableau) {
    ScopedTestFile file("sd_miner_zero_support_test.csv", kExactGapCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0,
                                     .min_support = 0.0,
                                     .g1 = 10.0,
                                     .g2 = 10.0});

    EXPECT_FALSE(miner->GetCandidates().empty());
    EXPECT_TRUE(miner->GetTableau().empty());
    EXPECT_EQ(miner->GetGlobalSupport(), 0);
}

TEST(SDMinerTest, SupportThresholdUsesTheSmallestSufficientRowCount) {
    std::string csv = "X,Y\n";
    for (size_t i = 0; i < 100; ++i) {
        csv += std::to_string(i) + ",0\n";
    }
    ScopedTestFile file("sd_miner_support_rounding_test.csv", csv);
    std::vector<std::pair<double, size_t>> const thresholds{
            {0.0, 0},
            {std::numeric_limits<double>::denorm_min(), 1},
            {std::nextafter(0.07, 0.0), 7},
            {0.07, 7},
            {std::nextafter(0.07, 1.0), 8},
            {0.079, 8},
            {std::nextafter(0.14, 0.0), 14},
            {0.14, 14},
            {std::nextafter(0.14, 1.0), 15},
            {1.0, 100}};
    for (auto strategy :
         {algos::sd_miner::AssemblyStrategy::kExact, algos::sd_miner::AssemblyStrategy::kGreedy}) {
        for (auto const& [threshold, expected] : thresholds) {
            SCOPED_TRACE(::testing::Message() << "support=" << threshold);
            auto miner = RunMiner(file.filename,
                                  {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                   .assembly_strategy = strategy,
                                   .min_confidence = 1.0,
                                   .min_support = threshold,
                                   .g1 = 1.0,
                                   .g2 = 1.0});
            EXPECT_EQ(miner->GetGlobalSupport(), expected);
            EXPECT_EQ(miner->GetTableau().size(), expected);
        }
    }
}

TEST(SDMinerTest, ExactFiniteGapDiscoversWholeSequence) {
    ScopedTestFile file("sd_miner_exact_gap_test.csv", kExactGapCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0,
                                     .min_support = 1.0,
                                     .g1 = 10.0,
                                     .g2 = 10.0});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 4.0);
    EXPECT_EQ(tableau[0].support, 4);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 1.0);
}

TEST(SDMinerTest, FiniteGapAllowsInsertionAccordingToConfidenceThreshold) {
    ScopedTestFile file("sd_miner_finite_gap_insertion_test.csv", kFiniteGapWithInsertionCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 0.75,
                                     .min_support = 1.0,
                                     .g1 = 4.0,
                                     .g2 = 5.0});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 4.0);
    EXPECT_EQ(tableau[0].support, 4);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 0.75);
}

TEST(SDMinerTest, ComputesPaperFiniteGapConfidenceExample) {
    ScopedTestFile file("sd_miner_paper_finite_gap_test.csv", kPaperFiniteGapCsv);

    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 0.5,
                                     .min_support = 1.0,
                                     .g1 = 4.0,
                                     .g2 = 6.0});

    auto const& tableau = miner->GetTableau();
    ASSERT_EQ(tableau.size(), 1);
    EXPECT_DOUBLE_EQ(tableau[0].left_x, 1.0);
    EXPECT_DOUBLE_EQ(tableau[0].right_x, 8.0);
    EXPECT_EQ(tableau[0].support, 8);
    EXPECT_DOUBLE_EQ(tableau[0].confidence, 0.5);
    EXPECT_EQ(miner->GetGlobalSupport(), 8);
}

TEST(SDMinerTest, GreedyAssemblyCoversSameSupportOnSimpleCase) {
    ScopedTestFile file("sd_miner_greedy_test.csv", kMiningCsv);

    auto miner = RunMiner(file.filename,
                          {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                           .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy});

    EXPECT_EQ(miner->GetGlobalSupport(), 20);
    EXPECT_LE(miner->GetTableau().size(), 3);
}

TEST(SDMinerTest, GreedyAssemblyRemovesPatternsCoveredByOtherSelectedPatterns) {
    ScopedTestFile file("sd_miner_redundant_greedy_test.csv", kRedundantGreedyCsv);

    auto exact =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 0.75,
                                     .min_support = 1.0,
                                     .g1 = 9.0,
                                     .g2 = 11.0});
    auto greedy = RunMiner(file.filename,
                           {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                            .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy,
                            .min_confidence = 0.75,
                            .min_support = 1.0,
                            .g1 = 9.0,
                            .g2 = 11.0});

    ASSERT_EQ(exact->GetTableau().size(), 2U);
    ASSERT_EQ(greedy->GetTableau().size(), exact->GetTableau().size());
    EXPECT_EQ(greedy->GetGlobalSupport(), 8U);
    ExpectTableauQuality(*greedy, 8, 8, 0.75);
    ExpectNoRedundantPatterns(greedy->GetTableau(), 8);
}

TEST(SDMinerTest, GreedyAssemblyKeepsPatternsWithExclusiveCoverage) {
    ScopedTestFile file("sd_miner_irreducible_greedy_test.csv", kIrreducibleGreedyCsv);

    auto exact =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 0.75,
                                     .min_support = 0.8,
                                     .g1 = 9.0,
                                     .g2 = 11.0});
    auto greedy = RunMiner(file.filename,
                           {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                            .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy,
                            .min_confidence = 0.75,
                            .min_support = 0.8,
                            .g1 = 9.0,
                            .g2 = 11.0});

    ASSERT_EQ(exact->GetTableau().size(), 3U);
    ASSERT_EQ(greedy->GetTableau().size(), 4U);
    EXPECT_EQ(greedy->GetGlobalSupport(), 7U);
    ExpectTableauQuality(*greedy, 8, 7, 0.75);
    ExpectNoRedundantPatterns(greedy->GetTableau(), 8);
}

TEST(SDMinerTest, ExactGenerationKeepsEarlierBlocksWhenLastCandidateReachesEnd) {
    ScopedTestFile file("sd_miner_tail_candidate_test.csv", kFiniteGapBlocksCsv);
    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 1.0,
                                     .min_support = 0.0,
                                     .g1 = 1.0,
                                     .g2 = 1.0});

    auto const& candidates = miner->GetCandidates();
    ASSERT_EQ(candidates.size(), 3U);
    std::array<std::pair<size_t, size_t>, 3> const expected{{{0, 2}, {3, 6}, {7, 11}}};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(candidates[i].left, expected[i].first);
        EXPECT_EQ(candidates[i].right, expected[i].second);
        EXPECT_DOUBLE_EQ(candidates[i].confidence, 1.0);
    }
    EXPECT_TRUE(miner->GetTableau().empty());
}

TEST(SDMinerTest, ExactGenerationDoesNotAssumeMonotonePrefixConfidence) {
    ScopedTestFile file("sd_miner_nonmonotone_confidence_test.csv",
                        "X,Y\n0,0\n1,1\n2,100\n3,2\n4,3\n");
    auto miner =
            RunMiner(file.filename, {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                     .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                     .min_confidence = 0.8,
                                     .min_support = 1.0});

    auto const& candidates = miner->GetCandidates();
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates[0].left, 0U);
    EXPECT_EQ(candidates[0].right, 4U);
    EXPECT_DOUBLE_EQ(candidates[0].confidence, 0.8);
}

TEST(SDMinerTest, ExactAssemblyChoosesFirstSufficientCandidate) {
    ScopedTestFile file("sd_miner_single_interval_choice_test.csv", kFiniteGapBlocksCsv);
    std::array<double, 3> const supports{0.1, 0.3, 0.4};
    std::array<std::pair<size_t, size_t>, 3> const expected{{{0, 2}, {3, 6}, {7, 11}}};
    for (size_t i = 0; i < supports.size(); ++i) {
        SCOPED_TRACE(supports[i]);
        auto miner = RunMiner(file.filename,
                              {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                               .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                               .min_confidence = 1.0,
                               .min_support = supports[i],
                               .g1 = 1.0,
                               .g2 = 1.0});
        ASSERT_EQ(miner->GetTableau().size(), 1U);
        EXPECT_EQ(miner->GetTableau()[0].left_position, expected[i].first);
        EXPECT_EQ(miner->GetTableau()[0].right_position, expected[i].second);
    }
}

TEST(SDMinerTest, ExactAssemblyMatchesBruteForceForFiniteGapCandidates) {
    std::array<std::pair<double, double>, 3> const gaps{{{0.0, 2.0}, {1.0, 2.0}, {1.0, 1.0}}};
    for (size_t bits = 0; bits < 32; ++bits) {
        ScopedTestFile file("sd_miner_finite_assembly_oracle_test.csv",
                            MakeBinarySequenceCsv(5, bits));
        for (auto const& [g1, g2] : gaps) {
            for (double confidence : {0.5, 0.75, 1.0}) {
                SCOPED_TRACE(::testing::Message() << "bits=" << bits << ", g1=" << g1 << ", g2="
                                                  << g2 << ", confidence=" << confidence);
                auto miner =
                        RunMiner(file.filename,
                                 {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                  .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                  .min_confidence = confidence,
                                  .min_support = 0.8,
                                  .g1 = g1,
                                  .g2 = g2});
                EXPECT_EQ(miner->GetTableau().size(),
                          BruteForceMinimumTableauSize(miner->GetCandidates(), 4, 5));
                ExpectTableauQuality(*miner, 5, 4, confidence);
            }
        }
    }
}

TEST(SDMinerTest, ExactAssemblyMatchesBruteForceOnExhaustiveSmallInputs) {
    struct Thresholds {
        double confidence;
        double support;
    };

    std::vector<Thresholds> const thresholds{{0.5, 0.5}, {0.75, 0.75}, {1.0, 1.0}};

    for (size_t length = 1; length <= 5; ++length) {
        for (size_t bit_mask = 0; bit_mask < (size_t{1} << length); ++bit_mask) {
            ScopedTestFile file("sd_miner_assembly_oracle_test.csv",
                                MakeBinarySequenceCsv(length, bit_mask));
            for (auto const [min_confidence, min_support] : thresholds) {
                SCOPED_TRACE(::testing::Message()
                             << "length=" << length << ", bits=" << bit_mask
                             << ", confidence=" << min_confidence << ", support=" << min_support);

                auto exact =
                        RunMiner(file.filename,
                                 {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                  .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                  .min_confidence = min_confidence,
                                  .min_support = min_support});
                size_t const target_support = static_cast<size_t>(std::ceil(min_support * length));
                size_t const optimum = BruteForceMinimumTableauSize(exact->GetCandidates(),
                                                                    target_support, length);

                ASSERT_NE(optimum, std::numeric_limits<size_t>::max());
                EXPECT_EQ(exact->GetTableau().size(), optimum);
                EXPECT_GE(exact->GetGlobalSupport(), target_support);
                ExpectTableauUsesCandidates(exact->GetTableau(), exact->GetCandidates());

                auto greedy =
                        RunMiner(file.filename,
                                 {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                  .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy,
                                  .min_confidence = min_confidence,
                                  .min_support = min_support});
                EXPECT_GE(greedy->GetGlobalSupport(), target_support);
                EXPECT_GE(greedy->GetTableau().size(), optimum);
                EXPECT_LE(greedy->GetTableau().size(), 9 * optimum);
                ExpectTableauUsesCandidates(greedy->GetTableau(), greedy->GetCandidates());
                ExpectNoRedundantPatterns(greedy->GetTableau(), length);
            }
        }
    }
}

TEST(SDMinerTest, ApproximateAssemblySatisfiesPaperQualityBoundsOnExhaustiveSmallInputs) {
    struct Thresholds {
        double confidence;
        double support;
    };

    std::vector<Thresholds> const thresholds{{0.5, 0.5}, {0.75, 0.75}, {1.0, 1.0}};
    std::vector<double> const deltas{0.1, 0.25, 0.5};

    for (size_t length = 1; length <= 5; ++length) {
        for (size_t bit_mask = 0; bit_mask < (size_t{1} << length); ++bit_mask) {
            ScopedTestFile file("sd_miner_approx_quality_oracle_test.csv",
                                MakeBinarySequenceCsv(length, bit_mask));
            for (auto const [min_confidence, min_support] : thresholds) {
                size_t const target_support = static_cast<size_t>(std::ceil(min_support * length));
                auto exact =
                        RunMiner(file.filename,
                                 {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                  .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                  .min_confidence = min_confidence,
                                  .min_support = min_support});
                size_t const optimum = BruteForceMinimumTableauSize(exact->GetCandidates(),
                                                                    target_support, length);

                ASSERT_NE(optimum, std::numeric_limits<size_t>::max());
                ASSERT_GT(optimum, 0U);

                for (double delta : deltas) {
                    SCOPED_TRACE(::testing::Message()
                                 << "length=" << length << ", bits=" << bit_mask
                                 << ", confidence=" << min_confidence << ", support=" << min_support
                                 << ", delta=" << delta);
                    double const relaxed_threshold = min_confidence * (1.0 - delta) / (1.0 + delta);

                    auto approximate_exact = RunMiner(
                            file.filename,
                            {.interval_strategy = algos::sd_miner::IntervalStrategy::kApproximate,
                             .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                             .min_confidence = min_confidence,
                             .min_support = min_support,
                             .g1 = 0.0,
                             .g2 = -1.0,
                             .delta = delta});
                    EXPECT_GE(approximate_exact->GetGlobalSupport(), target_support);
                    EXPECT_LE(approximate_exact->GetTableau().size(), optimum);
                    for (auto const& pattern : approximate_exact->GetTableau()) {
                        EXPECT_GE(pattern.confidence, relaxed_threshold);
                    }

                    auto approximate_greedy = RunMiner(
                            file.filename,
                            {.interval_strategy = algos::sd_miner::IntervalStrategy::kApproximate,
                             .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy,
                             .min_confidence = min_confidence,
                             .min_support = min_support,
                             .g1 = 0.0,
                             .g2 = -1.0,
                             .delta = delta});
                    EXPECT_GE(approximate_greedy->GetGlobalSupport(), target_support);
                    EXPECT_LE(approximate_greedy->GetTableau().size(), 9 * optimum);
                    for (auto const& pattern : approximate_greedy->GetTableau()) {
                        EXPECT_GE(pattern.confidence, relaxed_threshold);
                    }
                    ExpectNoRedundantPatterns(approximate_greedy->GetTableau(), length);
                }
            }
        }
    }
}

TEST(SDMinerTest, StrategyQualityBoundsWithManyCandidates) {
    size_t constexpr data_size = 64;
    double constexpr min_confidence = 0.75;
    double constexpr min_support = 0.125;
    double constexpr delta = 0.1;
    size_t constexpr target_support = 8;
    size_t constexpr exact_optimum = 2;

    std::ostringstream csv;
    csv << "X,Y\n";
    for (size_t i = 0; i < data_size; ++i) {
        csv << i + 1 << ',' << i % 2 << '\n';
    }
    ScopedTestFile file("sd_miner_many_candidates_quality_test.csv", csv.str());

    for (auto interval_strategy : {algos::sd_miner::IntervalStrategy::kExact,
                                   algos::sd_miner::IntervalStrategy::kApproximate}) {
        bool const exact_intervals = interval_strategy == algos::sd_miner::IntervalStrategy::kExact;
        size_t const expected_optimum = exact_intervals ? exact_optimum : 1;
        double const confidence_threshold =
                exact_intervals ? min_confidence : min_confidence * (1.0 - delta) / (1.0 + delta);
        for (auto assembly_strategy : {algos::sd_miner::AssemblyStrategy::kExact,
                                       algos::sd_miner::AssemblyStrategy::kGreedy}) {
            SCOPED_TRACE(::testing::Message()
                         << "interval_strategy=" << (exact_intervals ? "exact" : "approximate")
                         << ", assembly_strategy="
                         << (assembly_strategy == algos::sd_miner::AssemblyStrategy::kExact
                                     ? "exact"
                                     : "greedy"));
            auto miner = RunMiner(file.filename, {.interval_strategy = interval_strategy,
                                                  .assembly_strategy = assembly_strategy,
                                                  .min_confidence = min_confidence,
                                                  .min_support = min_support,
                                                  .g1 = 0.0,
                                                  .g2 = -1.0,
                                                  .delta = delta});
            auto const& candidates = miner->GetCandidates();
            ASSERT_EQ(candidates.size(), exact_intervals ? 31U : 29U);
            size_t const optimum =
                    BruteForceMinimumTableauSize(candidates, target_support, data_size, 2);
            ASSERT_EQ(optimum, expected_optimum);

            ASSERT_GT(candidates.size(), 9 * exact_optimum);
            EXPECT_LE(miner->GetTableau().size(), 9 * exact_optimum);
            EXPECT_EQ(miner->GetTableau().size(), optimum);
            ExpectTableauQuality(*miner, data_size, target_support, confidence_threshold);
        }
    }
}

TEST(SDMinerTest, StrategyQualityBoundsWithOverlappingIntervals) {
    ScopedTestFile file("sd_miner_overlapping_quality_test.csv", kOverlappingIntervalsCsv);
    size_t constexpr data_size = 6;
    double constexpr min_confidence = 0.75;
    double constexpr delta = 0.1;

    for (auto interval_strategy : {algos::sd_miner::IntervalStrategy::kExact,
                                   algos::sd_miner::IntervalStrategy::kApproximate}) {
        bool const exact_intervals = interval_strategy == algos::sd_miner::IntervalStrategy::kExact;
        double const confidence_threshold =
                exact_intervals ? min_confidence : min_confidence * (1.0 - delta) / (1.0 + delta);
        for (auto assembly_strategy : {algos::sd_miner::AssemblyStrategy::kExact,
                                       algos::sd_miner::AssemblyStrategy::kGreedy}) {
            bool const exact_assembly =
                    assembly_strategy == algos::sd_miner::AssemblyStrategy::kExact;
            SCOPED_TRACE(::testing::Message()
                         << "interval_strategy=" << (exact_intervals ? "exact" : "approximate")
                         << ", assembly_strategy=" << (exact_assembly ? "exact" : "greedy"));
            auto miner = RunMiner(file.filename, {.interval_strategy = interval_strategy,
                                                  .assembly_strategy = assembly_strategy,
                                                  .min_confidence = min_confidence,
                                                  .min_support = 1.0,
                                                  .g1 = 0.0,
                                                  .g2 = 3.0,
                                                  .delta = delta});
            ASSERT_EQ(miner->GetCandidates().size(), 3U);
            size_t const optimum =
                    BruteForceMinimumTableauSize(miner->GetCandidates(), data_size, data_size, 2);
            ASSERT_EQ(optimum, 2U);
            if (exact_assembly) {
                EXPECT_EQ(miner->GetTableau().size(), optimum);
            } else {
                EXPECT_GE(miner->GetTableau().size(), optimum);
                EXPECT_LE(miner->GetTableau().size(), 3U);
            }
            ExpectTableauQuality(*miner, data_size, data_size, confidence_threshold);
        }
    }
}

TEST(SDMinerTest, ApproximateCandidateConfidenceIsLowerBoundedByRelaxedThreshold) {
    ScopedTestFile file("sd_miner_relaxed_threshold_test.csv", kMiningCsv);
    auto params = BaseParams(file.filename);
    params[config::names::kSdMinimumConfidence] = 0.9;
    params[config::names::kSdMinimumSupport] = 0.5;
    params[config::names::kSdDelta] = 0.1;
    params[config::names::kSdIntervalStrategy] = algos::sd_miner::IntervalStrategy::kApproximate;
    params[config::names::kSdAssemblyStrategy] = algos::sd_miner::AssemblyStrategy::kGreedy;

    auto miner = algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params);
    miner->Execute();

    double const relaxed_threshold = 0.9 * (1.0 - 0.1) / (1.0 + 0.1);
    for (auto const& candidate : miner->GetCandidates()) {
        EXPECT_GE(candidate.confidence, relaxed_threshold);
    }
}

TEST(SDMinerTest, ApproximateCandidateMayUseRelaxedConfidenceThreshold) {
    ScopedTestFile file("sd_miner_relaxed_threshold_candidate_test.csv", kMiningCsv);
    double constexpr min_confidence = 0.9;
    double constexpr delta = 0.5;
    double const relaxed_threshold = min_confidence * (1.0 - delta) / (1.0 + delta);

    auto miner = RunMiner(file.filename,
                          {.interval_strategy = algos::sd_miner::IntervalStrategy::kApproximate,
                           .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy,
                           .min_confidence = min_confidence,
                           .min_support = 0.0,
                           .g1 = 0.0,
                           .g2 = -1.0,
                           .delta = delta});

    bool found_relaxed_only_candidate = false;
    for (auto const& candidate : miner->GetCandidates()) {
        EXPECT_GE(candidate.confidence, relaxed_threshold);
        if (candidate.confidence < min_confidence) {
            found_relaxed_only_candidate = true;
        }
    }

    EXPECT_TRUE(found_relaxed_only_candidate);
}

TEST(SDMinerTest, ApproximateCandidatesCoverExactCandidates) {
    ScopedTestFile file("sd_miner_approx_cover_test.csv", kMiningCsv);

    ExpectApproximateCoversExact(file.filename, 0.9, 0.0, -1.0, 0.1);
}

TEST(SDMinerTest, ApproximateCandidatesCoverExactCandidatesForFiniteGaps) {
    ScopedTestFile exact_gap_file("sd_miner_approx_cover_exact_gap_test.csv", kExactGapCsv);
    ScopedTestFile insertion_file("sd_miner_approx_cover_insertion_test.csv",
                                  kFiniteGapWithInsertionCsv);
    ScopedTestFile paper_file("sd_miner_approx_cover_paper_gap_test.csv", kPaperFiniteGapCsv);
    ScopedTestFile blocks_file("sd_miner_approx_cover_blocks_test.csv", kFiniteGapBlocksCsv);

    ExpectApproximateCoversExact(exact_gap_file.filename, 1.0, 10.0, 10.0, 0.1);
    ExpectApproximateCoversExact(insertion_file.filename, 0.75, 4.0, 5.0, 0.1);
    ExpectApproximateCoversExact(paper_file.filename, 0.5, 4.0, 6.0, 0.1);
    ExpectApproximateCoversExact(blocks_file.filename, 0.75, 0.0, 2.0, 0.1);
}

TEST(SDMinerTest, ApproximateCandidatesCoverSingletonExactCandidates) {
    ScopedTestFile file("sd_miner_approx_cover_singletons_test.csv", kSingletonCandidatesCsv);

    ExpectApproximateCoversExact(file.filename, 1.0, 10.0, 10.0, 0.3);
}

TEST(SDMinerTest, ApproximateGeometryCoversExactIntervalsOnExhaustiveSmallInputs) {
    std::vector<double> const confidence_thresholds{0.5, 0.75, 1.0};
    std::vector<double> const deltas{0.1, 0.25, 0.5};

    for (size_t length = 1; length <= 5; ++length) {
        for (size_t bit_mask = 0; bit_mask < (size_t{1} << length); ++bit_mask) {
            ScopedTestFile file("sd_miner_geometry_oracle_test.csv",
                                MakeBinarySequenceCsv(length, bit_mask));
            for (double min_confidence : confidence_thresholds) {
                auto exact =
                        RunMiner(file.filename,
                                 {.interval_strategy = algos::sd_miner::IntervalStrategy::kExact,
                                  .assembly_strategy = algos::sd_miner::AssemblyStrategy::kExact,
                                  .min_confidence = min_confidence,
                                  .min_support = 0.0});
                ASSERT_FALSE(exact->GetCandidates().empty());

                for (double delta : deltas) {
                    SCOPED_TRACE(::testing::Message()
                                 << "length=" << length << ", bits=" << bit_mask
                                 << ", confidence=" << min_confidence << ", delta=" << delta);
                    auto approximate = RunMiner(
                            file.filename,
                            {.interval_strategy = algos::sd_miner::IntervalStrategy::kApproximate,
                             .assembly_strategy = algos::sd_miner::AssemblyStrategy::kGreedy,
                             .min_confidence = min_confidence,
                             .min_support = 0.0,
                             .g1 = 0.0,
                             .g2 = -1.0,
                             .delta = delta});
                    double const relaxed_threshold = min_confidence * (1.0 - delta) / (1.0 + delta);

                    for (auto const& exact_candidate : exact->GetCandidates()) {
                        EXPECT_TRUE(std::any_of(
                                approximate->GetCandidates().begin(),
                                approximate->GetCandidates().end(),
                                [&](auto const& approximate_candidate) {
                                    return approximate_candidate.left <= exact_candidate.left &&
                                           exact_candidate.right <= approximate_candidate.right &&
                                           approximate_candidate.confidence >= relaxed_threshold;
                                }))
                                << "No approximate cover for exact interval ["
                                << exact_candidate.left << ", " << exact_candidate.right << ']';
                    }
                }
            }
        }
    }
}

TEST(SDMinerTest, ValidatesConditionalGapOptionsWhenSet) {
    ScopedTestFile file("sd_miner_option_lifecycle_test.csv", kExactGapCsv);
    algos::sd_miner::SDMiner miner;
    algos::LoadAlgorithmData(
            miner, {{config::names::kCsvConfig, CSVConfig{file.filename.c_str(), ',', true}}});

    EXPECT_THROW(miner.SetOption(config::names::kSdG2, 1.0), config::ConfigurationError);
    EXPECT_THROW(miner.SetOption(config::names::kSdG1, -1.0), std::runtime_error);
    miner.SetOption(config::names::kSdG1, 2.0);
    EXPECT_TRUE(miner.GetNeededOptions().contains(config::names::kSdG2));
    EXPECT_THROW(miner.SetOption(config::names::kSdG2, 1.0), std::runtime_error);
    EXPECT_NO_THROW(miner.SetOption(config::names::kSdG2, 3.0));
}

TEST(SDMinerTest, RejectsInvalidGapRange) {
    ScopedTestFile file("sd_miner_invalid_gap_test.csv", kExactGapCsv);

    EXPECT_THROW((void)RunMiner(file.filename, {.g1 = 5.0, .g2 = 1.0}), std::runtime_error);
}

TEST(SDMinerTest, RejectsZeroWidthZeroGap) {
    ScopedTestFile file("sd_miner_zero_gap_test.csv", kExactGapCsv);

    EXPECT_THROW((void)RunMiner(file.filename, {.g1 = 0.0, .g2 = 0.0}), std::runtime_error);
}

TEST(SDMinerTest, RejectsNonNumericValues) {
    ScopedTestFile file("sd_miner_non_numeric_test.csv", kNonNumericCsv);

    EXPECT_THROW((void)RunMiner(file.filename), std::runtime_error);
}

TEST(SDMinerTest, RejectsNonFiniteGaps) {
    ScopedTestFile file("sd_miner_non_finite_gap_test.csv", kExactGapCsv);
    std::vector<double> const non_finite_values{std::numeric_limits<double>::quiet_NaN(),
                                                std::numeric_limits<double>::infinity(),
                                                -std::numeric_limits<double>::infinity()};

    for (double value : non_finite_values) {
        EXPECT_THROW((void)RunMiner(file.filename, {.g1 = value, .g2 = -1.0}), std::runtime_error);
        EXPECT_THROW((void)RunMiner(file.filename, {.g1 = 0.0, .g2 = value}), std::runtime_error);
    }
}

TEST(SDMinerTest, RejectsNonFiniteDataValues) {
    ScopedTestFile lhs_file("sd_miner_non_finite_lhs_test.csv", kNonFiniteLhsCsv);
    ScopedTestFile rhs_file("sd_miner_non_finite_rhs_test.csv", kNonFiniteRhsCsv);

    EXPECT_THROW((void)RunMiner(lhs_file.filename), std::runtime_error);
    EXPECT_THROW((void)RunMiner(rhs_file.filename), std::runtime_error);
}

TEST(SDMinerTest, RejectsInvalidConfidenceThreshold) {
    ScopedTestFile file("sd_miner_invalid_confidence_test.csv", kExactGapCsv);
    auto params = BaseParams(file.filename);
    params[config::names::kSdMinimumConfidence] = 1.1;

    EXPECT_THROW((void)algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params),
                 std::runtime_error);
}

TEST(SDMinerTest, RejectsInvalidSupportThreshold) {
    ScopedTestFile file("sd_miner_invalid_support_test.csv", kExactGapCsv);
    auto params = BaseParams(file.filename);
    params[config::names::kSdMinimumSupport] = -0.1;

    EXPECT_THROW((void)algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params),
                 std::runtime_error);
}

TEST(SDMinerTest, RejectsNonFiniteProbabilityThresholds) {
    ScopedTestFile file("sd_miner_non_finite_threshold_test.csv", kExactGapCsv);

    auto confidence_params = BaseParams(file.filename);
    confidence_params[config::names::kSdMinimumConfidence] =
            std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW((void)algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(confidence_params),
                 std::runtime_error);

    auto support_params = BaseParams(file.filename);
    support_params[config::names::kSdMinimumSupport] = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW((void)algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(support_params),
                 std::runtime_error);
}

TEST(SDMinerTest, ReportsInvalidThresholdName) {
    ScopedTestFile file("sd_miner_threshold_message_test.csv", kExactGapCsv);
    std::array<double, 4> const invalid_values{-0.1, 1.1, std::numeric_limits<double>::quiet_NaN(),
                                               std::numeric_limits<double>::infinity()};
    for (double value : invalid_values) {
        std::array<std::pair<MinerOptions, std::string>, 2> const cases{
                {{{.min_confidence = value}, "minimum_confidence must be in [0, 1]."},
                 {{.min_support = value}, "minimum_support must be in [0, 1]."}}};
        for (auto const& [options, message] : cases) {
            SCOPED_TRACE(::testing::Message() << message << " value=" << value);
            try {
                (void)RunMiner(file.filename, options);
                FAIL() << "Expected an invalid threshold to be rejected.";
            } catch (std::runtime_error const& error) {
                EXPECT_EQ(error.what(), message);
            }
        }
    }
}

TEST(SDMinerTest, RejectsInvalidApproximationDelta) {
    ScopedTestFile file("sd_miner_invalid_delta_test.csv", kExactGapCsv);
    auto params = BaseParams(file.filename);
    params[config::names::kSdDelta] = 0.0;

    EXPECT_THROW((void)algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params),
                 std::runtime_error);
}

TEST(SDMinerTest, RejectsApproximationDeltaTooSmallToAdvance) {
    ScopedTestFile file("sd_miner_tiny_delta_test.csv", kExactGapCsv);
    auto params = BaseParams(file.filename);
    params[config::names::kSdDelta] = std::numeric_limits<double>::denorm_min();

    EXPECT_THROW((void)algos::CreateAndLoadAlgorithm<algos::sd_miner::SDMiner>(params),
                 std::runtime_error);
}

}  // namespace tests
