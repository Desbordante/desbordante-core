#pragma once

#include <sstream>

#include "condition.h"
#include "core/algorithms/ind/ind.h"

namespace algos::cind {
struct CIND {
    model::IND const& ind;
    std::vector<Condition> conditions;
    std::vector<std::string> conditional_attributes;

    std::string ToString() const {
        std::ostringstream oss;
        oss << ind.ToLongString() << "\nPossible conditions number: " << conditions.size() << "\n";
        if (conditions.empty()) {
            return oss.str();
        }
        oss << "Possible conditions:\n\t(";
        for (size_t i = 0; i < conditional_attributes.size(); ++i) {
            if (i != 0) oss << ", ";
            oss << conditional_attributes[i];
        }
        oss << ");\n";
        for (auto const& condition : conditions) {
            oss << "\t" << condition.ToString() << ";\n";
        }
        return oss.str();
    }

    size_t ConditionsNumber() const noexcept {
        return conditions.size();
    }

    bool operator==(const CIND& that) const {
        return this->conditions == that.conditions && &this->ind == &that.ind;
    }

    size_t Hash() const {
        static auto const kCondHasher = std::hash<Condition>{};
        size_t result = boost::hash_value(&ind);
        for (auto const& cond : conditions) {
            boost::hash_combine(result, kCondHasher(cond));
        }
        return result;
    }
};

}  // namespace algos::cind
