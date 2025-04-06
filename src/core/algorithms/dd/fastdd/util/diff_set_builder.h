#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "core/algorithms/dd/fastdd/model/diff_set.h"
#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/algorithms/dd/fastdd/util/isn_info.h"

namespace algos::dd {

class DiffSetBuilder {
private:
    std::shared_ptr<ISNInfo> isn_info_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    DiffSet diff_set_;

    std::unordered_map<std::size_t, std::size_t> BuildISNs(std::vector<PliShard> pli_shards) const;
    void AccumulateClues(std::unordered_map<std::size_t, std::size_t>& final_clue_map,
                         std::unordered_map<std::size_t, std::size_t> const& new_clue_map) const;

public:
    DiffSetBuilder(DifferentialFunctionBuilder const& df_builder,
                   std::shared_ptr<DistanceCalculator> distance_calculator)
        : isn_info_(std::make_shared<ISNInfo>(df_builder)),
          distance_calculator_(distance_calculator),
          diff_set_(df_builder, isn_info_) {}

    void BuildDiffSet(std::vector<PliShard> pli_shards);

    DiffSet GetDiffSet() const noexcept {
        return diff_set_;
    }
};

}  // namespace algos::dd
