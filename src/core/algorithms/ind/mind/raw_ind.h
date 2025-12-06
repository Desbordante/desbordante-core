/** \file
 * \brief Mind algorithm
 *
 * Mind-specific IND representation.
 */
#pragma once

#include <sstream>

#include "core/model/table/column_combination.h"

namespace algos::mind {

struct RawIND {
    model::ColumnCombination lhs;
    model::ColumnCombination rhs;

    bool operator==(RawIND const& other) const noexcept {
        return lhs == other.lhs && rhs == other.rhs;
    }

    bool operator!=(RawIND const& other) const noexcept {
        return !(*this == other);
    }
};

}  // namespace algos::mind

template <>
struct std::hash<algos::mind::RawIND> {
    size_t operator()(algos::mind::RawIND const& ind) const {
        size_t hash = 0;
        hash ^= ind.lhs.GetHash();
        hash = std::rotl(hash, 11) ^ ind.rhs.GetHash();
        return hash;
    }
};
