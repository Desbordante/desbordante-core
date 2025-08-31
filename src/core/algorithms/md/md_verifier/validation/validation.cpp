#include "algorithms/md/md_verifier/validation/validation.h"

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/index_range.h"
#include "algorithms/md/md_verifier/validation/records_pairs.h"
#include "boost/hof/first_of.hpp"
#include "util/worker_thread_pool.h"

namespace algos::md {
ColumnInfoView MDValidationCalculator::GetColumnInfo(
        hymd::ColumnMatchInfo const& column_match_info) {
    return {records_info_->GetLeftCompressor()
                    .GetPli(column_match_info.left_column_index)
                    .GetClusters(),
            records_info_->GetRightCompressor()
                    .GetPli(column_match_info.right_column_index)
                    .GetClusters(),
            column_match_info.similarity_info.similarity_matrix};
}

void MDValidationCalculator::ProcessSimilarityMatrix(
        hymd::ColumnMatchInfo const& column_match_info,
        model::md::DecisionBoundary decision_boundary,
        std::function<void(hymd::indexes::PliCluster, hymd::indexes::PliCluster)>&& lam) {
    auto const& [left_clusters, right_clusters, similarity_matrix] =
            GetColumnInfo(column_match_info);
    for (model::Index left_cluster_index : hymd::utility::IndexRange(similarity_matrix.size())) {
        for (auto const [right_cluster_index, similarity_index] :
             similarity_matrix[left_cluster_index]) {
            model::md::Similarity similarity =
                    column_match_info.similarity_info.classifier_values[similarity_index];
            if (similarity >= decision_boundary) {
                lam(left_clusters[left_cluster_index], right_clusters[right_cluster_index]);
            }
        }
    }
}

void MDValidationCalculator::InitRecords(hymd::ColumnMatchInfo const& column_match_info,
                                         model::md::DecisionBoundary decision_boundary) {
    auto InsertClusters = [this](hymd::indexes::PliCluster left_cluster,
                                 hymd::indexes::PliCluster right_cluster) {
        violating_records_.InsertClusters(left_cluster, right_cluster);
    };
    ProcessSimilarityMatrix(column_match_info, decision_boundary, InsertClusters);
}

void MDValidationCalculator::UpdateRecordsWithLhs(hymd::ColumnMatchInfo const& column_match_info,
                                                  model::md::DecisionBoundary decision_boundary) {
    IntersectionBuilder intersection_builder(violating_records_);

    auto AddIntersection = [&intersection_builder](hymd::indexes::PliCluster left_cluster,
                                                   hymd::indexes::PliCluster right_cluster) {
        intersection_builder.AddIntersection(left_cluster, right_cluster);
    };

    ProcessSimilarityMatrix(column_match_info, decision_boundary, AddIntersection);

    violating_records_ = intersection_builder.Build();
}

// void MDValidationCalculator::InitRecords(hymd::ColumnMatchInfo const& column_match_info,
//                                          model::md::DecisionBoundary decision_boundary) {
//     auto const& [left_clusters, right_clusters, similarity_matrix] =
//             GetColumnInfo(column_match_info);
//     for (model::Index left_cluster_index : hymd::utility::IndexRange(similarity_matrix.size())) {
//         for (auto const [right_cluster_index, similarity_index] :
//              similarity_matrix[left_cluster_index]) {
//             model::md::Similarity similarity =
//                     column_match_info.similarity_info.classifier_values[similarity_index];
//             if (similarity >= decision_boundary) {
//                 violating_records_.InsertClusters(left_clusters[left_cluster_index],
//                                                    right_clusters[right_cluster_index]);
//             }
//         }
//     }
// }

// void MDValidationCalculator::UpdateRecordsWithLhs(hymd::ColumnMatchInfo const& column_match_info,
//                                                   model::md::DecisionBoundary decision_boundary)
//                                                   {
//     IntersectionBuilder intersection_builder(violating_records_);

//     auto const& [left_clusters, right_clusters, similarity_matrix] =
//             GetColumnInfo(column_match_info);
//     for (model::Index left_cluster_index : hymd::utility::IndexRange(similarity_matrix.size())) {
//         for (auto const [right_cluster_index, similarity_index] :
//              similarity_matrix[left_cluster_index]) {
//             model::md::Similarity similarity =
//                     column_match_info.similarity_info.classifier_values[similarity_index];
//             if (similarity >= decision_boundary) {
//                 intersection_builder.AddIntersection(left_clusters[left_cluster_index],
//                                                      right_clusters[right_cluster_index]);
//             }
//         }
//     }

//     violating_records_ = intersection_builder.Build();
// }

void MDValidationCalculator::UpdateRecordsWithTrivialLhs(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary) {
    if (similarity >= decision_boundary) {
        return;
    }

    violating_records_.Clear();
}

void MDValidationCalculator::UpdateRecordsWithRhs(hymd::ColumnMatchInfo const& column_match_info,
                                                  model::md::DecisionBoundary decision_boundary) {
    auto const& [left_clusters, right_clusters, similarity_matrix] =
            GetColumnInfo(column_match_info);
    for (model::Index left_cluster_index : hymd::utility::IndexRange(left_clusters.size())) {
        for (model::Index right_cluster_index : hymd::utility::IndexRange(right_clusters.size())) {
            model::md::Similarity similarity = 0.0;
            hymd::indexes::SimilarityMatrixRow const& sim_map =
                    similarity_matrix[left_cluster_index];
            if (auto it = sim_map.find(right_cluster_index); it != sim_map.end()) {
                model::Index similarity_index = it->second;
                similarity = column_match_info.similarity_info.classifier_values[similarity_index];
            }

            if (similarity >= decision_boundary) {
                violating_records_.DeleteClusters(left_clusters[left_cluster_index],
                                                  right_clusters[right_cluster_index]);
            } else {
                UpdateRhsSimilarities(left_clusters[left_cluster_index],
                                      right_clusters[right_cluster_index], similarity);
            }
        }
    }
}

void MDValidationCalculator::UpdateRecordsWithTrivialRhs(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary) {
    if (similarity >= decision_boundary) {
        violating_records_.Clear();
        return;
    }

    // Need to do a lot of highlights...
    for (model::Index left_record :
         hymd::utility::IndexRange(records_info_->GetLeftCompressor().GetNumberOfRecords())) {
        for (model::Index right_record :
             hymd::utility::IndexRange(records_info_->GetRightCompressor().GetNumberOfRecords())) {
            rhs_records_pair_to_similarity_[{left_record, right_record}] = similarity;
        }
    }

    true_rhs_decision_boundary_ = similarity;

    violating_records_.Fill(records_info_->GetLeftCompressor().GetNumberOfRecords(),
                            records_info_->GetRightCompressor().GetNumberOfRecords());
}

void MDValidationCalculator::UpdateRhsSimilarities(hymd::indexes::PliCluster const& left_cluster,
                                                   hymd::indexes::PliCluster const& right_cluster,
                                                   model::md::Similarity rhs_similarity) {
    for (hymd::RecordIdentifier left_record : left_cluster) {
        for (hymd::RecordIdentifier right_record : right_cluster) {
            auto it = violating_records_.GetPairs().find(left_record);
            if (it != violating_records_.GetPairs().end() &&
                it->second.find(right_record) != it->second.end()) {
                true_rhs_decision_boundary_ = std::min(true_rhs_decision_boundary_, rhs_similarity);
                rhs_records_pair_to_similarity_[{left_record, right_record}] = rhs_similarity;
            }
        }
    }
}

bool MDValidationCalculator::TryValidateOrPrepare(
        std::vector<OneOfColumnMatchInfo> const& column_matches_similarity_infos) {
    auto it = std::ranges::find_if(
            lhs_column_similarity_classifiers_,
            [&column_matches_similarity_infos](model::md::ColumnSimilarityClassifier const& clf) {
                return std::holds_alternative<hymd::ColumnMatchInfo>(
                        column_matches_similarity_infos[clf.GetColumnMatchIndex()]);
            });

    bool all_lhs_trivial = it == lhs_column_similarity_classifiers_.end();

    if (all_lhs_trivial) {
        ValidateAllLhsTrivial(column_matches_similarity_infos);
        return true;
    }

    std::iter_swap(lhs_column_similarity_classifiers_.begin(), it);
    return false;
}

void MDValidationCalculator::ValidateAllLhsTrivial(
        std::vector<OneOfColumnMatchInfo> const& column_matches_similarity_infos) {
    bool any_lhs_fails = std::ranges::any_of(
            lhs_column_similarity_classifiers_,
            [&column_matches_similarity_infos](model::md::ColumnSimilarityClassifier const& clf) {
                OneOfColumnMatchInfo const& column_match_info =
                        column_matches_similarity_infos[clf.GetColumnMatchIndex()];
                assert(std::holds_alternative<TrivialColumnMatchInfo>(column_match_info));
                model::md::Similarity similarity =
                        std::get<TrivialColumnMatchInfo>(column_match_info);
                return similarity < clf.GetDecisionBoundary();
            });
    if (any_lhs_fails) {
        violating_records_.Clear();
        return;
    }

    OneOfColumnMatchInfo const& rhs_column_match_info =
            column_matches_similarity_infos[rhs_column_similarity_classifier_
                                                    .GetColumnMatchIndex()];
    model::md::DecisionBoundary rhs_decision_boundary =
            rhs_column_similarity_classifier_.GetDecisionBoundary();
    std::visit(boost::hof::first_of(
                       [&](hymd::ColumnMatchInfo const& column_match_info) {
                           violating_records_.Fill(
                                   records_info_->GetLeftCompressor().GetNumberOfRecords(),
                                   records_info_->GetRightCompressor().GetNumberOfRecords());
                           UpdateRecordsWithRhs(column_match_info, rhs_decision_boundary);
                       },
                       [&](TrivialColumnMatchInfo const& similarity) {
                           UpdateRecordsWithTrivialRhs(similarity, rhs_decision_boundary);
                       }),
               rhs_column_match_info);
}

void MDValidationCalculator::Validate(util::WorkerThreadPool* thread_pool) {
    hymd::SimilarityData similarity_data =
            hymd::SimilarityData::CreateFrom(records_info_.get(), column_matches_, thread_pool)
                    .first;
    std::vector<hymd::ColumnMatchInfo> const& column_matches_info =
            similarity_data.GetColumnMatchesInfo();
    std::vector<std::pair<model::md::DecisionBoundary, model::Index>> const& trivial_info =
            similarity_data.GetTrivialInfo();

    std::vector<OneOfColumnMatchInfo> column_matches_similarity_infos(column_matches_.size());
    std::vector<model::Index> const& sorted_to_original = similarity_data.GetIndexMapping();
    for (model::Index sorted_index : hymd::utility::IndexRange(sorted_to_original.size())) {
        model::Index original_index = sorted_to_original[sorted_index];

        column_matches_similarity_infos[original_index] = column_matches_info[sorted_index];
    }

    for (auto [decision_boundary, original_index] : trivial_info) {
        column_matches_similarity_infos[original_index] = decision_boundary;
    }

    if (TryValidateOrPrepare(column_matches_similarity_infos)) {
        holds_ = violating_records_.Empty();
        return;
    }

    InitRecords(std::get<hymd::ColumnMatchInfo>(column_matches_similarity_infos.front()),
                lhs_column_similarity_classifiers_.front().GetDecisionBoundary());

    for (std::size_t lhs_index = 1; lhs_index < lhs_column_similarity_classifiers_.size();
         ++lhs_index) {
        model::md::ColumnSimilarityClassifier const& clf =
                lhs_column_similarity_classifiers_[lhs_index];
        OneOfColumnMatchInfo const& column_match_info =
                column_matches_similarity_infos[clf.GetColumnMatchIndex()];
        std::visit(boost::hof::first_of(
                           [&](hymd::ColumnMatchInfo const& column_match_info) {
                               UpdateRecordsWithLhs(column_match_info, clf.GetDecisionBoundary());
                           },
                           [&](TrivialColumnMatchInfo similarity) {
                               UpdateRecordsWithTrivialLhs(similarity, clf.GetDecisionBoundary());
                           }),
                   column_match_info);
    }

    OneOfColumnMatchInfo const& rhs_column_match_info =
            column_matches_similarity_infos[rhs_column_similarity_classifier_
                                                    .GetColumnMatchIndex()];

    model::md::DecisionBoundary rhs_decision_boundary =
            rhs_column_similarity_classifier_.GetDecisionBoundary();
    std::visit(boost::hof::first_of(
                       [&](hymd::ColumnMatchInfo const& column_match_info) {
                           UpdateRecordsWithRhs(column_match_info, rhs_decision_boundary);
                       },
                       [&](TrivialColumnMatchInfo similarity) {
                           UpdateRecordsWithTrivialRhs(similarity, rhs_decision_boundary);
                       }),
               rhs_column_match_info);

    holds_ = violating_records_.Empty();
}
}  // namespace algos::md
