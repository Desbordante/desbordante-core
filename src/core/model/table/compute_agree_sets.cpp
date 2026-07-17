#include "core/model/table/compute_agree_sets.h"

#include <algorithm>
#include <compare>
#include <ranges>
#include <set>

#include "core/model/index.h"

namespace {
using Cluster = std::vector<int>;
using ClusterVec = std::vector<Cluster>;

void RemoveSubsets(ClusterVec& current_maximal_sets, Cluster const& new_cluster,
                   ClusterVec::iterator existing_cluster_it) {
    std::size_t const new_cluster_size = new_cluster.size();
    while (existing_cluster_it != current_maximal_sets.end()) {
        Cluster const& existing_cluster = *existing_cluster_it;
        std::size_t existing_cluster_size = existing_cluster.size();
        if (new_cluster_size > existing_cluster_size &&
            std::ranges::includes(new_cluster, existing_cluster)) {
            *existing_cluster_it = std::move(current_maximal_sets.back());
            current_maximal_sets.pop_back();
        } else {
            ++existing_cluster_it;
        }
    }
}

void AddToMaximalSets(ClusterVec& current_maximal_sets, Cluster const& new_cluster) {
    assert(!current_maximal_sets.empty());
    auto existing_cluster_it = current_maximal_sets.begin();
    std::size_t const new_cluster_size = new_cluster.size();
    do {
        Cluster const& existing_cluster = *existing_cluster_it;
        std::size_t existing_cluster_size = existing_cluster.size();
        if (new_cluster_size == existing_cluster_size) {
            if (new_cluster == existing_cluster) return;
        } else if (new_cluster_size > existing_cluster_size) {
            if (std::ranges::includes(new_cluster, existing_cluster)) {
                *existing_cluster_it = std::move(current_maximal_sets.back());
                current_maximal_sets.pop_back();
                RemoveSubsets(current_maximal_sets, new_cluster, existing_cluster_it);
                break;
            }
        } else {
            if (std::ranges::includes(existing_cluster, new_cluster)) return;
        }
    } while (++existing_cluster_it != current_maximal_sets.end());
    current_maximal_sets.push_back(new_cluster);
}

void AddToMaximalSets(ClusterVec& current_maximal_sets, std::deque<Cluster> const& new_sets) {
    for (Cluster const& new_set : new_sets) {
        AddToMaximalSets(current_maximal_sets, new_set);
    }
}

model::AttributeMask GetAgreeSet(std::vector<int> const& rec1, std::vector<int> const& rec2) {
    assert(rec1.size() == rec2.size());
    model::AttributeMask agree_set(rec1.size());
    for (model::Index i = 0; i != rec1.size(); ++i) {
        if (rec1[i] != model::PositionListIndex::kSingletonValueId && rec1[i] == rec2[i])
            agree_set.set(i);
    }
    return agree_set;
}

model::AttributeMask GetAllEqAgreeSet(std::size_t size) {
    model::AttributeMask agree_set;
    agree_set.resize(size, true);
    return agree_set;
}
}  // namespace

namespace model {
AttributeMaskSet ComputeAgreeSets(std::vector<PositionListIndex> const& plis) {
    std::size_t const num_rows = plis.empty() ? 0 : plis.front().GetCachedProbingTable()->size();
    if (num_rows == 0) return {};

    auto not_all_unique_pli = std::ranges::find_if(
            plis, [](PositionListIndex const& pli) { return !pli.AllValuesAreUnique(); });

    ClusterVec maximal_equivalence_classes;

    if (not_all_unique_pli != plis.end()) {
        maximal_equivalence_classes.assign(not_all_unique_pli->GetIndex().begin(),
                                           not_all_unique_pli->GetIndex().end());

        for (auto pli_iter = std::next(not_all_unique_pli); pli_iter != plis.end(); ++pli_iter) {
            if (pli_iter->AllValuesAreUnique()) continue;
            AddToMaximalSets(maximal_equivalence_classes, pli_iter->GetIndex());
        }
    }

    AttributeMaskSet agree_sets{GetAllEqAgreeSet(plis.size())};
    std::unordered_map<int, std::vector<int>> relevant_records;

    for (auto const& cluster : maximal_equivalence_classes) {
        for (auto p = cluster.begin(); p != cluster.end(); ++p) {
            auto [it, is_new_record] = relevant_records.try_emplace(*p);
            if (!is_new_record) continue;

            std::vector<int>& record_equiv_class_ids = it->second;
            record_equiv_class_ids.reserve(plis.size());
            for (PositionListIndex const& pli : plis) {
                // "Efficient Discovery of Functional Dependencies and Armstrong Relations"
                // calculates this without computing the value IDs for all records (Algorithm 3),
                // but this was in the original code.
                record_equiv_class_ids.push_back((*pli.GetCachedProbingTable())[*p]);
            }
        }
    }

    // The original algorithm is slightly incorrect, since it does not account for records that have
    // been excluded from every stripped partition.
    if (relevant_records.size() != num_rows) agree_sets.insert(AttributeMask(plis.size()));

    // TODO: parallel version?
    for (auto const& cluster : maximal_equivalence_classes) {
        assert(cluster.size() >= 2);
        auto const back_it = std::prev(cluster.end());
        for (auto p = cluster.begin(); p != back_it; ++p) {
            for (auto q = std::next(p); q != cluster.end(); ++q) {
                std::vector<int> const& rec1 = relevant_records.find(*p)->second;
                std::vector<int> const& rec2 = relevant_records.find(*q)->second;
                agree_sets.insert(GetAgreeSet(rec1, rec2));
            }
        }
    }

    return agree_sets;
}
}  // namespace model
