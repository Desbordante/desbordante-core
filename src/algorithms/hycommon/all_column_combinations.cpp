#include "all_column_combinations.h"

#include <utility>

#include <boost/dynamic_bitset.hpp>

namespace algos::hy {

void AllColumnCombinations::Add(boost::dynamic_bitset<>&& column_set) {
    Add(column_set);
}

void AllColumnCombinations::Add(boost::dynamic_bitset<> const& column_set) {
    if (total_ccs_.insert(column_set).second) {
        new_ccs_.Add(boost::dynamic_bitset<>(column_set));
    }
}

ColumnCombinationList AllColumnCombinations::MoveOutNewColumnCombinations() {
    size_t num_attributes = NumAttributes();
    ColumnCombinationList old = std::move(new_ccs_);
    new_ccs_ = ColumnCombinationList{num_attributes};
    return old;
}

}  // namespace algos::hy
