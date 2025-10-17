#include "pli_based_fd_algorithm.h"

#include <stdexcept>
#include <utility>

#include "common_option.h"
#include "config/equal_nulls/option.h"
#include "config/tabular_data/input_table/option.h"
#include "fd/fd_algorithm.h"
#include "table/column.h"
#include "table/column_data.h"
#include "table/column_layout_relation_data.h"
#include "table/position_list_index.h"

namespace algos {

PliBasedFDAlgorithm::PliBasedFDAlgorithm(std::vector<std::string_view> phase_names)
    : FDAlgorithm(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void PliBasedFDAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
}

void PliBasedFDAlgorithm::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }
}

std::vector<Column const*> PliBasedFDAlgorithm::GetKeys() const {
    assert(relation_ != nullptr);

    std::vector<Column const*> keys;
    for (ColumnData const& col : relation_->GetColumnData()) {
        if (col.GetPositionListIndex()->AllValuesAreUnique()) {
            keys.push_back(col.GetColumn());
        }
    }

    return keys;
}

}  // namespace algos
