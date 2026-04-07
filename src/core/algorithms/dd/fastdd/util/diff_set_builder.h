#pragma once

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/dd/fastdd/model/diff_set.h"
#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/cross_isn_builder.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/algorithms/dd/fastdd/util/isn_info.h"
#include "core/algorithms/dd/fastdd/util/single_isn_builder.h"

namespace algos::dd {

template <typename Bitset>
class DiffSetBuilder {
private:
    std::shared_ptr<ISNInfo> isn_info_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    DiffSet<Bitset> diff_set_;

    boost::unordered::unordered_flat_set<std::size_t> BuildISNs(
            std::vector<PliShard> pli_shards) const {
        boost::unordered::unordered_flat_set<std::size_t> final_clue_set;
        for (std::size_t i = 0; i != pli_shards.size(); ++i) {
            SingleISNBuilder single(isn_info_, distance_calculator_, pli_shards[i]);
            boost::unordered::unordered_flat_set<std::size_t> clue_set = single.BuildISNs();
            for (std::size_t clue : clue_set) {
                final_clue_set.insert(clue);
            }
            for (std::size_t j = i + 1; j != pli_shards.size(); ++j) {
                CrossISNBuilder cross(isn_info_, distance_calculator_, pli_shards[i],
                                      pli_shards[j]);
                boost::unordered::unordered_flat_set<std::size_t> clue_set = cross.BuildISNs();
                for (std::size_t clue : clue_set) {
                    final_clue_set.insert(clue);
                }
            }
        }

        return final_clue_set;
    }

public:
    DiffSetBuilder(DifferentialFunctionBuilder const& df_builder,
                   std::shared_ptr<DistanceCalculator> distance_calculator)
        : isn_info_(std::make_shared<ISNInfo>(df_builder)),
          distance_calculator_(distance_calculator),
          diff_set_(df_builder, isn_info_) {}

    void BuildDiffSet(std::vector<PliShard> pli_shards) {
        boost::unordered::unordered_flat_set<std::size_t> clue_set =
                BuildISNs(std::move(pli_shards));
        diff_set_.Build(clue_set);
    }

    DiffSet<Bitset> GetDiffSet() const noexcept {
        return diff_set_;
    }
};

}  // namespace algos::dd
