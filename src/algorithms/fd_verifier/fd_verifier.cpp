#include "algorithms/fd_verifier/fd_verifier.h"

#include <chrono>
#include <memory>
#include <stdexcept>

#include "algorithms/options/equal_nulls/option.h"
#include "algorithms/options/indices/option.h"
#include "algorithms/options/indices/validate_index.h"
#include "algorithms/options/names_and_descriptions.h"
#include "algorithms/options/new_options_shorthands.h"

namespace algos::fd_verifier {

FDVerifier::FDVerifier() : Primitive({}) {
    RegisterOptions();
}

void FDVerifier::RegisterOptions() {
    NEW_OPTIONS_SHORTHANDS

    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };
    auto check_rhs = [this](config::IndexType rhs_index) {
        config::ValidateIndex(rhs_index, relation_->GetSchema()->GetNumColumns());
    };

    RegisterInitialFitOption(config::EqualNullsOpt(&is_null_equal_null_));
    RegisterInitialExecuteOption(config::LhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterInitialExecuteOption(
            Option{&rhs_index_, kRhsIndex, kDRhsIndex}.SetValueCheck(check_rhs));
}

void FDVerifier::FitInternal(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);
    data_stream.Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD verifying is meaningless.");
    }
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(data_stream, is_null_equal_null_);
}

unsigned long long FDVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    stats_calculator_ =
            std::make_unique<StatsCalculator>(relation_, typed_relation_, lhs_indices_, rhs_index_);

    VerifyFD();
    SortHighlightsByProportionDescending();
    stats_calculator_->PrintStatistics();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void FDVerifier::VerifyFD() const {
    std::shared_ptr<util::PLI const> pli =
            relation_->GetColumnData(lhs_indices_[0]).GetPliOwnership();

    for (size_t i = 1; i < lhs_indices_.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(lhs_indices_[i]).GetPositionListIndex());
    }

    std::unique_ptr<util::PLI const> intersection_pli =
            pli->Intersect(relation_->GetColumnData(rhs_index_).GetPositionListIndex());

    if (pli->GetNumCluster() == intersection_pli->GetNumCluster()) {
        return;
    }

    auto& lhs_clusters = pli->GetIndex();
    stats_calculator_->CalculateStatistics(std::move(lhs_clusters));
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
