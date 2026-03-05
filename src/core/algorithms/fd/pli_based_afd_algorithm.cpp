#include "core/algorithms/fd/pli_based_afd_algorithm.h"

#include "core/config/equal_nulls/option.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos {

PliBasedAFDAlgorithm::PliBasedAFDAlgorithm(
        std::vector<std::string_view> phase_names)
    : AFDAlgorithm(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void PliBasedAFDAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
}

void PliBasedAFDAlgorithm::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);;

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }
}

}  // namespace algos
