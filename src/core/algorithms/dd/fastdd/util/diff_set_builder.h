#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/dd/fastdd/model/diff_set.h"
#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/algorithms/dd/fastdd/util/cross_isn_builder.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/algorithms/dd/fastdd/util/isn_info.h"
#include "core/algorithms/dd/fastdd/util/single_isn_builder.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class DiffSetBuilder {
private:
    std::shared_ptr<ISNInfo> isn_info_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    DiffSet<Bitset> diff_set_;
    std::vector<std::vector<Bitset>> const& offset_to_predicates_;
    std::size_t bitset_size_;
    std::vector<model::DFConstraint> min_max_dif_;

    template <typename ClueType>
    boost::unordered::unordered_flat_set<ClueType> BuildISNs(std::vector<PliShard> pli_shards) {
        boost::unordered::unordered_flat_set<ClueType> final_clue_set;

        for (std::size_t i = 0; i != pli_shards.size(); ++i) {
            SingleISNBuilder<ClueType, Bitset> single(isn_info_, distance_calculator_,
                                                      pli_shards[i], offset_to_predicates_,
                                                      min_max_dif_, bitset_size_);
            boost::unordered::unordered_flat_set<ClueType> clue_set = single.BuildISNs();
            for (auto&& clue : clue_set) {
                final_clue_set.insert(std::move(clue));
            }
            for (std::size_t j = i + 1; j != pli_shards.size(); ++j) {
                CrossISNBuilder<ClueType, Bitset> cross(
                        isn_info_, distance_calculator_, pli_shards[i], pli_shards[j],
                        offset_to_predicates_, min_max_dif_, bitset_size_);
                boost::unordered::unordered_flat_set<ClueType> clue_set = cross.BuildISNs();
                for (auto&& clue : clue_set) {
                    final_clue_set.insert(std::move(clue));
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
          diff_set_(df_builder, isn_info_),
          offset_to_predicates_(diff_set_.GetOffsetToPredicates()),
          bitset_size_(df_builder.GetDifFuncNum()),
          min_max_dif_(df_builder.GetDifFuncsSize(), {std::numeric_limits<double>::max(), 0}) {}

    void BuildDiffSet(std::vector<PliShard> pli_shards) {
        if (!isn_info_->Overflows()) {
            boost::unordered::unordered_flat_set<std::size_t> clue_set =
                    BuildISNs<std::size_t>(std::move(pli_shards));
            diff_set_.Build(clue_set);
        } else {
            boost::unordered::unordered_flat_set<Bitset> bitset_set =
                    BuildISNs<Bitset>(std::move(pli_shards));
            diff_set_.Build(std::move(bitset_set));
        }
    }

    DiffSet<Bitset> GetDiffSet() const noexcept {
        return diff_set_;
    }

    std::vector<model::DFConstraint> GetMinMaxDif() const noexcept {
        return min_max_dif_;
    }
};

}  // namespace algos::dd
