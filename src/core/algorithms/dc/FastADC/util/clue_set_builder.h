#pragma once

#include <easylogging++.h>

#include "cross_clue_set_builder.h"
#include "single_clue_set_builder.h"

namespace algos::fastadc {

/**
 * Manages construction of a set of clues based on PliShard inputs.
 */
class ClueSetBuilder {
public:
    ClueSetBuilder(PredicateBuilder const& pbuilder) : predicate_builder_(pbuilder){};

    ClueSet BuildClueSet(std::vector<PliShard> const& pliShards) {
        ClueSet clue_set;
        size_t task_count = (pliShards.size() * (pliShards.size() + 1)) / 2;
        LOG(DEBUG) << "  [CLUE] task count: " << task_count;

        for (size_t i = 0; i < pliShards.size(); i++) {
            for (size_t j = i; j < pliShards.size(); j++) {
                ClueSet partial_clue_set;

                if (i == j) {
                    partial_clue_set =
                            SingleClueSetBuilder{predicate_builder_, pliShards[i]}.BuildClueSet();
                } else {
                    partial_clue_set =
                            CrossClueSetBuilder{predicate_builder_, pliShards[i], pliShards[j]}
                                    .BuildClueSet();
                }

                for (auto const& [clue, count] : partial_clue_set) {
                    clue_set[clue] += count;
                }
            }
        }

        return clue_set;
    }

private:
    PredicateBuilder const& predicate_builder_;
};

}  // namespace algos::fastadc
