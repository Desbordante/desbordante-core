#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/model/types/type.h"

namespace algos::pac_verifier {
/// @brief Values that violate Domain PAC with given epsilon
class DomainPACHighlight {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;

    std::shared_ptr<pac::model::TupleType> tuple_type_;
    std::shared_ptr<Tuples> original_value_tuples_;
    std::vector<TuplesIter> highlighted_tuples_;

public:
    DomainPACHighlight(std::shared_ptr<pac::model::TupleType> tuple_type,
                       std::shared_ptr<Tuples> original_value_tuples,
                       std::vector<TuplesIter>&& highlighted_tuples)
        : tuple_type_(std::move(tuple_type)),
          original_value_tuples_(std::move(original_value_tuples)),
          highlighted_tuples_(std::move(highlighted_tuples)) {}

    /// @brief Get row numbers of highlighted values
    std::vector<std::size_t> GetRowNums() const {
        std::vector<std::size_t> indices(highlighted_tuples_.size());
        std::ranges::transform(highlighted_tuples_, indices.begin(),
                               [begin = original_value_tuples_->begin()](auto const it) {
                                   return std::distance(begin, it);
                               });
        return indices;
    }

    /// @brief Get @c Types of columns associated with this @c Highlight
    std::vector<model::Type const*> const& GetTypes() const {
        return tuple_type_->GetTypes();
    }

    /// @brief Get highlighted values as pointers to @c std::byte, that can be used with types (see
    /// @c GetTypes())
    Tuples GetByteData() const {
        Tuples tuples;
        tuples.reserve(highlighted_tuples_.size());
        std::ranges::transform(highlighted_tuples_, std::back_inserter(tuples),
                               [](auto const it) { return *it; });
        return tuples;
    }

    /// @brief Get highlighted values as strings
    std::vector<std::string> GetStringData() const {
        std::vector<std::string> strings;
        strings.reserve(highlighted_tuples_.size());
        std::ranges::transform(highlighted_tuples_, std::back_inserter(strings),
                               [this](auto const it) { return tuple_type_->ValueToString(*it); });
        return strings;
    }
};
}  // namespace algos::pac_verifier
