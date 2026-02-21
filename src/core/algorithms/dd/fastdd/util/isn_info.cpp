#include "core/algorithms/dd/fastdd/util/isn_info.h"

#include <utility>

namespace algos::dd {

ISNInfo::ISNInfo(DifferentialFunctionBuilder const& df_builder) {
    std::size_t const num_columns = df_builder.GetDifFuncsSize();
    bases_.reserve(num_columns);
    df_packs_.reserve(num_columns);
    std::size_t cur_base = 1;
    for (std::size_t index = 0; index != num_columns; ++index) {
        std::vector<ThresholdInfo> cur_thresholds = df_builder.GetThresholds(index);
        std::vector<std::size_t> cur_threshold_zones = df_builder.GetThresholdZones(index);
        std::size_t num_intervals = df_builder.GetZoneNum(index);
        bool is_distance_ordered = df_builder.IsDistanceOrdered(index);
        bases_.push_back(cur_base);
        model::ColumnIndex const column_index =
                df_builder.GetDifFuncs()[index][0].GetColumn()->GetIndex();
        df_packs_.emplace_back(std::move(cur_thresholds), std::move(cur_threshold_zones),
                               column_index, cur_base, is_distance_ordered);
        cur_base *= num_intervals;
    }
}

}  // namespace algos::dd
