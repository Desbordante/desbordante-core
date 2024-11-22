#include "algorithms/md/md_verifier/md_verifier.h"

#include <numeric>

#include "config/equal_nulls/option.h"
#include "config/exceptions.h"
#include "config/indices/option.h"
#include "config/indices/type.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

using DecisionBoundary = model::md::DecisionBoundary;

namespace algos::md {
MDVerifier::MDVerifier() : Algorithm({}), pairs_violating_md_({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void ValidateDecisionBoundaries(config::IndicesType const& indices,
                                std::vector<DecisionBoundary> const& decision_boundaries) {
    assert(indices.size() == decision_boundaries.size());
    for (int i = 0; i < decision_boundaries.size(); ++i) {
        if (decision_boundaries[i] < 0.0 || decision_boundaries[i] > 1.0) {
            throw config::ConfigurationError("Decision boundaries for column with index \"" +
                                             std::to_string(i) + "\" out of range.");
        }
    }
}

void MDVerifier::ResetState() {
    pairs_violating_md_ = {};
    md_holds_ = false;
}

void MDVerifier::LoadDataInternal() {
    relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, is_null_equal_null_);
    input_table_->Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: MD verifying is meaningless.");
    }
}

void MDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_columns = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(Option{&dist_from_null_is_infinity_, kDistFromNullIsInfinity,
                          kDDistFromNullIsInfinity, false});
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_columns));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_schema_columns));
}

void MDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({
            kDistFromNullIsInfinity,
            kEqualNulls,
            config::kLhsIndicesOpt.GetName(),
            config::kRhsIndicesOpt.GetName(),
    });
}

unsigned long long MDVerifier::ExecuteInternal() {
    using namespace std::chrono;

    auto start_time = system_clock::now();

    VerifyMD();

    return duration_cast<milliseconds>(system_clock::now() - start_time).count();
}

void MDVerifier::VerifyMD() {
    assert(lhs_indices_.size() == lhs_desicion_bondaries_.size() &&
           lhs_indices_.size() == lhs_similarity_measures_.size());
    assert(rhs_indices_.size() == rhs_desicion_bondaries_.size() &&
           rhs_indices_.size() == rhs_similarity_measures_.size());

    std::vector<DecisionBoundary> lhs_suggestion_boundaries = lhs_desicion_bondaries_;
    std::vector<DecisionBoundary> rhs_suggestion_boundaries = rhs_desicion_bondaries_;

    auto num_cols = relation_->GetNumRows();

    for (size_t first_row = 0; first_row < num_cols; ++first_row) {
        for (size_t second_row = first_row + 1; second_row < num_cols; ++second_row) {
        }
    }
}

}  // namespace algos::md