#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/model/table/column_index.h"

namespace algos::dd {

struct DFPack {
    std::vector<double> thresholds;
    model::ColumnIndex column_index;
    std::size_t base;
    bool is_distance_ordered;
};

class ISNInfo {
private:
    std::vector<std::size_t> bases_;
    std::vector<DFPack> df_packs_;

public:
    ISNInfo(DifferentialFunctionBuilder const& df_builder);

    std::vector<DFPack> const& GetDFPacks() const noexcept {
        return df_packs_;
    }

    std::vector<std::size_t> const& GetBases() const noexcept {
        return bases_;
    }
};

}  // namespace algos::dd
