#include "algorithms/md/md_verifier/md_verifier.h"

#include <algorithm>
#include <numeric>

#include "config/equal_nulls/option.h"
#include "config/exceptions.h"
#include "config/indices/option.h"
#include "config/indices/type.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::md {
MDVerifier::MDVerifier() : Algorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kLeftTable, kRightTable});
}

void MDVerifier::ResetState() {
    md_holds_ = true;
}

void MDVerifier::LoadDataInternal() {}

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

void MDVerifier::VerifyMD() {
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
}

}  // namespace algos::md
