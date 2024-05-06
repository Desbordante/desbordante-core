#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "config/exceptions.h"

namespace algos::hymd::preprocessing::similarity_measure {

using SimInfo = std::map<Similarity, indexes::RecSet>;

indexes::ColumnMatchSimilarityInfo DistanceSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const& clusters_right) const {
    std::vector<model::md::DecisionBoundary> decision_bounds;
    indexes::SimilarityMatrix similarity_matrix;
    indexes::SimilarityIndex similarity_index;
    auto const& data_left_size = data_info_left->GetElementNumber();
    auto const& data_right_size = data_info_right->GetElementNumber();
    Similarity lowest = 1.0;
    for (ValueIdentifier value_id_left = 0; value_id_left < data_left_size; ++value_id_left) {
        std::vector<std::pair<Similarity, RecordIdentifier>> sim_rec_id_vec;
        std::byte const* left_value = data_info_left->GetAt(value_id_left);
        double max_distance = 0;
        std::vector<double> distances;
        distances.reserve(data_info_right->GetElementNumber());
        for (ValueIdentifier right_index = 0; right_index < data_right_size; ++right_index) {
            std::byte const* right_value = data_info_right->GetAt(right_index);
            Similarity distance = compute_distance_(left_value, right_value);
            distances.push_back(distance);
            max_distance = std::max(max_distance, distance);
        }
        auto get_similarity = [max_distance, &distances](ValueIdentifier value_id_right) {
            if (max_distance == 0) return 1.0;
            Similarity distance = distances[value_id_right];
            return static_cast<Similarity>(max_distance - distance) /
                   static_cast<Similarity>(max_distance);
        };
        for (ValueIdentifier value_id_right = 0; value_id_right < data_right_size;
             ++value_id_right) {
            Similarity similarity = get_similarity(value_id_right);
            if (similarity < min_sim_) {
                lowest = 0.0;
                continue;
            }
            if (lowest > similarity) lowest = similarity;
            decision_bounds.push_back(similarity);
            similarity_matrix[value_id_left][value_id_right] = similarity;
            for (RecordIdentifier record_id : clusters_right.operator[](value_id_right)) {
                sim_rec_id_vec.emplace_back(similarity, record_id);
            }
        }
        if (sim_rec_id_vec.empty()) continue;
        std::sort(sim_rec_id_vec.begin(), sim_rec_id_vec.end(), std::greater<>{});
        std::vector<RecordIdentifier> records;
        records.reserve(sim_rec_id_vec.size());
        for (auto [_, rec] : sim_rec_id_vec) {
            records.push_back(rec);
        }
        SimInfo sim_info;
        Similarity previous_similarity = sim_rec_id_vec.begin()->first;
        auto const it_begin = records.begin();
        for (model::Index j = 0; j < sim_rec_id_vec.size(); ++j) {
            Similarity const similarity = sim_rec_id_vec[j].first;
            if (similarity == previous_similarity) continue;
            auto const it_end = it_begin + static_cast<long>(j);
            // TODO: use std::inplace_merge
            std::sort(it_begin, it_end);
            sim_info[previous_similarity] = {it_begin, it_end};
            previous_similarity = similarity;
        }
        std::sort(records.begin(), records.end());
        sim_info[previous_similarity] = {records.begin(), records.end()};
        similarity_index[value_id_left] = std::move(sim_info);
    }
    std::sort(decision_bounds.begin(), decision_bounds.end());
    decision_bounds.erase(std::unique(decision_bounds.begin(), decision_bounds.end()),
                          decision_bounds.end());
    return {std::move(decision_bounds), lowest, std::move(similarity_matrix),
            std::move(similarity_index)};
}

}  // namespace algos::hymd::preprocessing::similarity_measure
