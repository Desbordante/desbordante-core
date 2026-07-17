#pragma once

#include <utility>
#include <vector>

#include <boost/unordered_map.hpp>

#include "core/model/table/abstract_column_data.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

// Column presentation class for CFDRelationData.
class CFDColumnData : model::AbstractColumnData {
public:
    using NumToken = int;
    using ItemDictionary = boost::unordered_map<std::string, int>;

private:
    std::vector<NumToken> values_;
    // maps a value string to corresponding item id
    ItemDictionary values_dict_;

public:
    CFDColumnData(Column const* col, std::vector<int> col_values, ItemDictionary values_dict)
        : AbstractColumnData(col),
          values_(std::move(col_values)),
          values_dict_(std::move(values_dict)) {}

    std::vector<NumToken> const& GetValues() const {
        return values_;
    }

    ItemDictionary const& GetValueDict() const {
        return values_dict_;
    }

    std::string ToString() const final {
        return "Cfd data for " + column_->ToString();
    }
};
}  // namespace algos::cfd
