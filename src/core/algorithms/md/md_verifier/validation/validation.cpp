#include "algorithms/md/md_verifier/validation/validation.h"

#include <iostream>

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/index_range.h"
#include "algorithms/md/md_verifier/validation/rows_pairs.h"
#include "util/worker_thread_pool.h"

namespace algos::md {
ColumnInfoView MDValidationCalculator::GetColumnInfo(
        hymd::ColumnMatchInfo const& column_match_info) {
    return {records_info_->GetLeftCompressor()
                    .GetPli(column_match_info.left_column_index)
                    .GetClusters(),
            records_info_->GetLeftCompressor()
                    .GetPli(column_match_info.left_column_index)
                    .GetClusters(),
            column_match_info.similarity_info.similarity_matrix};
}

void MDValidationCalculator::InsertClustersInRowsPairsSet(
        hymd::indexes::PliCluster const& left_cluster,
        hymd::indexes::PliCluster const& right_cluster) {
    for (hymd::RecordIdentifier left_row : left_cluster) {
        for (hymd::RecordIdentifier right_row : right_cluster) {
            rows_pairs_[left_row].emplace(right_row);
        }
    }
}

void MDValidationCalculator::DeleteClustersFromRowsPairsSet(
        hymd::indexes::PliCluster const& left_cluster,
        hymd::indexes::PliCluster const& right_cluster) {
    for (hymd::RecordIdentifier left_row : left_cluster) {
        for (hymd::RecordIdentifier right_row : right_cluster) {
            auto it = rows_pairs_.find(left_row);
            if (it != rows_pairs_.end()) {
                it->second.erase(right_row);
                if (it->second.size() == 0) {
                    rows_pairs_.erase(left_row);
                }
            }
        }
    }
}

void MDValidationCalculator::UpdateNewRowsPairsSet(hymd::indexes::PliCluster const& left_cluster,
                                                   hymd::indexes::PliCluster const& right_cluster,
                                                   RowsPairSet& new_rows_pairs) {
    for (hymd::RecordIdentifier left_row : left_cluster) {
        for (hymd::RecordIdentifier right_row : right_cluster) {
            auto it = rows_pairs_.find(left_row);
            if (it != rows_pairs_.end() && it->second.find(right_row) != it->second.end()) {
                new_rows_pairs[left_row].emplace(right_row);
            }
        }
    }
}

void MDValidationCalculator::InitRowsPairsSet(hymd::ColumnMatchInfo const& column_match_info,
                                              model::md::DecisionBoundary decision_boundary) {
    auto const& [left_clusters, right_clusters, similarity_matrix] =
            GetColumnInfo(column_match_info);
    for (model::Index left_cluster_index : hymd::utility::IndexRange(similarity_matrix.size())) {
        for (auto const [right_cluster_index, similarity_index] :
             similarity_matrix[left_cluster_index]) {
            model::md::Similarity similarity =
                    column_match_info.similarity_info.classifier_values[similarity_index];
            if (similarity >= decision_boundary) {
                InsertClustersInRowsPairsSet(left_clusters[left_cluster_index],
                                             right_clusters[right_cluster_index]);
            }
        }
    }
}

void MDValidationCalculator::UpdateRowsPairsWithLhs(hymd::ColumnMatchInfo const& column_match_info,
                                                    model::md::DecisionBoundary decision_boundary) {
    RowsPairSet new_rows_pairs;
    auto const& [left_clusters, right_clusters, similarity_matrix] =
            GetColumnInfo(column_match_info);
    for (model::Index left_cluster_index : hymd::utility::IndexRange(similarity_matrix.size())) {
        for (auto const [right_cluster_index, similarity_index] :
             similarity_matrix[left_cluster_index]) {
            model::md::Similarity similarity =
                    column_match_info.similarity_info.classifier_values[similarity_index];
            if (similarity >= decision_boundary) {
                UpdateNewRowsPairsSet(left_clusters[left_cluster_index],
                                      right_clusters[right_cluster_index], new_rows_pairs);
            }
        }
    }
    rows_pairs_ = std::move(new_rows_pairs);
}

void MDValidationCalculator::UpdateRowsPairsWithRhs(hymd::ColumnMatchInfo const& column_match_info,
                                                    model::md::DecisionBoundary decision_boundary) {
    auto const& [left_clusters, right_clusters, similarity_matrix] =
            GetColumnInfo(column_match_info);
    for (model::Index left_cluster_index : hymd::utility::IndexRange(left_clusters.size())) {
        for (model::Index right_cluster_index : hymd::utility::IndexRange(right_clusters.size())) {
            model::md::Similarity similarity;
            if (similarity_matrix[left_cluster_index].find(right_cluster_index) ==
                similarity_matrix[left_cluster_index].end()) {
                similarity = 0.0;
            } else {
                model::Index similarity_index =
                        similarity_matrix[left_cluster_index].at(right_cluster_index);
                similarity = column_match_info.similarity_info.classifier_values[similarity_index];
            }

            if (similarity >= decision_boundary) {
                DeleteClustersFromRowsPairsSet(left_clusters[left_cluster_index],
                                               right_clusters[right_cluster_index]);
            } else {
                UpdateRhsSimilarities(left_clusters[left_cluster_index],
                                      right_clusters[right_cluster_index], similarity);
            }
        }
    }
}

void MDValidationCalculator::UpdateRhsSimilarities(hymd::indexes::PliCluster const& left_cluster,
                                                   hymd::indexes::PliCluster const& right_cluster,
                                                   model::md::Similarity rhs_similarity) {
    for (hymd::RecordIdentifier left_row : left_cluster) {
        for (hymd::RecordIdentifier right_row : right_cluster) {
            auto it = rows_pairs_.find(left_row);
            if (it != rows_pairs_.end() && it->second.find(right_row) != it->second.end()) {
                true_rhs_decision_boundary_ = std::min(true_rhs_decision_boundary_, rhs_similarity);
                rhs_pair_to_similarity_[{left_row, right_row}] = rhs_similarity;
            }
        }
    }
}

void MDValidationCalculator::Validate(util::WorkerThreadPool* thread_pool) {
    auto similarity_data =
            hymd::SimilarityData::CreateFrom(records_info_.get(), column_matches_, thread_pool)
                    .first;
    std::vector<hymd::ColumnMatchInfo> column_matches_info = similarity_data.GetColumnMatchesInfo();

    std::vector<model::Index> const& sorted_to_original = similarity_data.GetIndexMapping();
    std::vector<model::Index> original_to_sorted(sorted_to_original.size());
    for (model::Index i : hymd::utility::IndexRange(sorted_to_original.size())) {
        original_to_sorted[sorted_to_original[i]] = i;
    }

    InitRowsPairsSet(column_matches_info[original_to_sorted.front()],
                     lhs_column_similarity_classifiers_.front().GetDecisionBoundary());

    for (std::size_t lhs_index = 1; lhs_index < lhs_column_similarity_classifiers_.size();
         ++lhs_index) {
        model::Index column_match_info_index = original_to_sorted[lhs_index];
        UpdateRowsPairsWithLhs(column_matches_info[column_match_info_index],
                               lhs_column_similarity_classifiers_[lhs_index].GetDecisionBoundary());
    }

    UpdateRowsPairsWithRhs(column_matches_info[original_to_sorted.back()],
                           rhs_column_similarity_classifier_.GetDecisionBoundary());

    holds_ = rows_pairs_.empty();
}
}  // namespace algos::md
