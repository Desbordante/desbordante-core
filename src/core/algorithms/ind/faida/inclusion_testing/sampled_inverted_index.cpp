#include "sampled_inverted_index.h"

namespace algos::faida {

void SampledInvertedIndex::Init(std::vector<size_t> const& sampled_hashes, int max_id) {
    int constexpr initial_bucket_count = 4;
    for (size_t combined_hash : sampled_hashes) {
        inverted_index_.try_emplace(combined_hash,
                                    std::make_pair(emhash2::HashSet<int>(initial_bucket_count),
                                                   std::make_unique<LockPrimitive>()));
    }
    max_id_ = max_id;

    seen_cc_indices_ = boost::dynamic_bitset<>(max_id);
    non_covered_cc_indices_ = atomicbitvector::atomic_bv_t(max_id);

    discovered_inds_.clear();
}

void SampledInvertedIndex::FinalizeInsertion(
        std::unordered_map<TableIndex, emhash8::HashMap<std::shared_ptr<SimpleCC>, HLLData>> const&
                hlls_by_table) {
    std::unordered_map<int, std::vector<int>> ref_by_dep_ccs(max_id_ + 1);
    std::vector<std::shared_ptr<SimpleCC>> column_combinations(max_id_ + 1);

    for (auto const& [table_idx, hlls_by_cc] : hlls_by_table) {
        for (auto const& [cc, ints] : hlls_by_cc) {
            column_combinations[cc->GetIndex()] = cc;
        }
    }

    for (auto const& [record, _, hash] : inverted_index_) {
        emhash2::HashSet<int> const& cc_indices = record.first;

        for (int dep_cc_index : cc_indices) {
            seen_cc_indices_.set(dep_cc_index);
            auto ref_ccs_iter = ref_by_dep_ccs.find(dep_cc_index);

            if (ref_ccs_iter == ref_by_dep_ccs.end()) {
                std::vector<int> ref_ccs;
                ref_ccs.reserve(cc_indices.size() - 1);
                for (int ref_cc_index : cc_indices) {
                    if (dep_cc_index != ref_cc_index) {
                        ref_ccs.push_back(ref_cc_index);
                    }
                }
                ref_by_dep_ccs[dep_cc_index] = std::move(ref_ccs);
            } else if (!ref_ccs_iter->second.empty()) {
                std::vector<int>& ref_ccs = ref_ccs_iter->second;
                emhash2::HashSet<int> const& value_group = cc_indices;

                ref_ccs.erase(std::remove_if(ref_ccs.begin(), ref_ccs.end(),
                                             [&value_group](int cc_id) {
                                                 return value_group.find(cc_id) ==
                                                        value_group.end();
                                             }),
                              ref_ccs.end());
            }
        }
    }

    inverted_index_.clear();

    for (auto const& [lhs_idx, rhss] : ref_by_dep_ccs) {
        for (int rhs_idx : rhss) {
            discovered_inds_.insert(
                    SimpleIND(column_combinations[lhs_idx], column_combinations[rhs_idx]));
        }
    }
}

}  // namespace algos::faida
