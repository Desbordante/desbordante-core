#pragma once

#include <memory>

#include "core/algorithms/algorithm.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/stripped_partitions.h"
#include "core/model/table/table_header.h"

namespace algos {
// A few algorithms share the same LoadDataInternal method, this class is here purely to avoid code
// repetition.
class PartitionOnlyAlgorithm : public Algorithm {
    config::InputTable input_table_;

    void RegisterOptions();
    void LoadDataInternal() final;

protected:
    model::TableHeader table_header_;
    std::unique_ptr<model::StrippedPartitions> stripped_partitions_;

    PartitionOnlyAlgorithm();
};
}  // namespace algos
