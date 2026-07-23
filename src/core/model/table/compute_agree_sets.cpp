#include "core/model/table/compute_agree_sets.h"

#include <ranges>
#include <set>

namespace {
using Cluster = std::vector<int>;
using ClusterSet = std::unordered_set<Cluster, boost::hash<Cluster>>;

void CalculateSupersets(ClusterSet& current_maximal_sets /* I think?? */,
                        std::deque<Cluster> const& new_pli) {
    ClusterSet new_pli_incomparable_clusters;
    auto hash = [beg = current_maximal_sets.cbegin()](ClusterSet::const_iterator it) {
        return std::distance(beg, it);
    };
    std::unordered_set<ClusterSet::const_iterator, decltype(hash)> included_in_new_cluster(1, hash);
    std::set<std::deque<Cluster>::const_iterator> included_in_maximal_set;

    for (auto current_maximal_sets_it = current_maximal_sets.begin();
         current_maximal_sets_it != current_maximal_sets.end(); ++current_maximal_sets_it) {
        for (auto new_pli_iter = new_pli.begin();
             included_in_maximal_set.size() != new_pli.size() && new_pli_iter != new_pli.end();
             ++new_pli_iter) {
            if (included_in_maximal_set.contains(new_pli_iter)) {
                continue;
            }

            if (current_maximal_sets_it->size() >= new_pli_iter->size() &&
                std::ranges::includes(*current_maximal_sets_it, *new_pli_iter)) {
                new_pli_incomparable_clusters.erase(*new_pli_iter);
                included_in_maximal_set.insert(new_pli_iter);
                break;
            }

            if (new_pli_iter->size() >= current_maximal_sets_it->size() &&
                std::ranges::includes(*new_pli_iter, *current_maximal_sets_it)) {
                included_in_new_cluster.insert(current_maximal_sets_it);
            }

            new_pli_incomparable_clusters.insert(*new_pli_iter);
        }
    }

    for (auto& cluster : new_pli_incomparable_clusters) {
        current_maximal_sets.insert(std::move(cluster));
    }
    for (auto it : included_in_new_cluster) {
        current_maximal_sets.erase(it);
    }
}

boost::dynamic_bitset<> GetAgreeSet(std::vector<int> const& rec1, std::vector<int> const& rec2) {
    assert(rec1.size() == rec2.size());
    boost::dynamic_bitset<> agree_set(rec1.size());
    for (model::Index i = 0; i != rec1.size(); ++i) {
        if (rec1[i] != model::PositionListIndex::kSingletonValueId && rec1[i] == rec2[i])
            agree_set.set(i);
    }
    return agree_set;
}
}  // namespace

namespace model {
std::unordered_set<boost::dynamic_bitset<>> ComputeAgreeSets(
        StrippedPartitions const& stripped_partitions) {
    std::vector<PositionListIndex> const& plis = stripped_partitions.GetStrippedPartitions();
    auto not_empty_pli = std::ranges::find_if(
            plis, [](PositionListIndex const& pli) { return pli.GetSize() != 0; });

    ClusterSet max_representation;
    if (not_empty_pli != plis.end()) {
        max_representation.insert(not_empty_pli->GetIndex().begin(),
                                  not_empty_pli->GetIndex().end());

        for (auto pli_iter = std::next(not_empty_pli); pli_iter != plis.end(); ++pli_iter) {
            if (pli_iter->GetSize() != 0) {
                CalculateSupersets(max_representation, pli_iter->GetIndex());
            }
        }
    }

    std::unordered_set<boost::dynamic_bitset<>> agree_sets;
    std::unordered_map<int, std::vector<int>> relevant_records;

    for (auto const& cluster : max_representation) {
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

    // TODO: parallel version?
    for (auto const& cluster : max_representation) {
        auto const back_it = std::prev(cluster.end());
        for (auto p = cluster.begin(); p != back_it; ++p) {
            for (auto q = std::next(p); q != cluster.end(); ++q) {
                std::vector<int> const& rec1 = relevant_records.find(*p)->second;
                std::vector<int> const& rec2 = relevant_records.find(*q)->second;
                agree_sets.insert(GetAgreeSet(rec1, rec2));
            }
        }
    }

    agree_sets.insert(boost::dynamic_bitset<>(plis.size()));

    return agree_sets;
}
}  // namespace model
