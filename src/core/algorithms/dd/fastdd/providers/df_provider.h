#pragma once

#include <unordered_map>

#include "core/algorithms/dd/fastdd/model/differential_function.h"
#include "core/algorithms/dd/fastdd/model/operator.h"
#include "core/model/table/column.h"

namespace algos::dd {

class DFProvider {
private:
    std::unordered_map<
            Operator,
            std::unordered_map<Column const*, std::unordered_map<double, DifferentialFunction>>>
            df_map_;

public:
    DifferentialFunction GetDifferentialFunction(Operator op, Column const* column,
                                                 double threshold) {
        auto [it, is_new] = df_map_[op][column].try_emplace(threshold, op, threshold, column);

        return it->second;
    }
};

}  // namespace algos::dd
