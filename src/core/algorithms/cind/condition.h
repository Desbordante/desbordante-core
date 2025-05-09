#pragma once

#include <memory>
#include <string>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "cind/condition_miners/itemset_node.h"
#include "table/encoded_column_data.h"

namespace algos::cind {
char const* const kAnyValue = "-";

struct Condition {
    std::vector<std::string> condition_attrs_values;
    double validity;
    double completeness;

    Condition(std::shared_ptr<ItemsetNode> itemset,
              std::vector<model::EncodedColumnData const*> const& condition_attrs)
        : validity(itemset->GetValidity()), completeness(itemset->GetCompleteness()) {
        condition_attrs_values.resize(condition_attrs.size(), kAnyValue);
        ItemsetNode const* item_ptr = itemset.get();
        for (size_t column_id = condition_attrs.size() - 1; column_id >= 0; --column_id) {
            auto const& attribute = condition_attrs[column_id];
            if (auto const& item = item_ptr->GetValue();
                attribute->GetColumnId() == item.column_id) {
                condition_attrs_values[column_id] = attribute->DecodeValue(item.value);
                item_ptr = item_ptr->GetParent().get();
                if (item_ptr->GetParent() == nullptr) {
                    break;
                }
            }
        }
    }

    Condition(std::vector<int> const& cluster_attrs_ids, std::vector<int> const& cluster_value,
              std::vector<model::EncodedColumnData const*> const& condition_attrs, double _validity,
              double _completeness)
        : validity(_validity), completeness(_completeness) {
        condition_attrs_values.reserve(condition_attrs.size());
        size_t item_id = 0;
        for (size_t column_id = 0; column_id < condition_attrs.size(); ++column_id) {
            if (item_id < cluster_attrs_ids.size() &&
                column_id == (size_t)cluster_attrs_ids[item_id]) {
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
        return this->completeness == that.completeness && this->validity == that.validity &&
               this->condition_attrs_values == that.condition_attrs_values;
    }

    ~Condition() = default;
};
}  // namespace algos::cind

template <>
struct std::hash<algos::cind::Condition> {
    size_t operator()(algos::cind::Condition const& cond) const noexcept {
        size_t result = 0;
        boost::hash_combine(result, boost::hash_value(cond.validity));
        boost::hash_combine(result, boost::hash_value(cond.completeness));
        boost::hash_combine(result, boost::hash_value(cond.completeness));
        return result;  // or use boost::hash_combine
    }
};