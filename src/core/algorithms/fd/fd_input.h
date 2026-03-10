#pragma once

#include <string>
#include <variant>
#include <vector>

#include "core/model/index.h"
#include "core/util/export.h"

namespace model {
struct DESBORDANTE_EXPORT FdInput {
    std::vector<std::variant<std::string, Index>> lhs;
    std::vector<std::variant<std::string, Index>> rhs;

    FdInput(std::vector<std::variant<std::string, Index>> lhs,
            std::vector<std::variant<std::string, Index>> rhs)
        : lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    // This constructor only exists because otherwise specifying FdInput
    // in-place from C++ becomes overly verbose (see FDVerifier unit tests).
    FdInput(std::initializer_list<Index> const& lhs, std::initializer_list<Index> const& rhs)
        : lhs(lhs.begin(), lhs.end()), rhs(rhs.begin(), rhs.end()) {}

    // This constructor only exists so that algorithms don't have to wrap
    // FdInput members in a std::unique_ptr.
    FdInput() = default;

    bool operator==(FdInput const& other) const {
        return lhs == other.lhs && rhs == other.rhs;
    }
};
}  // namespace model
