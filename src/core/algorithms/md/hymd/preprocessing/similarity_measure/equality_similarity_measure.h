#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/indexes/keyed_position_list_index.h"
#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/column_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/transformed_columns_holder.h"

namespace algos::hymd::preprocessing::similarity_measure {
namespace detail {
using namespace indexes;

class EqualityTransformer {
public:
    TransformedColumnsHolder<GlobalValueIdentifier, GlobalValueIdentifier> Transform(
            std::vector<std::string> const&,
            std::vector<GlobalValueIdentifier> const& left_pli_keys,
            std::vector<GlobalValueIdentifier> const& right_pli_keys) const {
        return {&left_pli_keys, &right_pli_keys};
    }
};

class EqualityCalculator {
public:
    SimilarityMeasureOutput Calculate(std::vector<GlobalValueIdentifier> const* left_elements,
                                      std::vector<GlobalValueIdentifier> const* right_elements,
                                      KeyedPositionListIndex const& right_pli,
                                      util::WorkerThreadPool*) const {
        std::vector<ColumnClassifierValueId> lhs_ccv_ids;
        std::vector<Similarity> classifier_values;
        SimilarityMatrix similarity_matrix;
        SimilarityIndex similarity_index;
        std::size_t cluster_num = right_pli.GetClusters().size();
        if (left_elements == right_elements) {
            if (cluster_num == 1) {
                lhs_ccv_ids = {kLowestCCValueId};
                classifier_values = {1.0};
            } else {
                lhs_ccv_ids = {kLowestCCValueId, 1};
                classifier_values = {kLowestBound, 1.0};
                for (ValueIdentifier right_value_id = 0; right_value_id != cluster_num;
                     ++right_value_id) {
                    PliCluster const& cluster = right_pli.GetClusters()[right_value_id];
                    similarity_matrix.push_back({{right_value_id, 1}});
                    similarity_index.push_back({{cluster, {{1, cluster.size()}}}});
                }
            }
        } else {
            if (cluster_num == 1 && left_elements->size() == 1) {
                if (left_elements->front() == right_elements->front()) {
                    lhs_ccv_ids = {kLowestCCValueId};
                    classifier_values = {1.0};
                } else {
                    lhs_ccv_ids = {kLowestCCValueId, 1};
                    classifier_values = {kLowestBound, 1.0};
                }
            } else {
                lhs_ccv_ids = {kLowestCCValueId, 1};
                classifier_values = {kLowestBound, 1.0};
                std::unordered_map<GlobalValueIdentifier, ValueIdentifier> const& mapping =
                        right_pli.GetMapping();
                for (GlobalValueIdentifier global_left_value_id : *left_elements) {
                    auto it = mapping.find(global_left_value_id);
                    if (it == mapping.end()) {
                        similarity_matrix.emplace_back();
                        similarity_index.emplace_back();
                        continue;
                    }
                    ValueIdentifier right_value_id = it->second;
                    PliCluster const& cluster = right_pli.GetClusters()[right_value_id];
                    similarity_matrix.push_back({{right_value_id, 1}});
                    similarity_index.push_back({{cluster, {{1, cluster.size()}}}});
                }
            }
        }
        return {std::move(lhs_ccv_ids),
                {std::move(classifier_values), std::move(similarity_matrix),
                 std::move(similarity_index)}};
    }
};

using EqualityBase = ColumnSimilarityMeasure<EqualityTransformer, EqualityCalculator>;
}  // namespace detail

class EqualitySimilarityMeasure final : public detail::EqualityBase {
    static constexpr auto kName = "equality_similarity";

public:
    EqualitySimilarityMeasure(ColumnIdentifier left_column_identifier,
                              ColumnIdentifier right_column_identifier)
        : detail::EqualityBase(true, kName, std::move(left_column_identifier),
                               std::move(right_column_identifier), {}, {}) {}
};

}  // namespace algos::hymd::preprocessing::similarity_measure
