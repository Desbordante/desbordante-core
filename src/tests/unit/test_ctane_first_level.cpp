#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/cfd/ctane/c_lattice_level.h"
#include "core/algorithms/cfd/model/cfd_relation_data.h"
#include "core/algorithms/cfd/model/cfd_types.h"
#include "core/algorithms/cfd/model/partition_tidlist.h"
#include "core/model/table/idataset_stream.h"

namespace tests {
namespace {

class VectorDatasetStream final : public model::IDatasetStream {
private:
    std::vector<std::string> column_names_;
    std::vector<Row> rows_;
    std::size_t next_row_ = 0;

public:
    VectorDatasetStream(std::vector<std::string> column_names, std::vector<Row> rows)
        : column_names_(std::move(column_names)), rows_(std::move(rows)) {}

    Row GetNextRow() final {
        return rows_.at(next_row_++);
    }

    bool HasNextRow() const final {
        return next_row_ < rows_.size();
    }

    std::size_t GetNumberOfColumns() const final {
        return column_names_.size();
    }

    std::string GetColumnName(std::size_t index) const final {
        return column_names_.at(index);
    }

    std::string GetRelationName() const final {
        return "cust";
    }

    void Reset() final {
        next_row_ = 0;
    }
};

std::unique_ptr<algos::cfd::CFDRelationData> CreateArticleRelation() {
    // Fig. 1 from Fan et al., "Discovering Conditional Functional Dependencies" (2011)
    VectorDatasetStream input({"CC", "AC", "PN", "NM", "STR", "CT", "ZIP"},
                              {{"01", "908", "1111111", "Mike", "Tree Ave.", "MH", "07974"},
                               {"01", "908", "1111111", "Rick", "Tree Ave.", "MH", "07974"},
                               {"01", "212", "2222222", "Joe", "5th Ave", "NYC", "01202"},
                               {"01", "908", "2222222", "Jim", "Elm Str.", "MH", "07974"},
                               {"44", "131", "3333333", "Ben", "High St.", "EDI", "EH4 1DT"},
                               {"44", "131", "4444444", "Ian", "High St.", "EDI", "EH4 1DT"},
                               {"44", "908", "4444444", "Ian", "Port Pl", "MH", "W1B 1JH"},
                               {"01", "131", "2222222", "Sean", "3rd St.", "UN", "01202"}});
    return algos::cfd::CFDRelationData::CreateFrom(input);
}

using Pattern = std::pair<algos::cfd::AttributeIndex, algos::cfd::Item>;

std::set<Pattern> GetLevelPatterns(CLatticeLevel const& level) {
    std::set<Pattern> patterns;
    for (auto const& vertex : level.GetVertices()) {
        auto const& values = vertex->GetTuplePattern().GetPatternValues();
        EXPECT_EQ(values.size(), 1);
        patterns.insert(*values.begin());
    }
    return patterns;
}

CLatticeVertex const* FindVertex(CLatticeLevel const& level,
                                 algos::cfd::AttributeIndex column_index, algos::cfd::Item item) {
    auto const it = std::ranges::find_if(level.GetVertices(), [&](auto const& vertex) {
        return vertex->GetTuplePattern().HasColumnPattern(column_index, item);
    });
    return it == level.GetVertices().end() ? nullptr : it->get();
}

std::vector<std::vector<int>> GetClusters(algos::cfd::PartitionTIdList const& partition) {
    std::vector<std::vector<int>> clusters;
    std::vector<int> cluster;
    for (int tid : partition.tids) {
        if (tid == algos::cfd::PartitionTIdList::kSep) {
            clusters.push_back(std::move(cluster));
            cluster.clear();
        } else {
            cluster.push_back(tid);
        }
    }
    if (!cluster.empty()) clusters.push_back(std::move(cluster));
    std::sort(clusters.begin(), clusters.end());
    return clusters;
}

class CTaneFirstLevelTest : public ::testing::Test {
protected:
    static constexpr unsigned kMinSupport = 2;

    std::unique_ptr<algos::cfd::CFDRelationData> relation_;
    std::unique_ptr<CLatticeVertex> empty_vertex_;
    std::vector<std::unique_ptr<CLatticeLevel>> levels_;

    void SetUp() final {
        relation_ = CreateArticleRelation();
        empty_vertex_ = CLatticeLevel::GenerateFirstLevel(levels_, *relation_, kMinSupport);
    }

    algos::cfd::Item ItemAt(algos::cfd::AttributeIndex column_index,
                            std::string const& value) const {
        auto const item = relation_->GetItem(column_index, value);
        EXPECT_GT(item, 0);
        return item;
    }
};

TEST_F(CTaneFirstLevelTest, CreatesExactlyFrequentSingletonPatternsFromArticle) {
    ASSERT_EQ(levels_.size(), 1);
    ASSERT_EQ(levels_.front()->GetArity(), 1);

    std::vector<std::vector<std::string>> const frequent_values = {
            {"01", "44"},
            {"908", "131"},
            {"1111111", "2222222", "4444444"},
            {"Ian"},
            {"Tree Ave.", "High St."},
            {"MH", "EDI"},
            {"07974", "01202", "EH4 1DT"},
    };

    std::set<Pattern> expected;
    for (algos::cfd::AttributeIndex column_index = 0;
         static_cast<std::size_t>(column_index) < frequent_values.size(); ++column_index) {
        expected.emplace(column_index, -1 - column_index);
        for (auto const& value : frequent_values[column_index]) {
            expected.emplace(column_index, ItemAt(column_index, value));
        }
    }

    EXPECT_EQ(GetLevelPatterns(*levels_.front()), expected);
    EXPECT_EQ(levels_.front()->GetVertices().size(), 22);

    for (auto const& vertex : levels_.front()->GetVertices()) {
        auto const& column_indices = vertex->GetColumnIndices();
        EXPECT_EQ(column_indices.size(), relation_->GetNumColumns());
        EXPECT_EQ(column_indices.count(), 1);
        auto const column_index = vertex->GetTuplePattern().GetPatternValues().begin()->first;
        EXPECT_TRUE(column_indices.test(static_cast<std::size_t>(column_index)));
    }
}

TEST_F(CTaneFirstLevelTest, BuildsPartitionsAndSharesEmptyParent) {
    ASSERT_NE(empty_vertex_, nullptr);
    EXPECT_EQ(levels_.front()->GetLatticeVertex(empty_vertex_->GetTuplePattern()), nullptr);
    for (auto const& vertex : levels_.front()->GetVertices()) {
        EXPECT_EQ(levels_.front()->GetLatticeVertex(vertex->GetTuplePattern()), vertex.get());
    }

    EXPECT_EQ(empty_vertex_->GetColumnIndices().size(), relation_->GetNumColumns());
    EXPECT_TRUE(empty_vertex_->GetColumnIndices().none());
    auto const* empty_partition = empty_vertex_->GetPositionListIndex();
    ASSERT_NE(empty_partition, nullptr);
    EXPECT_EQ(empty_partition->sets_number, 1);
    EXPECT_EQ(empty_partition->tids, (algos::cfd::SimpleTIdList{0, 1, 2, 3, 4, 5, 6, 7}));

    auto const* ac_variable = FindVertex(*levels_.front(), 1, -2);
    ASSERT_NE(ac_variable, nullptr);
    auto const* ac_variable_partition = ac_variable->GetPositionListIndex();
    ASSERT_NE(ac_variable_partition, nullptr);
    EXPECT_EQ(ac_variable_partition->sets_number, 3);
    EXPECT_EQ(ac_variable_partition->Support(), 8);
    EXPECT_EQ(GetClusters(*ac_variable_partition),
              (std::vector<std::vector<int>>{{0, 1, 3, 6}, {2}, {4, 5, 7}}));

    auto const* ac_131 = FindVertex(*levels_.front(), 1, ItemAt(1, "131"));
    ASSERT_NE(ac_131, nullptr);
    auto const* ac_131_partition = ac_131->GetPositionListIndex();
    ASSERT_NE(ac_131_partition, nullptr);
    EXPECT_EQ(ac_131_partition->sets_number, 1);
    EXPECT_EQ(ac_131_partition->tids, (algos::cfd::SimpleTIdList{4, 5, 7}));

    for (auto const& vertex : levels_.front()->GetVertices()) {
        ASSERT_EQ(vertex->GetParents().size(), 1);
        EXPECT_EQ(vertex->GetParents().front(), empty_vertex_.get());
    }
}

TEST_F(CTaneFirstLevelTest, AssignsExpectedRhsCandidatesToEveryVertex) {
    std::set<algos::cfd::Item> all_candidates;
    for (auto const& vertex : levels_.front()->GetVertices()) {
        auto const& values = vertex->GetTuplePattern().GetPatternValues();
        ASSERT_EQ(values.size(), 1);
        all_candidates.insert(values.begin()->second);
    }
    // 22 is a sum of sizes of all frequent_values's elements (15) plus wildcard-patterns (7)
    ASSERT_EQ(all_candidates.size(), 22);

    for (auto const& vertex : levels_.front()->GetVertices()) {
        auto const& [column_index, item] = *vertex->GetTuplePattern().GetPatternValues().begin();
        std::set<algos::cfd::Item> expected = all_candidates;
        std::erase_if(expected, [&](algos::cfd::Item candidate) {
            return relation_->GetAttrIndex(candidate) == column_index && candidate != item;
        });

        auto const& actual_candidates = vertex->GetRhsCandidates();
        EXPECT_TRUE(std::ranges::is_sorted(actual_candidates));
        EXPECT_EQ(std::set<algos::cfd::Item>(actual_candidates.begin(), actual_candidates.end()),
                  expected);
    }
}

TEST(CTaneLatticeVertexTest, IntersectsSortedRhsCandidates) {
    algos::cfd::Itemset const lhs{-7, -3, 1, 4, 8};
    algos::cfd::Itemset const rhs{-7, -2, 1, 5, 8};

    EXPECT_EQ(CLatticeVertex::IntersectRhsCandidates(lhs, rhs), (algos::cfd::Itemset{-7, 1, 8}));
}

}  // namespace
}  // namespace tests
