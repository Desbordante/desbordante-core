#include "core/algorithms/cfd/cfun/quadruple.h"

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace algos::cfd::cfun {

std::vector<unsigned int> Quadruple::CalculateProbingTable(size_t num_rows) const {
    std::vector<unsigned int> probing_table(num_rows, 0);
    unsigned int next_cluster_id = 1;
    for (auto const& cluster : clusters_) {
        for (auto tuple : cluster.indices) {
            probing_table[tuple] = next_cluster_id;
        }
        ++next_cluster_id;
    }
    return probing_table;
}

Quadruple Quadruple::Intersect(Quadruple const& other, size_t num_rows, size_t min_support) const {
    auto probing = other.CalculateProbingTable(num_rows);

    std::vector<unsigned int> assoc(num_rows + 1, 0);
    std::vector<unsigned int> counts(num_rows + 1, 0);
    std::vector<unsigned int> elem_result(num_rows + 1, 0);

    unsigned int next_id = 1;

    for (auto const& cluster : clusters_) {
        unsigned int start_id = next_id;
        for (unsigned int tuple : cluster.indices) {
            unsigned int probing_value = probing[tuple];
            if (probing_value != 0) {
                if (assoc[probing_value] < start_id) {
                    assoc[probing_value] = next_id++;
                }
                unsigned int old_id = assoc[probing_value];
                ++counts[old_id];
                elem_result[tuple] = old_id;
            }
        }
    }

    std::vector<int> remap(next_id, -1);
    Partition new_clusters;
    new_clusters.reserve(next_id);

    for (unsigned int old = 1; old < next_id; ++old) {
        if (counts[old] >= min_support) {
            remap[old] = new_clusters.size();
            new_clusters.emplace_back();
            new_clusters.back().reserve(counts[old]);
        }
    }

    for (auto const& cluster : clusters_) {
        for (unsigned int elem : cluster.indices) {
            unsigned int old_cluster_id = elem_result[elem];
            if (old_cluster_id != 0) {
                int new_cluster_id = remap[old_cluster_id];
                if (new_cluster_id != -1) {
                    new_clusters[new_cluster_id].push_back(elem);
                }
            }
        }
    }

    return {std::move(new_clusters), this->attribute_ | other.attribute_};
}

bool Quadruple::HasSameBegin(Quadruple const& other) const noexcept {
    auto bit1 = attribute_.find_first();
    auto bit2 = other.attribute_.find_first();

    for (unsigned int i = 0; i < attribute_.count() - 1; ++i) {
        if (bit1 != bit2) {
            return false;
        }
        bit1 = attribute_.find_next(bit1);
        bit2 = other.attribute_.find_next(bit2);
    }

    return bit1 != bit2;
}

void Quadruple::PruneRedundantSets(Quadruple const& X, std::vector<size_t> const& used_clusters,
                                   size_t num_rows) {
    boost::dynamic_bitset<> used_rows_mask(num_rows);
    for (auto cluster_id : used_clusters) {
        for (auto tuple : X.clusters_[cluster_id].indices) {
            used_rows_mask.set(tuple);
        }
    }

    std::erase_if(clusters_,
                  [&](auto const& cluster) { return used_rows_mask.test(cluster.indices[0]); });
}

void Quadruple::UpdateClosure(std::vector<size_t> const& valid_cluster_ids,
                              unsigned int attribute_num) {
    for (auto id : valid_cluster_ids) {
        clusters_[id].closure.set(attribute_num);
    }
}

void Quadruple::UpdateQuasiClosure(std::vector<Quadruple const*> const& subsets, size_t num_rows) {
    auto candidate_ec = CalculateProbingTable(num_rows);

    for (Quadruple const* subset : subsets) {
        for (auto const& subset_cluster : subset->clusters_) {
            for (auto tuple_id : subset_cluster.indices) {
                unsigned int curr_ec = candidate_ec[tuple_id];
                if (curr_ec != 0) {
                    clusters_[curr_ec - 1].quasi_closure |= subset_cluster.closure;
                }
            }
        }
    }
    for (auto& cluster : clusters_) {
        cluster.closure = cluster.quasi_closure;
    }
}

std::vector<size_t> Quadruple::CheckCFD(unsigned int target_col,
                                        std::vector<unsigned int> const& target_col_ec) const {
    std::vector<size_t> valid_clusters_id;

    for (size_t i = 0; i < clusters_.size(); ++i) {
        auto const& cluster = clusters_[i];
        if (cluster.quasi_closure.test(target_col)) continue;

        unsigned int current_ec = target_col_ec[cluster.indices[0]];

        if (std::ranges::any_of(cluster.indices, [&](unsigned int tuple) {
                return target_col_ec[tuple] == 0 || target_col_ec[tuple] != current_ec;
            })) {
            continue;
        }
        valid_clusters_id.push_back(i);
    }
    return valid_clusters_id;
}
}  // namespace algos::cfd::cfun
