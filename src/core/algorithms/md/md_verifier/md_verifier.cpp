#include "algorithms/md/md_verifier/md_verifier.h"

#include <algorithm>
#include <numeric>
#include <ranges>

#include "algorithms/md/lhs_column_similarity_classifier.h"
#include "config/equal_nulls/option.h"
#include "config/exceptions.h"
#include "config/indices/option.h"
#include "config/indices/type.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::md {
MDVerifier::MDVerifier() : MdAlgorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kLeftTable, kRightTable});
}

void MDVerifier::ResetStateMd() {
    md_holds_ = true;
}

void MDVerifier::LoadDataInternal() {
    left_schema_ = std::make_shared<RelationalSchema>(left_table_->GetRelationName());
    std::size_t const left_table_cols = left_table_->GetNumberOfColumns();
    for (model::Index i : std::views::iota(model::Index(0), left_table_cols)) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }

    if (right_table_ == nullptr) {
        right_schema_ = left_schema_;

    } else {
        right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
        std::size_t const right_table_cols = right_table_->GetNumberOfColumns();
        for (model::Index i : std::views::iota(model::Index(0), right_table_cols)) {
            right_schema_->AppendColumn(right_table_->GetColumnName(i));
        }
    }
}

void MDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto not_null = [](config::InputTable const& table) {
        if (table == nullptr) throw config::ConfigurationError("Left table may not be null.");
    };

    RegisterOption(Option{&right_table_, kRightTable, kDRightTable, config::InputTable{nullptr}});
    RegisterOption(Option{&left_table_, kLeftTable, kDLeftTable}
                           .SetValueCheck(std::move(not_null))
                           .SetConditionalOpts({{{}, {kRightTable}}}));
    RegisterOption(Option{&lhs_, kMDLHS, kDMDLHS});
    RegisterOption(Option{&rhs_, kMDRHS, kDMDRHS});
}

void MDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRightTable, kLeftTable, kMDLHS, kMDRHS});
}

unsigned long long MDVerifier::ExecuteInternal() {
    using namespace std::chrono;

    auto start_time = system_clock::now();

    VerifyMD();

    return duration_cast<milliseconds>(system_clock::now() - start_time).count();
}

model::MD MDVerifier::BuildMD(std::vector<MDVerifierColumnSimilarityClassifier> const& lhs,
                              MDVerifierColumnSimilarityClassifier const& rhs) {
    std::shared_ptr<std::vector<model::md::ColumnMatch>> column_matches =
            std::make_shared<std::vector<model::md::ColumnMatch>>();
    std::transform(lhs_.begin(), lhs_.end(), std::back_inserter(*column_matches),
                   [](MDVerifierColumnSimilarityClassifier& classifier) {
                       return classifier.GetColumnMatch().ToStandardColumnMatch();
                   });
    column_matches->emplace_back(rhs_.GetColumnMatch().ToStandardColumnMatch());
    std::vector<model::md::LhsColumnSimilarityClassifier> lhs_transformed;
    model::Index column_match_current_index = 0;
    std::transform(lhs_.begin(), lhs_.end(), std::back_inserter(lhs_transformed),
                   [&column_match_current_index](MDVerifierColumnSimilarityClassifier& classifier) {
                       return model::md::LhsColumnSimilarityClassifier(
                               std::nullopt, column_match_current_index++,
                               classifier.GetDecisionBoundary());
                   });
    model::md::ColumnSimilarityClassifier rhs_transformed = model::md::ColumnSimilarityClassifier(
            column_match_current_index, rhs.GetDecisionBoundary());

    return model::MD(left_schema_, right_schema_, column_matches, lhs_transformed, rhs_transformed);
}

void MDVerifier::VerifyMD() {
    input_md_ = std::make_shared<model::MD>(BuildMD(lhs_, rhs_));
    std::vector<MDVerifierColumnMatch> column_matches;
    std::transform(lhs_.begin(), lhs_.end(), std::back_inserter(column_matches),
                   [](MDVerifierColumnSimilarityClassifier& classifier) {
                       return classifier.GetColumnMatch();
                   });
    column_matches.push_back(rhs_.GetColumnMatch());
    std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers;
    model::Index column_match_current_index = 0;
    std::transform(lhs_.begin(), lhs_.end(), std::back_inserter(lhs_column_similarity_classifiers),
                   [&column_match_current_index](MDVerifierColumnSimilarityClassifier& classifier) {
                       return model::md::ColumnSimilarityClassifier(
                               column_match_current_index++, classifier.GetDecisionBoundary());
                   });
    MDValidationCalculator validator(
            left_table_, right_table_, column_matches, lhs_column_similarity_classifiers,
            model::md::ColumnSimilarityClassifier(column_match_current_index,
                                                  rhs_.GetDecisionBoundary()));

    validator.Validate();
    md_holds_ = validator.Holds();
    true_rhs_decision_boundary_ = validator.GetTrueDecisionBoundary();
    highlights_ = validator.GetHighlights();
    MDVerifierColumnSimilarityClassifier suggested_rhs = MDVerifierColumnSimilarityClassifier(
            rhs_.GetColumnMatch(), true_rhs_decision_boundary_);
    RegisterMd(BuildMD(lhs_, suggested_rhs));
}

}  // namespace algos::md
