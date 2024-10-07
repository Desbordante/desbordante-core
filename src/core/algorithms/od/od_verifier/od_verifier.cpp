#include "od_verifier.h"

#include "ascending_od/option.h"
#include "config/equal_nulls/option.h"
#include "config/indices/od_context.h"
#include "config/indices/option.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::od_verifier {

ODVerifier::ODVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void ODVerifier::RegisterOptions() {
    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    IndicesType lhs_indices_, rhs_indices_;
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_schema_cols));
    RegisterOption(config::kODContextOpt(&context_indices_));
    RegisterOption(config::kAscendingODOpt(&ascending_));
    lhs_indicex_ = lhs_indices_[0];
    rhs_indicex_ = rhs_indices_[0];
}

void ODVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName(),
                          config::kODContextOpt.GetName(), config::kAscendingODOpt.GetName()});
}

void ODVerifier::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: OD verifying is meaningless.");
    }
    input_table_->Reset();
    data_ = std::make_shared<DataFrame>(DataFrame::FromInputTable(input_table_));
    if (data_->GetColumnCount() == 0) {
        throw std::runtime_error("Got an empty dataset: OD verifying is meaningless.");
    }
}

unsigned long long ODVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    if (ascending_) {
        VerifyOD<true>();
    } else {
        VerifyOD<false>();
    }
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

// checks whether the OD has broken
bool ODVerifier::ODHolds() const {
    return row_violate_ods_by_swap_.empty() && row_violate_ods_by_split_.empty();
}

// Returns the number of rows that violate the OD by split
size_t ODVerifier::GetNumRowsViolateBySplit() const {
    return row_violate_ods_by_split_.size();
}

// Returns the number of rows that violate the OD by swap
size_t ODVerifier::GetNumRowsViolateBySwap() const {
    return row_violate_ods_by_swap_.size();
}

}  // namespace algos::od_verifier
