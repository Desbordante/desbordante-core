#pragma once

#include <ind/ind.h>

#include "condition.h"

namespace algos::cind {
struct Cind {
    model::IND const& ind;
    std::vector<Condition> conditions;

    std::string ToString() const {
        std::string result = ind.ToLongString();
        result.append("\nPossible conditions:\n");
        for (auto const& condition : conditions) {
            result.append("\t").append(condition.ToString()).append(";\n");
        }
        return result;
    }
};
}  // namespace algos::cind