#include "core/algorithms/dd/fastdd/util/isn_info.h"

#include <utility>

namespace algos::dd {

ISNInfo::ISNInfo(DifferentialFunctionBuilder const& df_builder) {
    model::ColumnIndex num_columns = df_builder.GetNumColumns();
    bases_.reserve(num_columns);
    df_packs_.reserve(num_columns);
    std::size_t cur_base = 1;
    for (model::ColumnIndex index = 0; index != num_columns; ++index) {
        std::vector<double> cur_thresholds = df_builder.GetThresholds(index);
        std::size_t num_intervals = cur_thresholds.size() + 1;
        bool is_distance_ordered = df_builder.IsDistanceOrdered(index);
        bases_.push_back(cur_base);
        df_packs_.emplace_back(std::move(cur_thresholds), index, cur_base, is_distance_ordered);
        cur_base *= num_intervals;
    }
}

}  // namespace algos::dd
