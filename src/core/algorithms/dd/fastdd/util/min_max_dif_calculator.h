#pragma once

#include <memory>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"

namespace algos::dd {

class MinMaxDifCalculator {
private:
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    std::vector<model::DFConstraint> min_max_dif_;

    void CalculateSingleMinMaxDif(PliShard const& pli_shard);
    void CalculateCrossMinMaxDif(PliShard const& first_pli_shard, PliShard const& second_pli_shard);

public:
    MinMaxDifCalculator(std::shared_ptr<DistanceCalculator> distance_calculator,
                        std::vector<PliShard> const& pli_shards);

    std::vector<model::DFConstraint> GetMinMaxDif() const {
        return min_max_dif_;
    }
};

}  // namespace algos::dd
