#include "algorithms/fd/pattern_fd_verifier/pattern_fd_verifier.h"

#include <regex>
#include <stdexcept>
#include <string>

#include <easylogging++.h>

#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names.h"
#include "config/tabular_data/input_table/option.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "util/timed_invoke.h"

namespace algos::pattern_fd {

PatternFDVerifier::PatternFDVerifier() : Algorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable, kEqualNulls});
}

void PatternFDVerifier::RegisterOptions() {
    auto get_schema_cols = [this]() { return typed_relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterOption(config::Option<config::IndexType>(&rhs_index_, "rhs_index",
                                                     "Index of the RHS attribute")
                           .SetValueCheck([get_schema_cols](size_t index) {
                               if (index >= get_schema_cols()) {
                                   throw config::ConfigurationError("RHS index out of bounds: " +
                                                                    std::to_string(index));
                               }
                               return true;
                           }));
    RegisterOption(config::Option<PatternsTable>{
            &patterns_table_, "patterns",
            "Table of patterns (regex or token & index where the token is located)"});
    RegisterOption(config::Option<int>{&min_pattern_fd_coverage_, "min_pattern_fd_coverage",
                                       "Minimum number of rows in FD"});
    RegisterOption(config::Option<int>{&min_pattern_inclusion_, "min_pattern_inclusion",
                                       "Minimum number of rows with pattern inclusion"});
    RegisterOption(
            config::Option<double>{&max_rhs_deviation_, "max_rhs_deviation",
                                   "Maximum deviation for RHS attribute of specified pattern"});
}

void PatternFDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kLhsIndices, kRhsIndex, "patterns", "min_pattern_fd_coverage",
                          "min_pattern_inclusion", "max_rhs_deviation"});
}

void PatternFDVerifier::LoadDataInternal() {
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, is_null_equal_null_);
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: Pattern FD verifying is meaningless.");
    }
}

unsigned long long PatternFDVerifier::ExecuteInternal() {
    LOG(DEBUG) << "Starting Pattern FD verification...";

    auto verification_time = ::util::TimedInvoke(&PatternFDVerifier::VerifyPatternFD, this);
    LOG(DEBUG) << "CFD verification took " << std::to_string(verification_time) << "ms";

    auto stats_calculation_time =
            ::util::TimedInvoke(&PatternFDVerifier::CalculateStatistics, this);
    LOG(DEBUG) << "Statistics calculation took " << std::to_string(stats_calculation_time) << "ms";

    return verification_time + stats_calculation_time;
}

void PatternFDVerifier::VerifyPatternFD() {
    stats_calculator_ = std::make_unique<PatternFDStatsCalculator>(typed_relation_, patterns_table_,
                                                                   lhs_indices_, rhs_index_);
}

}  // namespace algos::pattern_fd