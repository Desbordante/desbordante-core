#include "core/algorithms/dd/fastdd/util/diff_set_builder.h"

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/algorithms/dd/fastdd/util/cross_isn_builder.h"
#include "core/algorithms/dd/fastdd/util/single_isn_builder.h"

namespace algos::dd {

void DiffSetBuilder::BuildDiffSet(std::vector<PliShard> pli_shards) {
    std::unordered_map<std::size_t, std::size_t> clue_map = BuildISNs(std::move(pli_shards));
    diff_set_.Build(clue_map);
}

void DiffSetBuilder::AccumulateClues(
        std::unordered_map<std::size_t, std::size_t>& final_clue_map,
        std::unordered_map<std::size_t, std::size_t> const& new_clue_map) const {
    for (auto const& clue : new_clue_map) {
        auto [it, inserted] = final_clue_map.try_emplace(clue.first, clue.second);
        if (!inserted) it->second += clue.second;
    }
}

// TODO: add thread pool here
std::unordered_map<std::size_t, std::size_t> DiffSetBuilder::BuildISNs(
        std::vector<PliShard> pli_shards) const {
    std::unordered_map<std::size_t, std::size_t> final_clue_map;
    for (std::size_t i = 0; i != pli_shards.size(); ++i) {
        SingleISNBuilder single(isn_info_, distance_calculator_, pli_shards[i]);
        std::unordered_map<std::size_t, std::size_t> clue_map = single.BuildISNs();
        AccumulateClues(final_clue_map, clue_map);
        for (std::size_t j = i + 1; j != pli_shards.size(); ++j) {
            CrossISNBuilder cross(isn_info_, distance_calculator_, pli_shards[i], pli_shards[j]);
            std::unordered_map<std::size_t, std::size_t> clue_map = cross.BuildISNs();
            AccumulateClues(final_clue_map, clue_map);
        }
    }

    return final_clue_map;
}

}  // namespace algos::dd
