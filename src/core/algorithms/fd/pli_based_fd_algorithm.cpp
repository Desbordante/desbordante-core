#include "core/algorithms/fd/pli_based_fd_algorithm.h"

#include <stdexcept>
#include <vector>

#include "core/config/common_option.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos {

PliBasedFDAlgorithm::PliBasedFDAlgorithm() : FDAlgorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void PliBasedFDAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
}

void PliBasedFDAlgorithm::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }
}

}  // namespace algos
