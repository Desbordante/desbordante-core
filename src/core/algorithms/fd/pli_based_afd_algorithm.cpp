#include "core/algorithms/fd/pli_based_afd_algorithm.h"

#include "core/config/tabular_data/input_table/option.h"

namespace algos {

PliBasedAFDAlgorithm::PliBasedAFDAlgorithm() : AFDAlgorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void PliBasedAFDAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
}

void PliBasedAFDAlgorithm::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: AFD mining is meaningless.");
    }
}

}  // namespace algos
