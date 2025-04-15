#include "pli_cind.h"

#include <map>
#include <set>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cind/condition.h"
#include "cind/condition_miners/position_lists_set.h"
#include "cind/condition_type.h"
#include "table/column.h"
#include "table/encoded_column_data.h"
#include "table/table_index.h"

namespace algos::cind {
using model::TableIndex;

PliCind::PliCind(config::InputTables& input_tables) : CindMiner(input_tables) {}

CIND PliCind::ExecuteSingle(model::IND const& aind) {
    auto attributes = ClassifyAttributes(aind);
    fprintf(stderr, "condition attributes: [");
    for (auto const attr : attributes.conditional) {
        fprintf(stderr, "%s::%s, ", attr->GetColumn()->GetSchema()->GetName().c_str(),
                attr->GetColumn()->GetName().c_str());
    }
    fprintf(stderr, "]\n");
    CIND result{.ind = aind,
                .conditions = GetConditions(attributes),
                .conditional_attributes = GetConditionalAttributesNames(attributes.conditional)};
    Reset();
    return result;
}

std::pair<std::vector<int>, std::vector<int>> PliCind::ClassifyRows(Attributes const& attrs) {
    std::set<std::vector<std::string>> rhs_values;
    fprintf(stderr, "rhs values: [");
    for (size_t index = 0; index < attrs.rhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        fprintf(stderr, "{");
        for (auto& attr : attrs.rhs_inclusion) {
            row.push_back(attr->GetStringValue(index));
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}, ");
        rhs_values.insert(std::move(row));
    }
    fprintf(stderr, "]\n");

    std::vector<int> included_pos;
    std::map<std::vector<std::string>, int> group_idx;
    std::vector<int> row_to_group;
    fprintf(stderr, "lhs values: [");
    for (size_t index = 0; index < attrs.lhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        for (auto& attr : attrs.lhs_inclusion) {
            row.push_back(attr->GetStringValue(index));
        }
        int row_id = index;
        if (condition_type_._value == CondType::group) {
            if (auto const& it = group_idx.find(row); it == group_idx.end()) {
                row_id = group_idx.size();
                group_idx[row] = row_id;
                row_to_group.push_back(row_id);
            } else {
                row_to_group.push_back(it->second);
                continue;
            }
        }
        fprintf(stderr, "{");
        for (auto& attr : attrs.lhs_inclusion) {
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}");
        if (rhs_values.contains(row)) {
            included_pos.push_back(row_id);
            fprintf(stderr, "::included");
        }
        fprintf(stderr, ", ");
    }
    fprintf(stderr, "]\n");
    if (condition_type_._value == CondType::group) {
        relation_size_ = group_idx.size();
    } else {
        relation_size_ = attrs.lhs_inclusion.front()->GetNumRows();
    }
    return {included_pos, row_to_group};
}

void PliCind::MakePLs(Attributes const& attrs, std::vector<int> const& row_to_group) {
    attr_idx_to_pls_.reserve(attrs.conditional.size());
    for (auto const& attr : attrs.conditional) {
        attr_idx_to_pls_.push_back(
                model::PLSet::CreateFor(attr->GetValues(), row_to_group, relation_size_));
    }
}

std::vector<Condition> PliCind::GetConditions(Attributes const& attrs) {
    auto const& [included_pos, row_to_group] = ClassifyRows(attrs);

    fprintf(stderr, "included positions: [");
    for (auto const pos : included_pos) {
        fprintf(stderr, "%d, ", pos);
    }
    fprintf(stderr, "]\n");

    MakePLs(attrs, row_to_group);

    std::vector<Condition> result;
    for (size_t attr_idx = 0; attr_idx < attrs.conditional.size(); ++attr_idx) {
        auto conditions = Analyze(attr_idx, {}, nullptr, attrs.conditional, included_pos);
        for (auto& cond : conditions) {
            result.push_back(std::move(cond));
        }
    }

    return result;
}

std::vector<Condition> PliCind::Analyze(size_t attr_idx, std::vector<int> curr_attrs,
                                        PLSetShared const& curr_pls, AttrsType const& cond_attrs,
                                        std::vector<int> const& included_pos) {
    std::vector<int> new_curr_attrs = curr_attrs;
    new_curr_attrs.push_back(attr_idx);
    PLSetShared curr_comb_pls = attr_idx_to_pls_.at(attr_idx);
    if (!curr_attrs.empty()) {
        curr_comb_pls = curr_pls->Intersect(attr_idx_to_pls_.at(attr_idx).get());
    }

    std::vector<Condition> result;
    std::vector<PLSet::ClusterCollection::value_type> good_clusters;
    for (auto& [cluster_value, cluster] : curr_comb_pls->GetClusters()) {
        std::vector<int> included_cluster;
        // both included_pos and cluster are sorted
        std::set_intersection(included_pos.begin(), included_pos.end(), cluster.begin(),
                              cluster.end(), std::back_inserter(included_cluster));
        if (double completeness = (double)included_cluster.size() / included_pos.size();
            completeness >= min_completeness_) {
            // LOG("included_pos: [");
            // for (const auto & elem : included_pos) {
            //     LOG("%d, ", elem);
            // }
            // LOG("]\n");
            // LOG("cluster: [");
            // for (const auto & elem : cluster) {
            //     LOG("%d, ", elem);
            // }
            // LOG("]\n");
            // LOG("included_cluster: [");
            // for (const auto & elem : included_cluster) {
            //     LOG("%d, ", elem);
            // }
            // LOG("], completeness: %f, validity: %f\n", (double)included_cluster.size() / included_pos.size(), (double)included_cluster.size() / cluster.size());
            good_clusters.emplace_back(cluster_value, cluster);
            if (double validity = (double)included_cluster.size() / cluster.size();
                validity >= min_validity_) {
                result.emplace_back(new_curr_attrs, cluster_value, cond_attrs, completeness,
                                    validity);
                // LOG("Result emplaced: %s\n", result.back().ToString().c_str());
            }
            // LOG("\n");
        }
    }
    if (!good_clusters.empty()) {
        PLSetShared new_curr_pls = PLSet::CreateFor(std::move(good_clusters), relation_size_);
        for (size_t next_attr_idx = attr_idx + 1; next_attr_idx < cond_attrs.size();
             ++next_attr_idx) {
            auto conditions =
                    Analyze(next_attr_idx, new_curr_attrs, new_curr_pls, cond_attrs, included_pos);

            for (auto& cond : conditions) {
                result.push_back(std::move(cond));
            }
        }
    }
    return result;
}

void PliCind::Reset() {
    attr_idx_to_pls_.clear();
    relation_size_ = 0;
}
}  // namespace algos::cind
