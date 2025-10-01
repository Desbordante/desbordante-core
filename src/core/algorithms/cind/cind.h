#pragma once

#include <ind/ind.h>

#include "condition.h"

namespace algos::cind {
struct CIND {
    model::IND const& ind;
    std::vector<Condition> conditions;
    std::vector<std::string> conditional_attributes;

    std::string ToString() const {
        std::string result = ind.ToLongString();
        result.append("\nPossible conditions number: ");
        result.append(std::to_string(conditions.size()));
        result.append("\n");
        if (conditions.size()) {
            result.append("Possible conditions:\n\t(");
            for (auto const& attr : conditional_attributes) {
                result.append(attr).append(", ");
            }
            result.resize(result.size() - 2);
            result.append(");\n");
            for (auto const& condition : conditions) {
                result.append("\t").append(condition.ToString()).append(";\n");
            }
        }
        return result;
    }

    size_t ConditionsNumber() const noexcept {
        return conditions.size();
    }

    bool operator==(const CIND& that) const {
        return this->conditions == that.conditions && &this->ind == &that.ind;
    }

    size_t Hash() const {
        static const auto kCondHasher = std::hash<Condition>{};
        size_t result = boost::hash_value(&ind);
        for (const auto& cond : conditions) {
            boost::hash_combine(result, kCondHasher(cond));
        }
        return result;
    }
};
}  // namespace algos::cind