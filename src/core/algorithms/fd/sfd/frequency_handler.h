#pragma once

#include <unordered_map>
#include <vector>
#include <cstddef>

#include "core/model/table/column_index.h"
#include "core/model/table/typed_column_data.h"

namespace algos {

class FrequencyHandler {
private:
    std::vector<std::size_t> cardinality_;
    std::vector<std::size_t> freq_sums_;
    std::vector<std::unordered_map<std::string, std::size_t>> frequency_maps_;

public:
    void InitFrequencyHandler(std::vector<model::TypedColumnData> const &data,
                              model::ColumnIndex columns, std::size_t max_amount_of_categories);

    [[nodiscard]] std::size_t GetColumnFrequencySum(model::ColumnIndex col_ind) const {
        return freq_sums_[col_ind];
    }

    [[nodiscard]] std::size_t GetColumnCardinality(model::ColumnIndex col_ind) const {
        return cardinality_[col_ind];
    }

    [[nodiscard]] std::size_t GetValueOrdinalNumberAtColumn(std::string const &val,
                                                       model::ColumnIndex col_ind) const {
        return frequency_maps_[col_ind].at(val);
    }

    [[nodiscard]] bool ContainsValAtColumn(std::string const &val,
                                           model::ColumnIndex col_ind) const {
        return frequency_maps_[col_ind].find(val) != frequency_maps_[col_ind].end();
    }

    [[nodiscard]] std::size_t Size() const {
        return frequency_maps_.size();
    }

    [[nodiscard]] std::size_t ColumnFrequencyMapSize(model::ColumnIndex col_ind) const {
        return frequency_maps_[col_ind].size();
    }

    void Clear() {
        cardinality_.clear();
        frequency_maps_.clear();
        freq_sums_.clear();
    }
};
}  // namespace algos
