#include "pli_cind.h"

#include <algorithm>

#include "ind/cind/condition_miners/cind_miner.hpp"
#include "ind/cind/condition_miners/position_lists_set.h"

namespace algos::cind {
PliCind::PliCind(std::shared_ptr<std::vector<model::ColumnDomain>> domains)
    : CindMiner(std::move(domains)) {}

void PliCind::ExecuteSingle(model::IND const& /*aind*/) {
}

void PliCind::MakePLs(std::vector<int> const& /*cond_attrs*/) {
    return;
}

// std::unordered_set<Condition> PliCind::GetConditions(std::vector<int> cond_attrs,
//                                                      std::vector<int> const& included_pos) {
//     MakePLs(cond_attrs);

//     std::unordered_set<Condition> result;
//     for (auto attr : cond_attrs) {
//         std::vector<int> empty_vector;
//         for (auto& cond : Analyze(cond_attrs, attr, empty_vector, nullptr, included_pos)) {
//             result.insert(cond);
//         }
//     }
//     return result;
// }

// std::unordered_set<Condition> PliCind::Analyze(std::vector<int> const& cond_attrs, int attr_idx,
//                                                std::vector<int> const& curr_attrs,
//                                                PLSetShared curr_pls,
//                                                std::vector<int> const& included_pos) {
//     std::vector<int> new_curr_attrs = curr_attrs;
//     new_curr_attrs.push_back(cond_attrs[attr_idx]);
//     PLSetShared curr_comb_pls;
//     if (!curr_attrs.empty()) {
//         curr_comb_pls = curr_pls->Intersect(attr_idx_to_pls_[attr_idx].get());
//     } else {
//         curr_comb_pls = PLSetShared(attr_idx_to_pls_[attr_idx].get());
//     }

//     std::unordered_set<Condition> result;
//     std::vector<PLSet::ClusterCollection::value_type> good_clusters;
//     for (auto& [cluster_value, cluster] : curr_comb_pls->GetClusters()) {
//         std::vector<int> included_cluster;
//         std::set_intersection(included_pos.begin(), included_pos.end(), cluster.begin(),
//                               cluster.end(), std::back_inserter(included_cluster));
//         if (double recall = (double)included_cluster.size() / included_pos.size();
//             recall > recall_) {
//             good_clusters.emplace_back(cluster_value, cluster);
//             if (double prec = (double)included_cluster.size() / cluster.size(); prec > precision_) {
//                 result.emplace(new_curr_attrs, cluster_value, recall, prec);
//             }
//         }
//     }
//     if (!good_clusters.empty()) {
//         PLSetShared new_curr_pls = PLSet::CreateFor(std::move(good_clusters));
//         for (size_t next_attr_idx = attr_idx + 1; next_attr_idx < included_pos.size();
//              ++next_attr_idx) {
//             for (auto& cond :
//                  Analyze(cond_attrs, next_attr_idx, new_curr_attrs, new_curr_pls, included_pos)) {
//                 result.insert(cond);
//             }
//         }
//     }
//     return result;
// }
}  // namespace algos::cind
