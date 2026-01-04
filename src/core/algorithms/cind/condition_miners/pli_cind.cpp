#include "pli_cind.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "core/algorithms/cind/condition.h"
#include "core/algorithms/cind/condition_miners/position_lists_set.h"
#include "core/algorithms/cind/types.h"
#include "core/model/table/encoded_column_data.h"

namespace algos::cind {

namespace {
struct VectorStringHash {
    std::size_t operator()(std::vector<std::string> const& vec) const noexcept {
        return boost::hash_value(vec);
    }
};
}  // namespace

PliCind::PliCind(config::InputTables& input_tables) : CindMiner(input_tables) {}

CIND PliCind::ExecuteSingle(model::IND const& aind) {
    auto attributes = ClassifyAttributes(aind);
    CIND result{.ind = aind,
                .conditions = GetConditions(attributes),
                .conditional_attributes = GetConditionalAttributesNames(attributes.conditional)};
    Reset();
    return result;
}

std::pair<std::vector<int>, std::vector<int>> PliCind::ClassifyRows(Attributes const& attrs) {
    std::unordered_set<std::vector<std::string>, VectorStringHash> rhs_values;
    for (size_t index = 0; index < attrs.rhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        row.reserve(attrs.rhs_inclusion.size());
        for (auto& attr : attrs.rhs_inclusion) {
            row.push_back(attr->GetStringValue(index));
        }
        rhs_values.insert(std::move(row));
    }

    // included row_id for rows, group_id for groups
    std::vector<int> included_pos;

    std::unordered_map<std::vector<std::string>, int, VectorStringHash> group_idx;
    std::vector<int> row_to_group;
    row_to_group.reserve(attrs.lhs_inclusion.front()->GetNumRows());

    for (size_t index = 0; index < attrs.lhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        row.reserve(attrs.lhs_inclusion.size());
        for (auto& attr : attrs.lhs_inclusion) {
            row.push_back(attr->GetStringValue(index));
        }

        int included_pos_id = static_cast<int>(index);

        if (condition_type_._value == CondType::group) {
            auto it = group_idx.find(row);
            if (it == group_idx.end()) {
                included_pos_id = static_cast<int>(group_idx.size());
                group_idx.emplace(row, included_pos_id);
                row_to_group.push_back(included_pos_id);
            } else {
                row_to_group.push_back(it->second);
                continue;
            }
        }

        if (rhs_values.contains(row)) {
            included_pos.push_back(included_pos_id);
        }
    }

    return {included_pos, row_to_group};
}

void PliCind::MakePLs(Attributes const& attrs) {
    relation_size_ = attrs.lhs_inclusion.front()->GetNumRows();
    attr_idx_to_pls_.reserve(attrs.conditional.size());
    for (auto const& attr : attrs.conditional) {
        attr_idx_to_pls_.push_back(model::PLSet::CreateFor(attr->GetValues(), relation_size_));
    }
}

std::vector<Condition> PliCind::GetConditions(Attributes const& attrs) {
    auto const& [included_pos, row_to_group] = ClassifyRows(attrs);
    if (included_pos.empty()) {
        return {};
    }

    MakePLs(attrs);

    std::vector<Condition> result;
    std::vector<int> empty_attrs;

    for (size_t attr_idx = 0; attr_idx < attrs.conditional.size(); ++attr_idx) {
        auto conditions = Analyze(attr_idx, empty_attrs, nullptr, attrs.conditional, row_to_group,
                                  included_pos);

        result.insert(result.end(), std::make_move_iterator(conditions.begin()),
                      std::make_move_iterator(conditions.end()));
    }

    return result;
}

std::vector<Condition> PliCind::Analyze(size_t attr_idx, std::vector<int> const& curr_attrs,
                                        PLSetShared const& curr_pls, AttrsType const& cond_attrs,
                                        std::vector<int> const& row_to_group,
                                        std::vector<int> const& included_pos) {
    std::vector<int> new_curr_attrs = curr_attrs;
    new_curr_attrs.push_back(static_cast<int>(attr_idx));

    PLSetShared curr_comb_pls = attr_idx_to_pls_.at(attr_idx);

    bool const is_first_level = curr_attrs.empty();
    if (!is_first_level) {
        curr_comb_pls = curr_pls->Intersect(attr_idx_to_pls_.at(attr_idx).get());
    }

    std::vector<Condition> result;
    std::vector<PLSet::ClusterCollection::value_type> good_clusters;

    for (auto& [cluster_value, cluster] : curr_comb_pls->GetClusters()) {
        std::vector<int> included_cluster;
        included_cluster.reserve(std::min(cluster.size(), included_pos.size()));

        size_t cluster_size = cluster.size();

        if (condition_type_._value == CondType::group) {
            std::vector<int> group_cluster;
            group_cluster.reserve(cluster.size());
            for (int row_id : cluster) {
                group_cluster.push_back(row_to_group.at(static_cast<size_t>(row_id)));
            }
            std::sort(group_cluster.begin(), group_cluster.end());
            group_cluster.erase(std::unique(group_cluster.begin(), group_cluster.end()),
                               group_cluster.end());

            std::set_intersection(included_pos.begin(), included_pos.end(), group_cluster.begin(),
                                  group_cluster.end(), std::back_inserter(included_cluster));
            cluster_size = group_cluster.size();
        } else {
            // both included_pos and cluster are sorted
            std::set_intersection(included_pos.begin(), included_pos.end(), cluster.begin(),
                                  cluster.end(), std::back_inserter(included_cluster));
        }

        double const completeness =
                static_cast<double>(included_cluster.size()) / static_cast<double>(included_pos.size());

        if (completeness >= min_completeness_) {
            good_clusters.emplace_back(
                    cluster_value, is_first_level ? cluster : std::move(cluster));

            double const validity =
                    static_cast<double>(included_cluster.size()) / static_cast<double>(cluster_size);

            if (validity >= min_validity_) {
                result.emplace_back(new_curr_attrs, cluster_value, cond_attrs, validity, completeness);
            }
        }
    }

    if (!good_clusters.empty()) {
        PLSetShared new_curr_pls = PLSet::CreateFor(std::move(good_clusters), relation_size_);

        for (size_t next_attr_idx = attr_idx + 1; next_attr_idx < cond_attrs.size(); ++next_attr_idx) {
            auto conditions = Analyze(next_attr_idx, new_curr_attrs, new_curr_pls, cond_attrs,
                                      row_to_group, included_pos);

            result.insert(result.end(), std::make_move_iterator(conditions.begin()),
                          std::make_move_iterator(conditions.end()));
        }
    }

    return result;
}

void PliCind::Reset() {
    attr_idx_to_pls_.clear();
    relation_size_ = static_cast<size_t>(0);
}

}  // namespace algos::cind
