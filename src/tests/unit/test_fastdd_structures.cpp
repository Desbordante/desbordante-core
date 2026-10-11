#include <list>
#include <optional>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/fastdd.h"
#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/threshold_calculator.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {

using FastDDCSVConfig = std::pair<CSVConfig, std::optional<CSVConfig>>;

class PliShardTest : public ::testing::TestWithParam<CSVConfig> {};

TEST_P(PliShardTest, Default) {
    using namespace algos::dd;

    auto const& csv_config = GetParam();
    auto typed_relation = model::ColumnLayoutTypedRelationData::CreateFrom(
            *MakeInputTable(csv_config), false, true);
    PliShardBuilder pli_shard_builder(800UL);
    std::vector<PliShard> pli_shards =
            pli_shard_builder.BuildPliShards(typed_relation->GetColumnData());
    for (auto const& pli_shard : pli_shards) {
        std::vector<Pli> const& plis = pli_shard.Plis();
        for (std::size_t column_index = 0; column_index != plis.size(); ++column_index) {
            model::TypedColumnData const& column = typed_relation->GetColumnData()[column_index];
            model::Type const& type = column.GetType();
            auto const& pli = plis[column_index];
            for (auto const& cluster : pli.GetClusters()) {
                ASSERT_FALSE(cluster.empty()) << "cluster is unexpectedly empty";
                auto expected_value = column.GetValue(cluster.front());
                for (model::TupleIndex tuple_index : cluster) {
                    auto actual_value = column.GetValue(tuple_index);
                    EXPECT_EQ(type.Compare(expected_value, actual_value),
                              model::CompareResult::kEqual)
                            << "PLI value mismatch in column" << column_index << " at row "
                            << tuple_index;
                }
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(FastDDTest, PliShardTest,
                         ::testing::Values(kTestDD, kTestDD1, kTestDD2, kTestDD3, kTestDD4,
                                           kTestDD5, kAdult));

struct ThresholdTestParams {
    std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation;
    std::vector<std::vector<model::DFConstraint>> expected_thresholds;

    ThresholdTestParams(CSVConfig const& csv_config,
                        std::vector<std::vector<model::DFConstraint>>&& expected)
        : difference_typed_relation(model::ColumnLayoutTypedRelationData::CreateFrom(
                  *MakeInputTable(csv_config), false)),
          expected_thresholds(std::move(expected)) {}
};

class ThresholdTest : public ::testing::TestWithParam<ThresholdTestParams> {};

TEST_P(ThresholdTest, Default) {
    using namespace algos::dd;

    auto const& params = GetParam();
    ThresholdCalculator threshold_calculator(params.difference_typed_relation, nullptr, 0,
                                             params.difference_typed_relation->GetNumColumns());
    std::vector<std::vector<model::DFConstraint>> const& actual_thresholds =
            threshold_calculator.GetThresholds();
    EXPECT_EQ(params.expected_thresholds, actual_thresholds) << "thresholds mismatched";
}

INSTANTIATE_TEST_SUITE_P(
        FastDDTest, ThresholdTest,
        ::testing::Values(
                ThresholdTestParams(kTestDif2, {{{0, 0}, {0, 1}, {0, 2}},
                                                {{0, 4}, {0, 6}, {0, 8}},
                                                {{0, 5}, {0, 7}, {0, 12}}}),
                ThresholdTestParams(kTestDif3,
                                    {{{0, 0}, {0, 1}, {0, 2}, {1, 1}, {1, 2}, {2, 2}},
                                     {{4, 4}, {4, 8}, {8, 8}},
                                     {{5, 5}, {5, 7}, {5, 12}, {7, 7}, {7, 12}, {12, 12}}}),
                ThresholdTestParams(kTestDif6, {{{0, 0}, {0, 1}, {0, 2}},
                                                {{0, 4}, {0, 6}, {0, 8}},
                                                {{0.12, 51e-1}, {1.5e-1, 7.01}, {0, 1.2E1}}})));

}  // namespace tests