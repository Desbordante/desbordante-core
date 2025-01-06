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
MDVerifier::MDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void MDVerifier::ResetState() {
    highlights.Reset();
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
    RegisterOption(
            Option{&lhs_desicion_bondaries_, kMDLhsDecisionBoundaries, kDMDLhsDecisionBoundaries});
    RegisterOption(
            Option{&rhs_desicion_bondaries_, kMDRhsDecisionBoundaries, kDMDRhsDecisionBoundaries});
    RegisterOption(
            Option{&lhs_similarity_measures_, kMDLhsSimilarityMeasures, kDMDLhsSimilarityMeasures});
    RegisterOption(
            Option{&rhs_similarity_measures_, kMDRhsSimilarityMeasures, kDMDRhsSimilarityMeasures});
}

void MDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({
            kEqualNulls,
            kDistFromNullIsInfinity,
            config::kLhsIndicesOpt.GetName(),
            config::kRhsIndicesOpt.GetName(),
            kMDLhsDecisionBoundaries,
            kMDRhsDecisionBoundaries,
            kMDLhsSimilarityMeasures,
            kMDRhsSimilarityMeasures,
    });
}

unsigned long long MDVerifier::ExecuteInternal() {
    using namespace std::chrono;

    auto start_time = system_clock::now();

    VerifyMD();

    return duration_cast<milliseconds>(system_clock::now() - start_time).count();
}

DecisionBoundary MDVerifier::CalculateSimilarity(std::byte const* first_val,
                                                 std::byte const* second_val, model::TypeId type_id,
                                                 std::shared_ptr<SimilarityMeasure> measure) {
    using namespace model;

    switch (type_id) {
        case TypeId::kInt: {
            auto first = static_cast<DecisionBoundary>(INumericType::GetValue<Int>(first_val));
            auto second = static_cast<DecisionBoundary>(INumericType::GetValue<Int>(second_val));
            return (*AsNumericMeasure(measure))(first, second);
        }

        case TypeId::kDouble: {
            auto first = static_cast<DecisionBoundary>(INumericType::GetValue<Double>(first_val));
            auto second = static_cast<DecisionBoundary>(INumericType::GetValue<Double>(first_val));
            return (*AsNumericMeasure(measure))(first, second);
        }

        case TypeId::kString: {
            auto string_type = StringType();
            auto first = string_type.ValueToString(first_val);
            auto second = string_type.ValueToString(second_val);
            return (*AsStringMeasure(measure))(first, second);
        }

        default:
            throw std::runtime_error(
                    "Failed to calcutate similarity measure: unsupported column type provided.");
    }
}

bool MDVerifier::CheckRows(size_t first_row, size_t second_row) {
    for (std::size_t i = 0; i < lhs_indices_.size(); ++i) {
        auto index = lhs_indices_[i];
        auto decision_boundary = lhs_desicion_bondaries_[i];
        auto similarity_measure = lhs_similarity_measures_[i];

        auto const& column = relation_->GetColumnData(index);
        auto similarity =
                CalculateSimilarity(column.GetValue(first_row), column.GetValue(second_row),
                                    column.GetTypeId(), similarity_measure);
        if (similarity < decision_boundary) {
            return true;
        }
    }
    bool holds_for_rhs = true;
    for (std::size_t i = 0; i < rhs_indices_.size(); ++i) {
        auto index = rhs_indices_[i];
        auto decision_boundary = rhs_desicion_bondaries_[i];
        auto similarity_measure = rhs_similarity_measures_[i];

        auto const& column = relation_->GetColumnData(index);
        auto similarity =
                CalculateSimilarity(column.GetValue(first_row), column.GetValue(second_row),
                                    column.GetTypeId(), similarity_measure);
        if (similarity < decision_boundary) {
            holds_for_rhs = false;
            highlights.AddHighlight({std::make_pair(first_row, second_row), index,
                                     column.GetDataAsString(first_row),
                                     column.GetDataAsString(second_row), similarity,
                                     decision_boundary});
            rhs_suggestion_boundaries_[i] = std::min(rhs_suggestion_boundaries_[i], similarity);
        }
    }
    return holds_for_rhs;
}

void MDVerifier::VerifyMD() {
    if (lhs_indices_.size() != lhs_desicion_bondaries_.size() ||
        lhs_indices_.size() != lhs_similarity_measures_.size()) {
        throw std::length_error(
                "Length of LHS Indices doesn't match length of LHS Decision Boundaries or/and LHS "
                "Similarity Measures.");
    }

    if (rhs_indices_.size() != rhs_desicion_bondaries_.size() ||
        rhs_indices_.size() != rhs_similarity_measures_.size()) {
        throw std::length_error(
                "Length of RHS Indices doesn't match length of RHS Decision Boundaries or/and RHS "
                "Similarity Measures.");
    }

    md_holds_ = true;

    rhs_suggestion_boundaries_ = rhs_desicion_bondaries_;

    auto num_cols = relation_->GetNumRows();

    for (size_t first_row = 0; first_row < num_cols; ++first_row) {
        for (size_t second_row = first_row + 1; second_row < num_cols; ++second_row) {
            if (!CheckRows(first_row, second_row)) {
                md_holds_ = false;
            }
        }
    }
}

}  // namespace algos::md