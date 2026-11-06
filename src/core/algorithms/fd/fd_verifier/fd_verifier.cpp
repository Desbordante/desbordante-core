#include "core/algorithms/fd/fd_verifier/fd_verifier.h"

#include <memory>
#include <numeric>
#include <stdexcept>

#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/config/equal_nulls/option.h"
#include "core/config/error_measure/option.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos::fd_verifier {

FDVerifier::FDVerifier() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void FDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_schema_cols));
    RegisterOption(config::kAfdErrorMeasureOpt(&afd_error_measure_));
}

void FDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName(),
                          config::kAfdErrorMeasureOpt.GetName()});
}

void FDVerifier::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_);
    input_table_->Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD verifying is meaningless.");
    }
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, is_null_equal_null_);
}

void FDVerifier::ExecuteInternal() {
    stats_calculator_ = std::make_unique<StatsCalculator>(relation_, typed_relation_, lhs_indices_,
                                                          rhs_indices_);

    VerifyFD();
    SortHighlightsByProportionDescending();
    stats_calculator_->PrintStatistics();
}

void FDVerifier::VerifyFD() const {
    std::shared_ptr<model::PLI const> lhs_pli = relation_->CalculatePLI(lhs_indices_);
    std::shared_ptr<model::PLI const> rhs_pli = relation_->CalculatePLI(rhs_indices_);

    std::unique_ptr<model::PLI const> intersection_pli = lhs_pli->Intersect(rhs_pli.get());
    if (lhs_pli->GetNumCluster() == intersection_pli->GetNumCluster()) {
        return;
    }

    stats_calculator_->CalculateStatistics(
            lhs_pli.get(), rhs_pli.get(),
            CalculateError(lhs_pli.get(), rhs_pli.get(), intersection_pli.get()));
}

long double FDVerifier::CalculateError(model::PLI const* lhs, model::PLI const* rhs,
                                       model::PLI const* joint) const {
    using Calculator = afd_metric_calculator::AFDMetricCalculator;
    auto const num_rows = relation_->GetNumRows();
    switch (afd_error_measure_) {
        case AfdErrorMeasure::kG1:
            return Calculator::CalculateG1Error(lhs, joint, relation_->GetNumTuplePairs());
        case AfdErrorMeasure::kG2:
            return Calculator::CalculateG2Error(lhs, rhs, num_rows);
        case AfdErrorMeasure::kG3:
            return 1.L - Calculator::CalculateG3(lhs, rhs, num_rows);
        case AfdErrorMeasure::kG1Prime: {
            config::IndicesType indices(relation_->GetNumColumns());
            std::iota(indices.begin(), indices.end(), 0);
            auto full_pli = relation_->CalculatePLI(indices);
            return Calculator::CalculateG1PrimeError(
                    lhs, joint, relation_->GetNumTuplePairs() - full_pli->GetNepAsLong());
        }
        case AfdErrorMeasure::kG3Prime:
            return Calculator::CalculateG3PrimeError(lhs, rhs, num_rows);
        case AfdErrorMeasure::kPdep:
            return 1.L - Calculator::CalculatePdepMeasure(lhs, rhs);
        case AfdErrorMeasure::kTau:
            return 1.L - Calculator::CalculateTau(lhs, rhs);
        case AfdErrorMeasure::kMuPlus:
            return 1.L - Calculator::CalculateMuPlus(lhs, rhs);
        case AfdErrorMeasure::kRho:
            return 1.L - Calculator::CalculateRhoMeasure(lhs, joint);
        case AfdErrorMeasure::kFi:
            return 1.L - Calculator::CalculateFI(lhs, rhs, num_rows);
        case AfdErrorMeasure::kG1S:
            return 1.L - Calculator::CalculateG1SMeasure(lhs, rhs, num_rows);
        case AfdErrorMeasure::kRfiPlus:
            return 1.L - Calculator::CalculateRfiPlusMeasure(lhs, rhs);
        case AfdErrorMeasure::kRfiPrimePlus:
            return 1.L - Calculator::CalculateRfiPrimePlusMeasure(lhs, rhs);
        case AfdErrorMeasure::kSfi:
            return 1.L - Calculator::CalculateSfiMeasure(lhs, rhs);
    }
    throw std::logic_error("Unknown AFD error measure");
}

void FDVerifier::SortHighlightsByProportionAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByProportionAscending());
}

void FDVerifier::SortHighlightsByProportionDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByProportionDescending());
}

void FDVerifier::SortHighlightsByNumAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByNumAscending());
}

void FDVerifier::SortHighlightsByNumDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByNumDescending());
}

void FDVerifier::SortHighlightsBySizeAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsBySizeAscending());
}

void FDVerifier::SortHighlightsBySizeDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsBySizeDescending());
}

void FDVerifier::SortHighlightsByLhsAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(stats_calculator_->CompareHighlightsByLhsAscending());
}

void FDVerifier::SortHighlightsByLhsDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(stats_calculator_->CompareHighlightsByLhsDescending());
}

}  // namespace algos::fd_verifier
