#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "condition_miners/itemset_node.h"
#include "condition_miners/position_lists_set.h"
#include "core/model/table/encoded_column_data.h"

namespace algos::cind {
char const* const kAnyValue = "-";

struct Condition {
    std::vector<std::string> condition_attrs_values;
    double validity;
    double completeness;

    static constexpr double kDoubleEps = 1e-9;

    static std::int64_t QuantizeDouble(double v) noexcept {
        return static_cast<std::int64_t>(std::llround(v / kDoubleEps));
    }

    Condition(std::shared_ptr<ItemsetNode> itemset,
              std::vector<model::EncodedColumnData const*> const& condition_attrs)
        : validity(itemset->GetValidity()), completeness(itemset->GetCompleteness()) {
        condition_attrs_values.resize(condition_attrs.size(), kAnyValue);

        ItemsetNode const* item_ptr = itemset.get();
        for (size_t column_id = condition_attrs.size(); column_id-- > 0;) {
            auto const& attribute = condition_attrs[column_id];

            if (auto const& item = item_ptr->GetValue(); attribute->GetColumnId() == item.column_id) {
                condition_attrs_values[column_id] = attribute->DecodeValue(item.value);

                item_ptr = item_ptr->GetParent().get();
                if (item_ptr == nullptr || item_ptr->GetParent() == nullptr) {
                    break;
                }
            }
        }
    }

    Condition(std::vector<int> const& cluster_attrs_ids,
              model::PLSet::ClusterValue const& cluster_value,
              std::vector<model::EncodedColumnData const*> const& condition_attrs, double _validity,
              double _completeness)
        : validity(_validity), completeness(_completeness) {
        condition_attrs_values.reserve(condition_attrs.size());

        size_t item_id = 0;
        for (size_t column_id = 0; column_id < condition_attrs.size(); ++column_id) {
            if (item_id < cluster_attrs_ids.size() &&
                column_id == static_cast<size_t>(cluster_attrs_ids[item_id])) {
                condition_attrs_values.push_back(
                        condition_attrs[column_id]->DecodeValue(cluster_value[item_id++]));
            } else {
                condition_attrs_values.push_back(kAnyValue);
            }
        }
    }

    std::string ToString() const {
        std::string result = "(";
        for (auto const& value : condition_attrs_values) {
            result.append("\"").append(value).append("\", ");
        }
        result.append("validity = ")
                .append(std::to_string(validity))
                .append(", completeness = ")
                .append(std::to_string(completeness))
                .append(")");
        return result;
    }

    bool operator==(Condition const& that) const {
        return QuantizeDouble(this->validity) == QuantizeDouble(that.validity) &&
               QuantizeDouble(this->completeness) == QuantizeDouble(that.completeness) &&
               this->condition_attrs_values == that.condition_attrs_values;
    }
};
}  // namespace algos::cind

template <>
struct std::hash<algos::cind::Condition> {
    size_t operator()(algos::cind::Condition const& cond) const noexcept {
        size_t result = 0;

        boost::hash_combine(result, boost::hash_value(algos::cind::Condition::QuantizeDouble(cond.validity)));
        boost::hash_combine(result,
                            boost::hash_value(algos::cind::Condition::QuantizeDouble(cond.completeness)));
        boost::hash_combine(result,
                            boost::hash_range(cond.condition_attrs_values.begin(),
                                              cond.condition_attrs_values.end()));

        return result;
    }
};
