#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "core/algorithms/md/column_match.h"
#include "core/algorithms/md/column_similarity_classifier.h"
#include "core/algorithms/md/decision_boundary.h"
#include "core/algorithms/md/hymd/column_match_info.h"
#include "core/algorithms/md/hymd/similarity_data.h"
#include "core/algorithms/md/md_verifier/cmptr.h"
#include "core/algorithms/md/md_verifier/highlights/highlights.h"
#include "core/algorithms/md/similarity.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::md {

using TrivialColumnMatchInfo = model::md::DecisionBoundary;

using OneOfColumnMatchInfo = std::variant<hymd::ColumnMatchInfo, TrivialColumnMatchInfo>;

class MDValidationCalculator {
private:
    std::unique_ptr<hymd::indexes::RecordsInfo> records_info_;

    // Column Matches and Column Similarity Classifiers for both rhs and lhs are being stored in the
    // same vector. Last elements of such vectors refers to rhs, others to lhs
    std::vector<CMPtr> column_matches_;
    std::vector<bool> non_informative_lhs_classifiers_;  // Indicates whenever lhs classifier is non
                                                         // informative
    std::vector<model::md::ColumnSimilarityClassifier> column_similarity_classifiers_;
    std::vector<OneOfColumnMatchInfo> column_matches_similarity_infos_;

    model::Index starting_lhs_classifier_index_ = 0;

    bool holds_ = true;
    bool validation_finished_ = false;

    model::md::DecisionBoundary true_rhs_decision_boundary_;
    std::shared_ptr<MDHighlights> highlights_;

    void SelectStartingLhsClassifierIndex();

    void ExecuteValidationFrom(model::Index lhs_classifier_index);

    void ValidateMdConstraint(hymd::RecordIdentifier left_record_id,
                              hymd::RecordIdentifier right_record_id);
    void ValidateRhsForRecordsPair(hymd::RecordIdentifier left_record_id,
                                   hymd::RecordIdentifier right_record_id);

    bool MatchedByClassifier(hymd::RecordIdentifier left_record_id,
                             hymd::RecordIdentifier right_record_id, model::Index classifier_index,
                             auto&& on_lesser_boundary, auto&& on_greater_boundary);

    bool MatchedByLhsClassifier(hymd::RecordIdentifier left_record_id,
                                hymd::RecordIdentifier right_record_id,
                                model::Index lhs_classifier_index);

    bool MatchedByRhsClassifier(hymd::RecordIdentifier left_record_id,
                                hymd::RecordIdentifier right_record_id);

    void CreateColumnMatchesSimilarityInfos(hymd::SimilarityData const& similarity_data);

    model::md::Similarity GetRecordsPairSimilarity(hymd::RecordIdentifier left_record_id,
                                                   hymd::RecordIdentifier right_record_id,
                                                   OneOfColumnMatchInfo column_match_info);

public:
    MDValidationCalculator(
            config::InputTable const& left_table, config::InputTable const& right_table,
            std::vector<CMPtr> const& column_matches,
            std::vector<model::md::ColumnSimilarityClassifier> const& column_similarity_classifiers,
            std::shared_ptr<MDHighlights> const& highlights)
        : column_matches_(std::move(column_matches)),
          non_informative_lhs_classifiers_(column_similarity_classifiers.size() - 1, false),
          column_similarity_classifiers_(std::move(column_similarity_classifiers)),
          true_rhs_decision_boundary_(column_similarity_classifiers.back().GetDecisionBoundary()),
          highlights_(highlights) {
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
};
}  // namespace algos::md
