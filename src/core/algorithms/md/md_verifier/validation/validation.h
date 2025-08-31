#pragma once

#include <functional>
#include <variant>
#include <vector>

#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/md_verifier/cmptr.h"
#include "algorithms/md/md_verifier/validation/records_pairs.h"
#include "algorithms/md/similarity.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::md {
struct ColumnInfoView {
    std::vector<hymd::indexes::PliCluster> const& left_clusters;
    std::vector<hymd::indexes::PliCluster> const& right_clusters;
    hymd::indexes::SimilarityMatrix const& similarity_matrix;
};

using TrivialColumnMatchInfo = model::md::DecisionBoundary;

using OneOfColumnMatchInfo = std::variant<hymd::ColumnMatchInfo, TrivialColumnMatchInfo>;

class MDValidationCalculator {
private:
    std::unique_ptr<hymd::indexes::RecordsInfo> records_info_;

    std::vector<CMPtr> column_matches_;
    std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers_;
    model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier_;

    bool holds_;

    model::md::DecisionBoundary true_rhs_decision_boundary_;

    ViolatingRecordsSet violating_records_;
    RecordsPairToSimilarityMap rhs_records_pair_to_similarity_;

    ColumnInfoView GetColumnInfo(hymd::ColumnMatchInfo const& column_match_info);

    void ProcessSimilarityMatrix(
            hymd::ColumnMatchInfo const& column_match_info,
            model::md::DecisionBoundary decision_boundary,
            std::function<void(hymd::indexes::PliCluster, hymd::indexes::PliCluster)>&& lam);

    void InitRecords(hymd::ColumnMatchInfo const& column_match_info,
                     model::md::DecisionBoundary decision_boundary);

    void UpdateRecordsWithLhs(hymd::ColumnMatchInfo const& column_match_info,
                              model::md::DecisionBoundary decision_boundary);
    void UpdateRecordsWithTrivialLhs(model::md::Similarity similarity,
                                     model::md::DecisionBoundary decision_boundary);

    void UpdateRecordsWithRhs(hymd::ColumnMatchInfo const& column_match_info,
                              model::md::DecisionBoundary decision_boundary);
    void UpdateRecordsWithTrivialRhs(model::md::Similarity similarity,
                                     model::md::DecisionBoundary decision_boundary);

    void UpdateRhsSimilarities(hymd::indexes::PliCluster const& left_cluster,
                               hymd::indexes::PliCluster const& right_cluster,
                               model::md::Similarity rhs_similarity);

    bool TryValidateOrPrepare(
            std::vector<OneOfColumnMatchInfo> const& column_matches_similarity_infos);

    void ValidateAllLhsTrivial(
            std::vector<OneOfColumnMatchInfo> const& column_matches_similarity_infos);

public:
    MDValidationCalculator(
            config::InputTable const& left_table, config::InputTable const& right_table,
            std::vector<CMPtr> column_matches,
            std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers,
            model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier)
        : column_matches_(std::move(column_matches)),
          lhs_column_similarity_classifiers_(std::move(lhs_column_similarity_classifiers)),
          rhs_column_similarity_classifier_(std::move(rhs_column_similarity_classifier)),
          holds_(true),
          true_rhs_decision_boundary_(rhs_column_similarity_classifier_.GetDecisionBoundary()) {
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

    RecordsPairsSet const& GetViolatingRecordsPairs() const {
        return violating_records_.GetPairs();
    }

    RecordsPairToSimilarityMap const& GetRhsPairsToSimilarityMapping() const {
        return rhs_records_pair_to_similarity_;
    }
};
}  // namespace algos::md
