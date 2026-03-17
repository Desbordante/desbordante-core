#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "core/algorithms/fd/fdhits/treesearch/hypergraph.h"
#include "core/algorithms/fd/fdhits/treesearch/node_type.h"

namespace algos::fd::fdhits {

template <typename E>
class Validator {
public:
    virtual ~Validator() = default;

    virtual std::pair<bool, std::optional<Hypergraph<E>>> Check(
            E const& candidate, std::vector<NodeType> const& columns,
            std::vector<E> const& cand) = 0;

    virtual void Push(std::vector<NodeType> const& columns) = 0;
};

}  // namespace algos::fd::fdhits
