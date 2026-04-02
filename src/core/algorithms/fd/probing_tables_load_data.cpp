#include "core/algorithms/fd/probing_tables_load_data.h"

#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/create_stripped_partitions.h"

namespace algos {
ProbingTablesLoadData::ProbingTablesLoadData() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void ProbingTablesLoadData::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
}

void ProbingTablesLoadData::LoadDataInternal() {
    table_header_ = model::TableHeader::FromDatasetStream(*input_table_);
    input_table_column_plis_ = model::CreateStrippedPartitions(*input_table_);
    if (input_table_column_plis_.empty()) {
        throw std::runtime_error(
                "Dataset \"" + table_header_.table_name +
                "\" is empty, mining dependencies on empty datasets is not supported.");
    }
    for (model::PositionListIndex& pli : input_table_column_plis_) {
        pli.ForceCacheProbingTable();
        if (pli.GetCachedProbingTable()->empty())
            throw std::runtime_error(
                    "Dataset \"" + table_header_.table_name +
                    "\" is empty, mining dependencies on empty datasets is not supported.");
    }
}
}  // namespace algos
