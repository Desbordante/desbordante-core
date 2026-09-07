#pragma once

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/algorithms/fd/fd_input.h"
#include "core/model/table/attribute.h"

namespace model {
struct FunctionalDependency {
    std::string table_name;
    std::vector<Attribute> lhs;
    std::vector<Attribute> rhs;

    FunctionalDependency(std::string table_name, std::vector<Attribute> lhs,
                         std::vector<Attribute> rhs)
        : table_name(std::move(table_name)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    bool operator==(FunctionalDependency const& other) const noexcept {
        return table_name == other.table_name && lhs == other.lhs && rhs == other.rhs;
    }

    FdInput ToInput() const {
        std::vector<std::variant<std::string, Index>> input_lhs;
        input_lhs.reserve(lhs.size());
        for (auto const& [name, id] : lhs) {
            input_lhs.push_back(id);
        }
        std::vector<std::variant<std::string, Index>> input_rhs;
        input_rhs.reserve(rhs.size());
        for (auto const& [name, id] : rhs) {
            input_rhs.push_back(id);
        }

        return {std::move(input_lhs), std::move(input_rhs)};
    }
};
}  // namespace model
