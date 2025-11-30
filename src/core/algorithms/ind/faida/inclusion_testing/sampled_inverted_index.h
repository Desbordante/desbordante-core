#pragma once

#include <atomic_bitvector.hpp>
#include <hash_set2.hpp>
#include <hash_table8.hpp>
#include <mutex>
#include <unordered_map>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/ind/faida/inclusion_testing/hll_data.h"
#include "core/algorithms/ind/faida/util/simple_ind.h"

namespace algos::faida {

class SampledInvertedIndex {
private:
    using LockPrimitive = std::mutex;

    // Maps combined hash to the column combination IDs
    emhash8::HashMap<size_t, std::pair<emhash2::HashSet<int>, std::unique_ptr<LockPrimitive>>>
            inverted_index_;

    emhash2::HashSet<SimpleIND> discovered_inds_;

    boost::dynamic_bitset<> seen_cc_indices_;
    atomicbitvector::atomic_bv_t non_covered_cc_indices_;

    int max_id_;

public:
    SampledInvertedIndex() : non_covered_cc_indices_(0), max_id_(0) {}

    void Init(std::vector<size_t> const& sampled_hashes, int max_id);

    bool Update(SimpleCC const& combination, size_t hash) {
        auto set_iter = inverted_index_.find(hash);

        if (set_iter == inverted_index_.end()) {
            if (!non_covered_cc_indices_.test(combination.GetIndex())) {
                non_covered_cc_indices_.set(combination.GetIndex());
            }
            return false;
        }

        auto& [set, lock_primitive] = set_iter->second;
        std::lock_guard lock(*lock_primitive);
        set.insert(combination.GetIndex());
        return true;
    }

    bool IsCovered(std::shared_ptr<SimpleCC> const& combination) {
        return !non_covered_cc_indices_.test(combination->GetIndex());
    }

    bool IsIncludedIn(std::shared_ptr<SimpleCC> const& a, std::shared_ptr<SimpleCC> const& b) {
        return !seen_cc_indices_.test(a->GetIndex()) ||
               discovered_inds_.find(SimpleIND(a, b)) != discovered_inds_.end();
    }

    void FinalizeInsertion(std::unordered_map<
                           TableIndex, emhash8::HashMap<std::shared_ptr<SimpleCC>, HLLData>> const&
                                   hlls_by_table);
};

}  // namespace algos::faida
