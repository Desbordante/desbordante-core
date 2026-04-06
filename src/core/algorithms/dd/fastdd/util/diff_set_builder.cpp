#include "core/algorithms/dd/fastdd/util/diff_set_builder.h"

#include <cstddef>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/algorithms/dd/fastdd/util/cross_isn_builder.h"
#include "core/algorithms/dd/fastdd/util/single_isn_builder.h"

namespace algos::dd {

void DiffSetBuilder::BuildDiffSet(std::vector<PliShard> pli_shards) {
    boost::unordered::unordered_flat_set<std::size_t> clue_set = BuildISNs(std::move(pli_shards));
    diff_set_.Build(clue_set);
}

// TODO: add thread pool here
boost::unordered::unordered_flat_set<std::size_t> DiffSetBuilder::BuildISNs(
        std::vector<PliShard> pli_shards) const {
    boost::unordered::unordered_flat_set<std::size_t> final_clue_set;
    for (std::size_t i = 0; i != pli_shards.size(); ++i) {
        SingleISNBuilder single(isn_info_, distance_calculator_, pli_shards[i]);
        boost::unordered::unordered_flat_set<std::size_t> clue_set = single.BuildISNs();
        for (std::size_t clue : clue_set) {
            final_clue_set.insert(clue);
        }
        for (std::size_t j = i + 1; j != pli_shards.size(); ++j) {
            CrossISNBuilder cross(isn_info_, distance_calculator_, pli_shards[i], pli_shards[j]);
            boost::unordered::unordered_flat_set<std::size_t> clue_set = cross.BuildISNs();
            for (std::size_t clue : clue_set) {
                final_clue_set.insert(clue);
            }
        }
    }

    return final_clue_set;
}

}  // namespace algos::dd
