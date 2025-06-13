#pragma once

#include <vector>

#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/md_verifier/cmptr.h"
#include "algorithms/md/md_verifier/validation/rows_pairs.h"
#include "algorithms/md/similarity.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::md {
struct ColumnInfoView {
    std::vector<hymd::indexes::PliCluster> const& left_clusters;
    std::vector<hymd::indexes::PliCluster> const& right_clusters;
    hymd::indexes::SimilarityMatrix const& similarity_matrix;
};

class MDValidationCalculator {
private:
    std::unique_ptr<hymd::indexes::RecordsInfo> records_info_;

    std::vector<CMPtr> column_matches_;
    std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers_;
    model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier_;

    bool holds_;

    model::md::DecisionBoundary true_rhs_decision_boundary_;

    RowsPairSet rows_pairs_;
    RowsToSimilarityMap rhs_pair_to_similarity_;

    ColumnInfoView GetColumnInfo(hymd::ColumnMatchInfo const& column_match_info);

    void InsertClustersInRowsPairsSet(hymd::indexes::PliCluster const& left_cluster,
                                      hymd::indexes::PliCluster const& right_cluster);

    void DeleteClustersFromRowsPairsSet(hymd::indexes::PliCluster const& left_cluster,
                                        hymd::indexes::PliCluster const& right_cluster);

    void UpdateNewRowsPairsSet(hymd::indexes::PliCluster const& left_cluster,
                               hymd::indexes::PliCluster const& right_cluster,
                               RowsPairSet& new_rows_pairs);

    void InitRowsPairsSet(hymd::ColumnMatchInfo const& column_match_info,
                          model::md::DecisionBoundary decision_boundary);

    void UpdateRowsPairsWithLhs(hymd::ColumnMatchInfo const& column_match_info,
                                model::md::DecisionBoundary decision_boundary);

    void UpdateRowsPairsWithRhs(hymd::ColumnMatchInfo const& column_match_info,
                                model::md::DecisionBoundary decision_boundary);

    void UpdateRhsSimilarities(hymd::indexes::PliCluster const& left_cluster,
                               hymd::indexes::PliCluster const& right_cluster,
                               model::md::Similarity rhs_similarity);

public:
    MDValidationCalculator(
            config::InputTable left_table, config::InputTable right_table,
            std::vector<CMPtr> column_matches,
            std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers,
            model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier)
        : column_matches_(column_matches),
          lhs_column_similarity_classifiers_(lhs_column_similarity_classifiers),
          rhs_column_similarity_classifier_(rhs_column_similarity_classifier),
          holds_(true),
          true_rhs_decision_boundary_(rhs_column_similarity_classifier.GetDecisionBoundary()) {
        if (right_table == nullptr) {
            records_info_ = hymd::indexes::RecordsInfo::CreateFrom(*left_table);
        } else {
            records_info_ = hymd::indexes::RecordsInfo::CreateFrom(*left_table, *right_table);
        }
    }

    void Validate(util::WorkerThreadPool* thread_pool);

    bool Holds() const {
        return holds_;
    }

    model::md::DecisionBoundary GetTrueRhsDecisionBoundary() const {
        return true_rhs_decision_boundary_;
    }

    RowsPairSet const& GetRowsPairs() const {
        return rows_pairs_;
    }

    RowsToSimilarityMap const& GetRhsPairToSimilarityMapping() const {
        return rhs_pair_to_similarity_;
    }
};
}  // namespace algos::md
