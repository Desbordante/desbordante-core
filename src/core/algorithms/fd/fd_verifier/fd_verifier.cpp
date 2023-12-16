#include "algorithms/fd/fd_verifier/fd_verifier.h"

#include <chrono>
#include <memory>
#include <stdexcept>

#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/indices/validate_index.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::fd_verifier {

FDVerifier::FDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::TableOpt.GetName(), config::EqualNullsOpt.GetName()});
}

void FDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::TableOpt(&input_table_));
    RegisterOption(config::EqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::LhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterOption(config::RhsIndicesOpt(&rhs_indices_, get_schema_cols));
}

void FDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({config::LhsIndicesOpt.GetName(), config::RhsIndicesOpt.GetName()});
}

void FDVerifier::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);
    input_table_->Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD verifying is meaningless.");
    }
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, is_null_equal_null_);
}

unsigned long long FDVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    stats_calculator_ = std::make_unique<StatsCalculator>(relation_, typed_relation_, lhs_indices_,
                                                          rhs_indices_);

    VerifyFD();
    SortHighlightsByProportionDescending();
    stats_calculator_->PrintStatistics();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void FDVerifier::VerifyFD() const {
    std::shared_ptr<model::PLI const> lhs_pli = CalculatePLI(lhs_indices_);
    std::shared_ptr<model::PLI const> rhs_pli = CalculatePLI(rhs_indices_);

    std::unique_ptr<model::PLI const> intersection_pli = lhs_pli->Intersect(rhs_pli.get());
    if (lhs_pli->GetNumCluster() == intersection_pli->GetNumCluster()) {
        return;
    }

    stats_calculator_->CalculateStatistics(lhs_pli.get(), rhs_pli.get());
}

std::shared_ptr<model::PLI const> FDVerifier::CalculatePLI(
        config::IndicesType const& indices) const {
    std::shared_ptr<model::PLI const> pli = relation_->GetColumnData(indices[0]).GetPliOwnership();

    for (size_t i = 1; i < indices.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(indices[i]).GetPositionListIndex());
    }
    return pli;
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
