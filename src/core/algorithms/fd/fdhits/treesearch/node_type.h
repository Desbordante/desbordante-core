#pragma once

#include <cstddef>
#include <variant>

namespace algos::fd::fdhits {
using NodeType = std::variant<size_t, std::monostate>;

namespace node_type {

inline NodeType LHS(size_t index) {
    return NodeType{index};
}

inline NodeType RHS() {
    return NodeType{std::monostate{}};
}

inline bool IsLHS(NodeType const& node) {
    return std::holds_alternative<size_t>(node);
}

inline bool IsRHS(NodeType const& node) {
    return std::holds_alternative<std::monostate>(node);
}

inline size_t GetLHS(NodeType const& node) {
    return std::get<size_t>(node);
}

}  // namespace node_type
}  // namespace algos::fd::fdhits
