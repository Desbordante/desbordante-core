#include "core/algorithms/partition_only_algorithm.h"

#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/create_stripped_partitions.h"

namespace algos {
PartitionOnlyAlgorithm::PartitionOnlyAlgorithm() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void PartitionOnlyAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
}

void PartitionOnlyAlgorithm::LoadDataInternal() {
    table_header_ = model::TableHeader::FromDatasetStream(*input_table_);
    stripped_partitions_ = util::CreateStrippedPartitions(*input_table_);
    if (stripped_partitions_.empty()) {
        throw std::runtime_error(
                "Dataset \"" + table_header_.table_name +
                "\" is empty, mining dependencies on empty datasets is not supported.");
    }
    for (model::PositionListIndex& pli : stripped_partitions_) {
        pli.ForceCacheProbingTable();
        if (pli.GetCachedProbingTable()->empty())
            throw std::runtime_error(
                    "Dataset \"" + table_header_.table_name +
                    "\" is empty, mining dependencies on empty datasets is not supported.");
    }
}
}  // namespace algos
