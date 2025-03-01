#include "ucc_verifier.h"

#include <bits/chrono.h>  // for duration_cast
#include <functional>     // for function
#include <numeric>        // for iota
#include <stdexcept>      // for runtime_error
#include <string_view>    // for basic_string_view
#include <utility>        // for move

#include "algorithm.h"                  // for Algorithm
#include "common_option.h"              // for CommonOption
#include "config/equal_nulls/option.h"  // for kEqualNullsOpt
#include "config/indices/option.h"      // for IndicesOption
#include "config/names_and_descriptions.h"
#include "config/option_using.h"                     // for DESBORDANTE_OPTI...
#include "config/tabular_data/input_table/option.h"  // for kTableOpt
#include "indices/type.h"                            // for IndicesType
#include "table/column_data.h"                       // for ColumnData
#include "table/column_layout_relation_data.h"       // for ColumnLayoutRela...
#include "table/relational_schema.h"                 // for RelationalSchema
#include "ucc/ucc_verifier/ucc_stats_calculator.h"   // for UCCStatsCalculator

namespace algos {

UCCVerifier::UCCVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void UCCVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };
    auto calculate_default = [get_schema_cols]() {
        config::IndicesType indices(get_schema_cols());
        std::iota(indices.begin(), indices.end(), 0);
        return indices;
    };
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::IndicesOption{
            kUCCIndices, kDUCCIndices, config::IndicesOption::NormalizeIndices,
            std::move(calculate_default)}(&column_indices_, std::move(get_schema_cols)));
}

void UCCVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kUCCIndices});
}

void UCCVerifier::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC verifying is meaningless.");
    }
}

unsigned long long UCCVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    VerifyUCC();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

std::shared_ptr<model::PLI const> UCCVerifier::CalculatePLI() {
    std::shared_ptr<model::PLI const> pli =
            relation_->GetColumnData(column_indices_[0]).GetPliOwnership();
    for (size_t i = 1; i < column_indices_.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(column_indices_[i]).GetPositionListIndex());
    }
    return pli;
}

void UCCVerifier::VerifyUCC() {
    std::shared_ptr<model::PLI const> pli = CalculatePLI();
    stats_calculator_ = std::make_unique<UCCStatsCalculator>(relation_);
    stats_calculator_->CalculateStatistics(pli->GetIndex());
}

}  // namespace algos
