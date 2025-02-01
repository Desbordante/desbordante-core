#include "pli_cind.h"

#include <algorithm>
#include <set>
#include <utility>

#include "ind/cind/condition_miners/cind_miner.hpp"
#include "table/column_combination.h"
#include "table/encoded_column_data.h"
#include "table/table_index.h"

namespace algos::cind {
using model::TableIndex;

namespace {
std::vector<int> GetIncludedPositions(AttrsType const& lhs_inclusion_attrs,
                                      AttrsType const& rhs_inclusion_attrs) {
    std::set<std::vector<std::string>> rhs_values;
    fprintf(stderr, "rhs values: [");
    for (size_t index = 0; index < rhs_inclusion_attrs.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        fprintf(stderr, "{");
        for (auto& attr : rhs_inclusion_attrs) {
            row.push_back(attr->GetStringValue(index));
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}, ");
        rhs_values.insert(std::move(row));
    }
    fprintf(stderr, "]\n");

    std::vector<int> result;
    fprintf(stderr, "lhs values: [");
    for (size_t index = 0; index < lhs_inclusion_attrs.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        fprintf(stderr, "{");
        for (auto& attr : lhs_inclusion_attrs) {
            row.push_back(attr->GetStringValue(index));
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}");
        if (rhs_values.contains(row)) {
            result.push_back(index);
            fprintf(stderr, "::included");
        }
        fprintf(stderr, ", ");
    }
    fprintf(stderr, "]\n");
    return result;
}
}  // namespace

PliCind::PliCind(config::InputTables& input_tables) : CindMiner(input_tables) {}

void PliCind::ExecuteSingle(model::IND const& aind) {
    auto const [included_pos, cond_attrs] = ScanDomains(aind);
    fprintf(stderr, "included positions: [");
    for (auto const pos : included_pos) {
        fprintf(stderr, "%d, ", pos);
    }
    fprintf(stderr, "]\ncondition attributes: [");
    for (auto const attr : cond_attrs) {
        fprintf(stderr, "%s::%s, ", attr->GetColumn()->GetSchema()->GetName().c_str(), attr->GetColumn()->GetName().c_str());
    }
    fprintf(stderr, "]\n");
}

void PliCind::MakePLs(std::vector<int> const& /*cond_attrs*/) {
    return;
}

std::pair<std::vector<int>, AttrsType> PliCind::ScanDomains(model::IND const& aind) const {
    AttrsType lhs_inclusion_attrs, rhs_inclusion_attrs;
    AttrsType condition_attrs;
    auto process_column = [&](EncodedColumnData const& column, model::ColumnCombination const& cc,
                              AttrsType& inclusion_attrs_dst) {
        auto const& ind_columns = cc.GetColumnIndices();
        if (std::find(ind_columns.cbegin(), ind_columns.cend(), column.GetColumn()->GetIndex()) !=
            ind_columns.cend()) {
            inclusion_attrs_dst.push_back(&column);
        } else {
            condition_attrs.push_back(&column);
        }
    };

    for (auto const& column : tables_.at(aind.GetLhs().GetTableIndex())->GetColumnData()) {
        process_column(column, aind.GetLhs(), lhs_inclusion_attrs);
    }
    for (auto const& column : tables_.at(aind.GetRhs().GetTableIndex())->GetColumnData()) {
        process_column(column, aind.GetRhs(), rhs_inclusion_attrs);
    }

    std::vector<int> included_pos = GetIncludedPositions(lhs_inclusion_attrs, rhs_inclusion_attrs);

    return std::make_pair(included_pos, condition_attrs);
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
//             if (double prec = (double)included_cluster.size() / cluster.size(); prec >
//             precision_) {
//                 result.emplace(new_curr_attrs, cluster_value, recall, prec);
//             }
//         }
//     }
//     if (!good_clusters.empty()) {
//         PLSetShared new_curr_pls = PLSet::CreateFor(std::move(good_clusters));
//         for (size_t next_attr_idx = attr_idx + 1; next_attr_idx < included_pos.size();
//              ++next_attr_idx) {
//             for (auto& cond :
//                  Analyze(cond_attrs, next_attr_idx, new_curr_attrs, new_curr_pls, included_pos))
//                  {
//                 result.insert(cond);
//             }
//         }
//     }
//     return result;
// }
}  // namespace algos::cind
