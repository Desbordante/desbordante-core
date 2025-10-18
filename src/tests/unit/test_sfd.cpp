#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/container/allocator_traits.hpp>
#include <gtest/gtest.h>

#include "algo_factory.h"
#include "algorithms/fd/sfd/cords.h"
#include "algorithms/fd/sfd/frequency_handler.h"
#include "algorithms/fd/sfd/sample.h"
#include "all_csv_configs.h"
#include "config/max_lhs/type.h"
#include "config/names.h"
#include "csv_config_util.h"
#include "fd/fd.h"
#include "fd/sfd/correlation.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "table/column.h"
#include "table/column_index.h"
#include "table/typed_column_data.h"

struct CSVConfig;

namespace {
void AssertVectors(std::vector<Column> const& expected,
                   std::vector<model::ColumnIndex> const& actual) {
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); i++) {
        ASSERT_EQ(expected[i].GetIndex(), actual[i]);
    }
}

void AssertSFDList(std::list<FD> const& actual,
                   std::vector<std::pair<model::ColumnIndex, model::ColumnIndex>> expected) {
    ASSERT_EQ(actual.size(), expected.size());
    size_t ind = 0;
    for (auto i : actual) {
        ASSERT_EQ(i.GetLhsIndices()[0], expected[ind].first);
        ASSERT_EQ(i.GetRhsIndex(), expected[ind++].second);
    }
}

void AssertCorrsList(std::list<algos::Correlation> const& actual,
                     std::vector<std::pair<model::ColumnIndex, model::ColumnIndex>> expected) {
    ASSERT_EQ(actual.size(), expected.size());
    size_t ind = 0;
    for (auto i : actual) {
        ASSERT_EQ(i.GetLhsIndex(), expected[ind].first);
        ASSERT_EQ(i.GetRhsIndex(), expected[ind++].second);
    }
}
}  // namespace

namespace tests {

TEST(TestCordsUtils, FrequenciesOfIris) {
    std::vector<std::vector<std::pair<std::string, size_t>>> expected = {
            {{"4.300000", 34}, {"4.500000", 33}, {"5.300000", 32}, {"7.100000", 30},
             {"5.400000", 11}, {"6.600000", 24}, {"6.100000", 8},  {"7.600000", 27},
             {"5.800000", 6},  {"4.800000", 14}, {"5.200000", 18}, {"6.400000", 5},
             {"4.900000", 12}, {"5.500000", 7},  {"5.700000", 4},  {"4.600000", 19},
             {"5.100000", 2},  {"6.000000", 9},  {"5.600000", 10}, {"6.700000", 3},
             {"6.300000", 1},  {"6.500000", 13}, {"6.200000", 17}, {"7.700000", 15},
             {"7.200000", 20}, {"7.300000", 29}, {"6.900000", 16}, {"7.900000", 26},
             {"6.800000", 21}, {"7.000000", 31}, {"5.900000", 22}, {"4.700000", 25},
             {"5.000000", 0},  {"4.400000", 23}, {"7.400000", 28}},

            {{"4.000000", 21}, {"4.200000", 19}, {"4.400000", 18}, {"2.400000", 15},
             {"4.100000", 20}, {"3.600000", 14}, {"3.700000", 13}, {"2.800000", 1},
             {"3.800000", 8},  {"3.200000", 2},  {"3.900000", 17}, {"3.100000", 4},
             {"3.300000", 10}, {"2.900000", 5},  {"2.000000", 22}, {"2.500000", 7},
             {"3.400000", 3},  {"3.500000", 9},  {"2.300000", 12}, {"2.200000", 16},
             {"3.000000", 0},  {"2.700000", 6},  {"2.600000", 11}},

            {{"1.000000", 42}, {"1.100000", 41}, {"3.000000", 40}, {"3.600000", 39},
             {"6.300000", 36}, {"6.600000", 34}, {"6.900000", 33}, {"5.900000", 24},
             {"4.900000", 7},  {"4.500000", 3},  {"1.300000", 5},  {"1.900000", 31},
             {"4.800000", 11}, {"5.800000", 16}, {"3.700000", 38}, {"5.200000", 27},
             {"5.400000", 25}, {"5.100000", 2},  {"6.000000", 23}, {"1.600000", 4},
             {"4.400000", 12}, {"5.000000", 10}, {"6.700000", 22}, {"6.400000", 35},
             {"3.500000", 29}, {"1.500000", 0},  {"4.200000", 13}, {"6.100000", 15},
             {"3.300000", 30}, {"4.700000", 8},  {"1.700000", 14}, {"3.800000", 37},
             {"1.400000", 1},  {"5.700000", 17}, {"4.600000", 19}, {"5.500000", 18},
             {"1.200000", 32}, {"4.100000", 20}, {"4.000000", 9},  {"5.600000", 6},
             {"3.900000", 21}, {"5.300000", 26}, {"4.300000", 28}},

            {{"0.600000", 20}, {"1.700000", 19}, {"1.100000", 18}, {"1.200000", 13},
             {"0.200000", 0},  {"2.400000", 16}, {"1.300000", 1},  {"2.100000", 9},
             {"1.800000", 2},  {"2.200000", 17}, {"0.400000", 7},  {"1.500000", 3},
             {"0.100000", 11}, {"2.300000", 4},  {"1.400000", 5},  {"1.000000", 6},
             {"0.300000", 8},  {"0.500000", 21}, {"2.500000", 15}, {"1.600000", 14},
             {"2.000000", 10}, {"1.900000", 12}},

            {{"Iris-setosa", 2}, {"Iris-versicolor", 1}, {"Iris-virginica", 0}}};

    auto table = MakeInputTable(kIris);
    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation =
            model::ColumnLayoutTypedRelationData::CreateFrom(*table, false);
    std::vector<model::TypedColumnData> const& data = typed_relation->GetColumnData();

    algos::FrequencyHandler handler;
    handler.InitFrequencyHandler(data, data.size(), 70);

    ASSERT_EQ(handler.Size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++) {
        ASSERT_EQ(handler.ColumnFrequencyMapSize(i), expected[i].size());
        for (auto const& [value, ordinal_number] : expected[i]) {
            ASSERT_EQ(handler.ContainsValAtColumn(value, i), true);
            ASSERT_EQ(handler.GetValueOrdinalNumberAtColumn(value, i), ordinal_number);
        }
    }
}

TEST(TestCordsUtils, SampleSize) {
    ASSERT_EQ(algos::Sample::CalculateSampleSize(465, 4, 1e-06, 0.05), 4215);
    ASSERT_EQ(algos::Sample::CalculateSampleSize(472, 7, 1e-06, 0.05), 3005);
    ASSERT_EQ(algos::Sample::CalculateSampleSize(4198, 2632, 0.1, 0.1), 111);
    ASSERT_EQ(algos::Sample::CalculateSampleSize(765987, 292784, 0.149804, 0.14367), 168);
    ASSERT_EQ(algos::Sample::CalculateSampleSize(640568, 365981, 0.173673, 0.284799), 64);
    ASSERT_EQ(algos::Sample::CalculateSampleSize(319, 4, 0.0181818, 0.16), 485);
}

class CordsAlgorithmTest : public ::testing::Test {
public:
    struct Config {
        bool is_null_equal_null;
        bool only_sfd;
        bool fixed_sample;
        long double min_cardinality;
        long double max_diff_vals_proportion;
        long double min_sfd_strength_measure;
        long double min_skew_threshold;
        long double min_structural_zeroes_amount;
        long double max_false_positive_probability;
        long double delta;
        size_t max_amount_of_categories;
        config::MaxLhsType max_lhs;
    };

    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, Config const& test_config) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kEqualNulls, test_config.is_null_equal_null},
                {kOnlySFD, test_config.only_sfd},
                {kMinCard, test_config.min_cardinality},
                {kMaxDiffValsProportion, test_config.max_diff_vals_proportion},
                {kMinSFDStrengthMeasure, test_config.min_sfd_strength_measure},
                {kMinSkewThreshold, test_config.min_skew_threshold},
                {
                        kMinStructuralZeroesAmount,
                        test_config.min_structural_zeroes_amount,
                },
                {kMaxFalsePositiveProbability, test_config.max_false_positive_probability},
                {kDelta, test_config.delta},
                {kMaxAmountOfCategories, test_config.max_amount_of_categories},
                {kMaximumLhs, test_config.max_lhs},
                {kFixedSample, test_config.fixed_sample}};
    }

    static std::unique_ptr<algos::Cords> CreateCordsInstance(CSVConfig const& csv_config,
                                                             Config const& test_config) {
        return algos::CreateAndLoadAlgorithm<algos::Cords>(GetParamMap(csv_config, test_config));
    }
};

using TestConfig = CordsAlgorithmTest::Config;
TestConfig const kTestConfigDefault{
        .is_null_equal_null = true,
        .only_sfd = false,
        .fixed_sample = true,
        .min_cardinality = 0.04L,
        .max_diff_vals_proportion = 0.4L,
        .min_sfd_strength_measure = 0.3L,
        .min_skew_threshold = 0.3L,
        .min_structural_zeroes_amount = 1e-01L,
        .max_false_positive_probability = 1e-06L,
        .delta = 0.05L,
        .max_amount_of_categories = 70LU,
        .max_lhs = 1,
};

TEST_F(CordsAlgorithmTest, LineItem) {
    auto a = CreateCordsInstance(kLineItem, kTestConfigDefault);
    a->Execute();
    AssertVectors(a->GetSoftKeys(), {1, 2, 5, 15});
    AssertVectors(a->GetTrivialColumns(), {});
    AssertSFDList(a->FdList(),
                  {{0, 8},  {0, 9},  {10, 3},  {11, 3},  {12, 3},  {10, 6},  {11, 6},  {12, 6},
                   {10, 7}, {11, 7}, {12, 7},  {8, 9},   {10, 8},  {11, 8},  {12, 8},  {10, 9},
                   {11, 9}, {12, 9}, {10, 13}, {10, 14}, {11, 13}, {11, 14}, {12, 13}, {12, 14}});
}

TEST_F(CordsAlgorithmTest, iris) {
    auto a = CreateCordsInstance(kIris, kTestConfigDefault);

    a->Execute();
    AssertVectors(a->GetSoftKeys(), {});
    AssertVectors(a->GetTrivialColumns(), {});
    AssertSFDList(a->FdList(), {{2, 4}, {3, 4}});
    AssertCorrsList(a->GetCorrelations(),
                    {{0, 1}, {2, 0}, {0, 3}, {0, 4}, {2, 1}, {1, 3}, {1, 4}, {2, 3}});
}

TEST_F(CordsAlgorithmTest, CIPublicHighway10k) {
    auto a = CreateCordsInstance(kCIPublicHighway10k, kTestConfigDefault);
    a->Execute();
    AssertVectors(a->GetSoftKeys(), {0, 2});
    AssertVectors(a->GetTrivialColumns(), {1, 7, 8, 9, 10, 14, 15, 16, 17});
    AssertSFDList(a->FdList(), {{11, 3}, {11, 4}, {11, 5}, {11, 6}, {11, 12}});
    AssertCorrsList(a->GetCorrelations(), {{3, 4},
                                           {5, 3},
                                           {3, 6},
                                           {12, 3},
                                           {13, 3},
                                           {5, 4},
                                           {4, 6},
                                           {12, 4},
                                           {13, 4},
                                           {5, 6},
                                           {12, 5},
                                           {13, 5},
                                           {12, 6},
                                           {13, 6},
                                           {11, 13},
                                           {13, 12}});
}

}  // namespace tests
