#include "ind_verifier.h"

#include <unordered_set>

#include <boost/functional/hash.hpp>

#include "config/indices/option.h"
#include "config/option_using.h"
#include "config/tabular_data/input_tables/option.h"
#include "indices/option.h"
#include "model/table/dataset_stream_projection.h"
#include "table/dataset_stream_fixed.h"
#include "table/tuple_index.h"
#include "timed_invoke.h"

namespace algos {

INDVerifier::INDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

void INDVerifier::RegisterOptions() {
    RegisterOption(config::kTablesOpt(&input_tables_, 2));
    /* TODO: Add check to prevent indices duplication. */
    RegisterOption(config::kLhsIndicesOpt(
            &ind_.lhs, [this]() { return input_tables_.front()->GetNumberOfColumns(); }));
    RegisterOption(config::kRhsIndicesOpt(
            &ind_.rhs, [this]() { return input_tables_.back()->GetNumberOfColumns(); }));
}

void INDVerifier::ResetState() {
    for (auto const& table : input_tables_) {
        table->Reset();
    }

    clusters_.clear();
    error_ = 0;
    violating_rows_ = 0;
    violating_unique_rows_ = 0;
}

void INDVerifier::LoadDataInternal() {
    /* Do nothing, we don't prepocess any data before executing. */
}

void INDVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName()});
}

void INDVerifier::VerifyIND() {
    using DatasetStreamFixedProjection =
            model::DatasetStreamProjection<model::DatasetStreamFixed<>>;
    using Row = DatasetStreamFixedProjection::Row;
    using ValueIndex = int;

    config::InputTable const& lhs_table = input_tables_.front();
    config::InputTable const& rhs_table = input_tables_.back();

    std::unordered_set<Row, boost::hash<Row>> rhs_rows;
    DatasetStreamFixedProjection rhs_stream{rhs_table, ind_.rhs};
    while (rhs_stream.HasNextRow()) {
        rhs_rows.insert(rhs_stream.GetNextRow());
    }

    if (lhs_table == rhs_table) {
        /* We would iterate through the same table, so we should reset this table */
        lhs_table->Reset();
    }

    std::unordered_map<Row, ValueIndex, boost::hash<Row>> lhs_violating_row_to_id_map;
    std::unordered_map<ValueIndex, Cluster> lhs_id_to_cluster_map;

    ValueIndex next_value_id = 0;
    model::TupleIndex current_row_id = 0;

    std::unordered_set<Row, boost::hash<Row>> lhs_rows;

    DatasetStreamFixedProjection lhs_projected_stream{lhs_table, ind_.lhs};
    while (lhs_projected_stream.HasNextRow()) {
        Row row = lhs_projected_stream.GetNextRow();
        lhs_rows.insert(row);

        if (rhs_rows.contains(row)) {
            ++current_row_id;
            continue;
        }

        auto const& [it, inserted] = lhs_violating_row_to_id_map.try_emplace(row, next_value_id);
        if (inserted) {
            ++next_value_id;
        }

        ValueIndex value_id = it->second;
        lhs_id_to_cluster_map[value_id].push_back(current_row_id);

        ++violating_rows_;
        ++current_row_id;
    }

    for (auto& [value_id, cluster] : lhs_id_to_cluster_map) {
        clusters_.push_back(std::move(cluster));
    }

    model::TupleIndex violating_unique_rows = next_value_id;
    model::TupleIndex lhs_cardinality = lhs_rows.size();
    error_ = static_cast<Error>(violating_unique_rows) / lhs_cardinality;
}

unsigned long long INDVerifier::ExecuteInternal() {
    return util::TimedInvoke(&INDVerifier::VerifyIND, this);
}

}  // namespace algos
