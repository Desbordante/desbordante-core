#pragma once

#include <string>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "cind/condition_miners/itemset.h"
#include "table/encoded_column_data.h"

namespace algos::cind {
char const* const kAnyValue = "-";

struct Condition {
    std::vector<std::string> condition_attrs_values;
    double validity;
    double completeness;

    Condition(Itemset itemset, std::vector<model::EncodedColumnData const*> const& condition_attrs,
              double _validity, double _completeness)
        : validity(_validity), completeness(_completeness) {
        condition_attrs_values.reserve(condition_attrs.size());
        size_t item_id = 0;
        for (size_t column_id = 0; column_id < condition_attrs.size(); ++column_id) {
            auto const& attribute = condition_attrs[column_id];
            while (item_id < itemset.GetSize() &&
                   attribute->GetColumnId() > itemset.GetItem(item_id).column_id) {
                ++item_id;
            }
            if (item_id < itemset.GetSize() &&
                attribute->GetColumnId() == itemset.GetItem(item_id).column_id) {
                condition_attrs_values.push_back(
                        attribute->DecodeValue(itemset.GetItem(item_id++).value));
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