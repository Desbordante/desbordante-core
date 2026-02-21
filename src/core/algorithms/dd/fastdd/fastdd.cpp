#include "core/algorithms/dd/fastdd/fastdd.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <list>
#include <memory>
#include <stdexcept>
#include <utility>

#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/diff_set_builder.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"
#include "core/algorithms/dd/fastdd/util/hybrid_evidence_inverter.h"
#include "core/algorithms/dd/fastdd/util/min_max_dif_calculator.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/column_index.h"
#include "core/util/logger.h"

namespace algos::dd {

FastDD::FastDD() : DDAlgorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void FastDD::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    config::InputTable default_table;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&difference_table_, kDifferenceTable, kDDifferenceTable, default_table});
    RegisterOption(Option{&num_rows_, kNumRows, kDNumRows, 0U});
    RegisterOption(Option{&num_columns_, kNumColumns, kDNumColumns, 0U});
    RegisterOption(Option{&shard_length_, kShardLength, kDShardLength, 10000U});
}

void FastDD::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kDifferenceTable, kNumRows, kNumColumns, kShardLength});
}

void FastDD::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                       false);  // nulls are ignored
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: DD mining is meaningless.");
    }
}

void FastDD::SetLimits() {
    unsigned all_rows_num = typed_relation_->GetNumRows();
    model::ColumnIndex all_columns_num = typed_relation_->GetNumColumns();
    if (num_rows_ > all_rows_num) {
        throw std::invalid_argument(
                "'num_rows' must be less or equal to the number of rows in the table (total "
                "rows: " +
                std::to_string(all_rows_num) + ")");
    }
    if (num_columns_ > all_columns_num) {
        throw std::invalid_argument(
                "'num_columns' must be less or equal to the number of columns in the table (total "
                "columns: " +
                std::to_string(all_columns_num) + ")");
    }
    if (num_rows_ == 0) num_rows_ = all_rows_num;
    if (num_columns_ == 0) num_columns_ = all_columns_num;
}

void FastDD::CheckTypes() {
    type_ids_.resize(num_columns_, model::TypeId::kUndefined);
    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
        model::TypeId type_id = column.GetTypeId();

        if (type_id == model::TypeId::kUndefined) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" type undefined.");
        }
        if (type_id == model::TypeId::kMixed) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" contains values of different types.");
        }

        type_ids_[column_index] = type_id;

        for (std::size_t row_index = 0; row_index < num_rows_; row_index++) {
            if (column.IsNull(row_index)) {
                throw std::runtime_error("Some of the value coordinates are nulls.");
            }
            if (column.IsEmpty(row_index)) {
                throw std::runtime_error("Some of the value coordinates are empty.");
            }
        }
    }
}

void FastDD::ParseDifferenceTable() {
    if (difference_table_) {
        difference_typed_relation_ =
                model::ColumnLayoutTypedRelationData::CreateFrom(*difference_table_,
                                                                 false);  // nulls are ignored
        if (typed_relation_->GetNumColumns() != num_columns_) {
            throw std::invalid_argument(
                    "The number of columns in the difference table must be equal to the number of "
                    "columns in the loaded table or to 'num_columns' if specified");
        }
    }
}

unsigned long long FastDD::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();
    LOG_INFO("Start");

    SetLimits();
    CheckTypes();
    ParseDifferenceTable();

    std::shared_ptr<DistanceCalculator> distance_calculator =
            std::make_shared<DistanceCalculator>(typed_relation_);

    PliShardBuilder pli_shard_builder(shard_length_);
    std::vector<PliShard> pli_shards =
            pli_shard_builder.BuildPliShards(typed_relation_->GetColumnData());
    LOG_INFO("Built PLIs");
    LOG_DEBUG("Number of PLI shards: {}", pli_shards.size());
    MinMaxDifCalculator min_max_dif_calculator(distance_calculator, pli_shards);

    DifferentialFunctionBuilder df_builder(typed_relation_, num_rows_, num_columns_,
                                           distance_calculator,
                                           min_max_dif_calculator.GetMinMaxDif());
    df_builder.BuildDFList(difference_typed_relation_);
    LOG_INFO("Built DF set");
    LOG_INFO("Search space size: {}", df_builder.GetDifFuncNum());

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_DEBUG("Current time: {}", elapsed_milliseconds.count());

    DiffSetBuilder diff_set_builder(df_builder, distance_calculator);
    diff_set_builder.BuildDiffSet(std::move(pli_shards));
    DiffSet diff_set = diff_set_builder.GetDiffSet();
    LOG_INFO("Built Diff-Set");
    std::vector<MatchDF> match_dfs = diff_set.GetMatchDFs();
    LOG_INFO("Diff-Set size: {}", match_dfs.size());
    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_DEBUG("Current time: {}", elapsed_milliseconds.count());

    HybridEvidenceInverter hybrid_evidence_inverter(std::move(match_dfs), df_builder);
    LOG_INFO("Built Inverter");

    dds_ = hybrid_evidence_inverter.BuildDDs();

    std::ranges::for_each(dds_, [this](auto const& dd) {
        auto df_to_constraint = [](DifferentialFunction const& df) {
            return model::DFStringConstraint{df.GetColumn()->GetName(), df.GetConstraint()};
        };

        std::list<model::DFStringConstraint> lhs;
        std::ranges::transform(dd.GetLhs(), std::back_inserter(lhs), df_to_constraint);

        std::list<model::DFStringConstraint> rhs = {df_to_constraint(dd.GetRhs())};

        RegisterDD(model::DDString{std::move(lhs), std::move(rhs)});
    });

    LOG_INFO("Built DDs: {}", DDList().size());
    if (DDList().size() <= 100) {
        for (auto const& dd : DDList()) {
            LOG_DEBUG(dd.ToString());
        }
    }

    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_INFO("Algorithm time: {}", elapsed_milliseconds.count());
    return elapsed_milliseconds.count();
}

}  // namespace algos::dd
