#include "core/algorithms/fd/pli_based_fd_algorithm.h"

#include "core/config/equal_nulls/option.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos {

LegacyPliBasedFDAlgorithm::LegacyPliBasedFDAlgorithm() : FDAlgorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void LegacyPliBasedFDAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
}

void LegacyPliBasedFDAlgorithm::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }
}

}  // namespace algos
