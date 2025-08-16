#include "pli_based_fd_algorithm.h"

#include <stdexcept>  // for runtime_error
#include <utility>    // for move

#include <boost/type_index/type_index_facade.hpp>  // for operator==

#include "common_option.h"                           // for CommonOption
#include "config/equal_nulls/option.h"               // for kEqualNullsOpt
#include "config/tabular_data/input_table/option.h"  // for kTableOpt
#include "fd/fd_algorithm.h"                         // for FDAlgorithm
#include "table/column_data.h"                       // for ColumnData
#include "table/column_layout_relation_data.h"       // for ColumnLayoutRela...
#include "table/position_list_index.h"               // for PositionListIndex

namespace algos {

PliBasedFDAlgorithm::PliBasedFDAlgorithm(
        std::vector<std::string_view> phase_names,
        std::optional<ColumnLayoutRelationDataManager> relation_manager)
    : FDAlgorithm(std::move(phase_names)),
      relation_manager_(relation_manager.has_value()
                                ? *relation_manager
                                : ColumnLayoutRelationDataManager{
                                          &input_table_, &is_null_equal_null_, &relation_}) {
    if (relation_manager.has_value()) return;
    RegisterRelationManagerOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void PliBasedFDAlgorithm::RegisterRelationManagerOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
}

void PliBasedFDAlgorithm::LoadDataInternal() {
    relation_ = relation_manager_.GetRelation();

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
