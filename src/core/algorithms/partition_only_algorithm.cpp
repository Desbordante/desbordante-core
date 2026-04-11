#include "core/algorithms/partition_only_algorithm.h"

#include "core/config/tabular_data/input_table/option.h"

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
    stripped_partitions_ = model::StrippedPartitions::CreateFrom(*input_table_);
    if (stripped_partitions_->GetNumRows() == 0)
        throw std::runtime_error("Dataset \"" + input_table_->GetRelationName() +
                                 "\" is empty, mining dependencies on empty is not supported.");
}
}  // namespace algos
